/*
 * \brief  Genode backend for GDBServer (C++)
 * \author Christian Prochaska
 * \author Norman Feske
 * \date   2011-05-06
 */

/*
 * Copyright (C) 2011-2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/env.h>
#include <base/attached_rom_dataspace.h>

/* GDB monitor includes */
#include "app_child.h"
#include "cpu_thread_component.h"
#include "genode_child_resources.h"

/* libc includes */
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "genode-low.h"
#include "server.h"
#include "linux-low.h"

static bool verbose = false;

Genode::Env *genode_env;

/*
 * 'waitpid()' is implemented using 'select()'. When a new thread is created,
 * 'select()' needs to unblock, so there is a dedicated pipe for that. The
 * lwpid of the new thread needs to be read from the pipe in 'waitpid()', so
 * that the next 'select()' call can block again. The lwpid needs to be stored
 * in a variable until it is inquired later.
 */
static int _new_thread_pipe[2];
static unsigned long _new_thread_lwpid;

/*
 * When 'waitpid()' reports a SIGTRAP, this variable stores the lwpid of the
 * corresponding thread. This information is used in the initial breakpoint
 * handler to let the correct thread handle the event.
 */
static unsigned long _sigtrap_lwpid;

using namespace Genode;
using namespace Gdb_monitor;

class Memory_model
{
	private:

		Mutex _mutex { };

		Region_map_component &_address_space;

		Region_map           &_rm;

		/**
		 * Representation of a currently mapped region
		 */
		struct Mapped_region
		{
			Region_map_component::Region *_region;
			unsigned char                *_local_base;

			Mapped_region() : _region(0), _local_base(0) { }

			bool valid() { return _region != 0; }

			bool loaded(Region_map_component::Region const * region)
			{
				return _region == region;
			}

			void flush(Region_map &rm)
			{
				if (!valid()) return;
				rm.detach(_local_base);
				_local_base = 0;
				_region = 0;
			}

			void load(Region_map_component::Region *region, Region_map &rm)
			{
				if (region == _region)
					return;

				if (!region || valid())
					flush(rm);

				if (!region)
					return;

				try {
					_region     = region;
					_local_base = rm.attach(_region->ds_cap(),
					                        0, _region->offset());
				}
				catch (Region_map::Region_conflict) {
					flush(rm);
					error(__func__, ": RM attach failed (region conflict)");
				}
				catch (Region_map::Invalid_dataspace) {
					flush(rm);
					error(__func__, ": RM attach failed (invalid dataspace)");
				}
			}

			unsigned char *local_base() { return _local_base; }
		};

		enum { NUM_MAPPED_REGIONS = 1 };

		Mapped_region _mapped_region[NUM_MAPPED_REGIONS];

		unsigned _evict_idx = 0;

		/**
		 * Return local address of mapped region
		 *
		 * The function returns 0 if the mapping fails
		 */
		unsigned char *_update_curr_region(Region_map_component::Region *region)
		{
			for (unsigned i = 0; i < NUM_MAPPED_REGIONS; i++) {
				if (_mapped_region[i].loaded(region))
					return _mapped_region[i].local_base();
			}

			/* flush one currently mapped region */
			_evict_idx++;
			if (_evict_idx == NUM_MAPPED_REGIONS)
				_evict_idx = 0;

			_mapped_region[_evict_idx].load(region, _rm);

			return _mapped_region[_evict_idx].local_base();
		}

	public:

		Memory_model(Region_map_component &address_space,
		             Region_map           &rm)
		:
			_address_space(address_space),
			_rm(rm)
		{ }

		unsigned char read(void *addr)
		{
			Mutex::Guard guard(_mutex);

			addr_t offset_in_region = 0;

			Region_map_component::Region *region =
				_address_space.find_region(addr, &offset_in_region);

			unsigned char *local_base = _update_curr_region(region);

			if (!local_base) {
				warning(__func__, ": no memory at address ", addr);
				throw No_memory_at_address();
			}

			unsigned char value =
				local_base[offset_in_region];

			if (verbose)
				log(__func__, ": read addr=", addr, ", value=", Hex(value));

			return value;
		}

		void write(void *addr, unsigned char value)
		{
			if (verbose)
				log(__func__, ": write addr=", addr, ", value=", Hex(value));

			Mutex::Guard guard(_mutex);

			addr_t offset_in_region = 0;
			Region_map_component::Region *region =
				_address_space.find_region(addr, &offset_in_region);

			unsigned char *local_base = _update_curr_region(region);

			if (!local_base) {
				warning(__func__, ": no memory at address=", addr);
				warning("(attempted to write ", Hex(value), ")");
				throw No_memory_at_address();
			}

			local_base[offset_in_region] = value;
		}
};


static Genode_child_resources *_genode_child_resources = 0;
static Memory_model *_memory_model = 0;


Genode_child_resources &genode_child_resources()
{
	if (!_genode_child_resources) {
		Genode::error("_genode_child_resources is not set");
		abort();
	}

	return *_genode_child_resources;
}


/**
 * Return singleton instance of memory model
 */
Memory_model &memory_model()
{
	if (!_memory_model) {
		Genode::error("_memory_model is not set");
		abort();
	}

	return *_memory_model;
}


static void genode_stop_thread(unsigned long lwpid)
{
	Cpu_session_component &csc = genode_child_resources().cpu_session_component();

	Cpu_thread_component *cpu_thread = csc.lookup_cpu_thread(lwpid);

	if (!cpu_thread) {
		error(__PRETTY_FUNCTION__, ": "
		      "could not find CPU thread object for lwpid ", lwpid);
		return;
	}

	cpu_thread->pause();
}


pid_t my_waitpid(pid_t pid, int *status, int flags)
{
	extern int remote_desc;

	fd_set readset;

	Cpu_session_component &csc = genode_child_resources().cpu_session_component();

	while(1) {

		FD_ZERO (&readset);

		if (remote_desc != -1)
			FD_SET (remote_desc, &readset);

		if (pid == -1) {

			FD_SET(_new_thread_pipe[0], &readset);

			Thread_capability thread_cap = csc.first();

			while (thread_cap.valid()) {
				FD_SET(csc.signal_pipe_read_fd(thread_cap), &readset);
				thread_cap = csc.next(thread_cap);
			}

		} else {

			FD_SET(csc.signal_pipe_read_fd(csc.thread_cap(pid)), &readset);
		}

		struct timeval wnohang_timeout = {0, 0};
		struct timeval *timeout = (flags & WNOHANG) ? &wnohang_timeout : NULL;

		/* TODO: determine the highest fd in the set for optimization */
		int res = select(FD_SETSIZE, &readset, 0, 0, timeout);

		if (res > 0) {

			if ((remote_desc != -1) && FD_ISSET(remote_desc, &readset)) {

				/* received input from GDB */

				int cc;
				char c = 0;

				cc = read (remote_desc, &c, 1);

				if (cc == 1 && c == '\003' && current_thread != NULL) {
					/* this causes a SIGINT to be delivered to one of the threads */
					the_target->request_interrupt();
					continue;
				} else {
					if (verbose)
						log("input_interrupt, "
						    "count=", cc, " c=", c, " ('", Char(c), "%c')");
				}

			} else if (FD_ISSET(_new_thread_pipe[0], &readset)) {

				/*
				 * Linux 'ptrace(2)' manual text related to the main thread:
				 *
				 * "If the PTRACE_O_TRACEEXEC option is not in effect, all
				 *  successful calls to execve(2) by the traced process will
				 *  cause it to be sent a SIGTRAP signal, giving the parent a
				 *  chance to gain control before the new program begins
				 *  execution."
				 *
                 * Linux 'ptrace' manual text related to other threads:
                 *
                 * "PTRACE_O_CLONE
                 *  ...
                 *  A waitpid(2) by the tracer will return a status value such
                 *  that
                 *
                 *  status>>8 == (SIGTRAP | (PTRACE_EVENT_CLONE<<8))
                 *
                 *  The PID of the new process can be retrieved with
                 *  PTRACE_GETEVENTMSG."
                 */

				*status = W_STOPCODE(SIGTRAP);

				read(_new_thread_pipe[0], &_new_thread_lwpid,
				     sizeof(_new_thread_lwpid));

				if (_new_thread_lwpid != GENODE_MAIN_LWPID) {
					*status |= (PTRACE_EVENT_CLONE << 16);
					genode_stop_thread(GENODE_MAIN_LWPID);
				}

				return GENODE_MAIN_LWPID;

			} else {

				/* received a signal */

				Thread_capability thread_cap = csc.first();

				while (thread_cap.valid()) {
					if (FD_ISSET(csc.signal_pipe_read_fd(thread_cap), &readset))
						break;
					thread_cap = csc.next(thread_cap);
				}

				if (!thread_cap.valid())
					continue;

				int signal;
				read(csc.signal_pipe_read_fd(thread_cap), &signal, sizeof(signal));

				unsigned long lwpid = csc.lwpid(thread_cap);

				if (verbose)
					log("thread ", lwpid, " received signal ", signal);

				if (signal == SIGTRAP) {

					_sigtrap_lwpid = lwpid;

				} else if (signal == SIGSTOP) {

					/*
					 * Check if a SIGTRAP is pending
					 *
					 * This can happen if a single-stepped thread gets paused while gdbserver
					 * handles a signal of a different thread and the exception signal after
					 * the single step has not arrived yet. In this case, the SIGTRAP must be
					 * delivered first, otherwise gdbserver would single-step the thread again.
					 */

					Cpu_thread_component *cpu_thread = csc.lookup_cpu_thread(lwpid);

					Thread_state thread_state = cpu_thread->state();

					if (thread_state.exception) {
						/* resend the SIGSTOP signal */
						csc.send_signal(cpu_thread->cap(), SIGSTOP);
						continue;
					}

				} else if (signal == SIGINFO) {

					if (verbose)
						log("received SIGINFO for new lwpid ", lwpid);

					/*
					 * First signal of a new thread. On Genode originally a
					 * SIGTRAP, but gdbserver expects SIGSTOP.
					 */

					signal = SIGSTOP;
				}

				*status = W_STOPCODE(signal);

				return lwpid;
			}

		} else {

			return res;

		}
	}
}


extern "C" long ptrace(enum __ptrace_request request, pid_t pid, void *addr, void *data)
{
	const char *request_str = 0;

	switch (request) {
		case PTRACE_TRACEME:    request_str = "PTRACE_TRACEME";    break;
		case PTRACE_PEEKTEXT:   request_str = "PTRACE_PEEKTEXT";   break;
		case PTRACE_PEEKUSER:   request_str = "PTRACE_PEEKUSER";   break;
		case PTRACE_POKETEXT:   request_str = "PTRACE_POKETEXT";   break;
		case PTRACE_POKEUSER:   request_str = "PTRACE_POKEUSER";   break;
		case PTRACE_CONT:       request_str = "PTRACE_CONT";       break;
		case PTRACE_KILL:       request_str = "PTRACE_KILL";       break;
		case PTRACE_SINGLESTEP: request_str = "PTRACE_SINGLESTEP"; break;
		case PTRACE_GETREGS:    request_str = "PTRACE_GETREGS";    break;
		case PTRACE_SETREGS:    request_str = "PTRACE_SETREGS";    break;
		case PTRACE_ATTACH:     request_str = "PTRACE_ATTACH";     break;
		case PTRACE_DETACH:     request_str = "PTRACE_DETACH";     break;
		case PTRACE_GETSIGINFO: request_str = "PTRACE_GETSIGINFO"; break;
		case PTRACE_GETEVENTMSG:
			/*
			 * Only PTRACE_EVENT_CLONE is currently supported.
			 */
			*(unsigned long*)data = _new_thread_lwpid;
			return 0;
		case PTRACE_GETREGSET: request_str = "PTRACE_GETREGSET";  break;
	}

	warning("ptrace(", request_str, " (", Hex(request), ")) called - not implemented!");

	errno = EINVAL;
	return -1;
}


extern "C" int vfork()
{
	/* create the thread announcement pipe */

	if (pipe(_new_thread_pipe) != 0) {
		error("could not create the 'new thread' pipe");
		return -1;
	}

	/* extract target filename from config file */

	typedef String<32> Filename;
	Filename filename { };

	Genode::Attached_rom_dataspace config { *genode_env, "config" };

	try {
		Xml_node const target = config.xml().sub_node("target");

		if (!target.has_attribute("name")) {
			error("missing 'name' attribute of '<target>' sub node");
			return -1;
		}
		filename = target.attribute_value("name", Filename());

	} catch (Xml_node::Nonexistent_sub_node) {
		error("missing '<target>' sub node");
		return -1;
	}

	/* extract target node from config file */
	Xml_node target_node = config.xml().sub_node("target");

	/*
	 * preserve the configured amount of memory for gdb_monitor and give the
	 * remainder to the child
	 */
	Number_of_bytes preserved_ram_quota = 0;
	try {
		Xml_node preserve_node = config.xml().sub_node("preserve");
		if (preserve_node.attribute("name").has_value("RAM"))
			preserve_node.attribute("quantum").value(preserved_ram_quota);
		else
			throw Xml_node::Exception();
	} catch (...) {
		error("could not find a valid <preserve> config node");
		return -1;
	}

	Number_of_bytes ram_quota = genode_env->pd().avail_ram().value - preserved_ram_quota;

	Cap_quota const avail_cap_quota = genode_env->pd().avail_caps();

	Genode::size_t const preserved_caps = 100;

	if (avail_cap_quota.value < preserved_caps) {
		error("not enough available caps for preservation of ", preserved_caps);
		return -1;
	}

	Cap_quota const cap_quota { avail_cap_quota.value - preserved_caps };

	/* start the application */

	static Heap alloc(genode_env->ram(), genode_env->rm());

	enum { SIGNAL_EP_STACK_SIZE = 2*1024*sizeof(addr_t) };
	static Entrypoint signal_ep { *genode_env, SIGNAL_EP_STACK_SIZE,
	                              "sig_handler", Affinity::Location() };

	int breakpoint_len = 0;
	unsigned char const *breakpoint_data =
		the_target->sw_breakpoint_from_kind(0, &breakpoint_len);

	App_child *child = new (alloc) App_child(*genode_env,
	                                         alloc,
	                                         filename.string(),
	                                         Ram_quota{ram_quota},
	                                         cap_quota,
	                                         signal_ep,
	                                         target_node,
	                                         _new_thread_pipe[1],
	                                         breakpoint_len,
	                                         breakpoint_data);

	_genode_child_resources = child->genode_child_resources();

	static Memory_model memory_model(genode_child_resources().region_map_component(), genode_env->rm());

	_memory_model = &memory_model;

	try { child->start(); }
	catch (Out_of_caps)    { error("out of caps during child startup");    return -1; }
	catch (Out_of_ram)     { error("out of RAM during child startup");     return -1; }
	catch (Service_denied) { error("service denied during child startup"); return -1; }
	catch (...)            { error("could not start child process");       return -1; }

	return GENODE_MAIN_LWPID;
}


extern "C" int kill(pid_t pid, int sig)
{
	Cpu_session_component &csc = genode_child_resources().cpu_session_component();

	if (pid <= 0) pid = GENODE_MAIN_LWPID;

	Thread_capability thread_cap = csc.thread_cap(pid);

	if (!thread_cap.valid()) {
		error(__PRETTY_FUNCTION__, ": "
		      "could not find thread capability for pid ", pid);
		return -1;
	}

	return csc.send_signal(thread_cap, sig);
}


extern "C" int initial_breakpoint_handler(CORE_ADDR addr)
{
	Cpu_session_component &csc = genode_child_resources().cpu_session_component();
	return csc.handle_initial_breakpoint(_sigtrap_lwpid);
}


void genode_set_initial_breakpoint_at(unsigned long addr)
{
	set_breakpoint_at(addr, initial_breakpoint_handler);
}


void genode_remove_thread(unsigned long lwpid)
{
	struct thread_info *thread_info =
		find_thread_ptid(ptid_t(GENODE_MAIN_LWPID, lwpid, 0));
	lwp_info *lwp = get_thread_lwp(thread_info);
	the_linux_target->detach_one_lwp(lwp);
}


void genode_stop_all_threads()
{
	Cpu_session_component &csc = genode_child_resources().cpu_session_component();
	csc.pause_all_threads();
}


void genode_resume_all_threads()
{
	Cpu_session_component &csc = genode_child_resources().cpu_session_component();
	csc.resume_all_threads();
}


static int genode_detach(int)
{
    genode_resume_all_threads();

    return 0;
}


int linux_process_target::detach(process_info *process)
{
    return genode_detach(process->pid);
}


int linux_process_target::kill(process_info *process)
{
    /* TODO */
    if (verbose) warning(__func__, " not implemented, just detaching instead...");

    return genode_detach(process->pid);
}


void genode_continue_thread(unsigned long lwpid, int single_step)
{
	Cpu_session_component &csc = genode_child_resources().cpu_session_component();

	Cpu_thread_component *cpu_thread = csc.lookup_cpu_thread(lwpid);

	if (!cpu_thread) {
		error(__func__, ": " "could not find CPU thread object for lwpid ", lwpid);
		return;
	}

	cpu_thread->single_step(single_step);
	cpu_thread->resume();
}


void linux_process_target::fetch_registers(regcache *regcache, int regno)
{
	const struct regs_info *regs_info = get_regs_info();

	unsigned long reg_content = 0;

	if (regno == -1) {
		for (regno = 0; regno < regs_info->usrregs->num_regs; regno++) {
			if (genode_fetch_register(regno, &reg_content) == 0)
				supply_register(regcache, regno, &reg_content);
			else
				supply_register(regcache, regno, 0);
		}
	} else {
		if (genode_fetch_register(regno, &reg_content) == 0)
			supply_register(regcache, regno, &reg_content);
		else
			supply_register(regcache, regno, 0);
	}
}


void linux_process_target::store_registers(regcache *regcache, int regno)
{
	if (verbose) log(__func__, ": regno=", regno);

	const struct regs_info *regs_info = get_regs_info();

	unsigned long reg_content = 0;

	if (regno == -1) {
		for (regno = 0; regno < regs_info->usrregs->num_regs; regno++) {
			if ((Genode::size_t)register_size(regcache->tdesc, regno) <=
			    sizeof(reg_content)) {
				collect_register(regcache, regno, &reg_content);
				genode_store_register(regno, reg_content);
			}
		}
	} else {
		if ((Genode::size_t)register_size(regcache->tdesc, regno) <=
		    sizeof(reg_content)) {
			collect_register(regcache, regno, &reg_content);
			genode_store_register(regno, reg_content);
		}
	}
}


unsigned char genode_read_memory_byte(void *addr)
{
	return memory_model().read(addr);
}


int genode_read_memory(CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
	if (verbose)
		log(__func__, "(", Hex(memaddr), ", ", myaddr, ", ", len, ")");

	if (myaddr)
		try {
			for (int i = 0; i < len; i++)
				myaddr[i] = genode_read_memory_byte((void*)((unsigned long)memaddr + i));
		} catch (No_memory_at_address) {
			return EFAULT;
		}

	return 0;
}


int linux_process_target::read_memory(CORE_ADDR memaddr,
                                      unsigned char *myaddr, int len)
{
	return genode_read_memory(memaddr, myaddr, len);
}


void genode_write_memory_byte(void *addr, unsigned char value)
{
	memory_model().write(addr, value);
}


int genode_write_memory(CORE_ADDR memaddr, const unsigned char *myaddr, int len)
{
	if (verbose)
		log(__func__, "(", Hex(memaddr), ", ", myaddr, ", ", len, ")");

	if (myaddr && (len > 0)) {
		if (debug_threads) {
			/* Dump up to four bytes.  */
			unsigned int val = * (unsigned int *) myaddr;
			if (len == 1)
				val = val & 0xff;
			else if (len == 2)
				val = val & 0xffff;
			else if (len == 3)
				val = val & 0xffffff;
			fprintf(stderr, "Writing %0*x to 0x%08lx", 2 * ((len < 4) ? len : 4),
			        val, (long)memaddr);
		}

	for (int i = 0; i < len; i++)
		try {
			genode_write_memory_byte((void*)((unsigned long)memaddr + i), myaddr[i]);
		} catch (No_memory_at_address) {
			return EFAULT;
		}
	}

	return 0;
}


int linux_process_target::write_memory(CORE_ADDR memaddr,
                                        const unsigned char *myaddr, int len)
{
	return genode_write_memory(memaddr, myaddr, len);
}


LONGEST linux_common_xfer_osdata(char const*, gdb_byte*, ULONGEST, ULONGEST)
{
	Genode::error(__func__, " called, not implemented");
	return -1;
}


int linux_common_core_of_thread(ptid_t)
{
	return 0;
}


bool linux_process_target::supports_qxfer_libraries_svr4()
{
  return false;
}


int linux_process_target::qxfer_libraries_svr4(const char *,
                                               unsigned char *,
                                               unsigned const char *,
                                               CORE_ADDR, int)
{
	Genode::error(__func__, " called, not implemented");
	return -1;
}


char *linux_proc_pid_to_exec_file(int)
{
	return nullptr;
}


ssize_t linux_mntns_readlink(pid_t, const char *, char *, size_t)
{
	Genode::error(__func__, " called, not implemented");
	return -1;
}


int linux_mntns_unlink (pid_t, const char *)
{
	Genode::error(__func__, " called, not implemented");
	return -1;
}


int linux_mntns_open_cloexec(pid_t, const char *, int, mode_t)
{
	Genode::error(__func__, " called, not implemented");
	return -1;
}


const char *linux_proc_tid_get_name(ptid_t)
{
	return "";
}
