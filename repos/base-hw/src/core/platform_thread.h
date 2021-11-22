/*
 * \brief   Userland interface for the management of kernel thread-objects
 * \author  Martin Stein
 * \author  Stefan Kalkowski
 * \date    2012-02-02
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__PLATFORM_THREAD_H_
#define _CORE__PLATFORM_THREAD_H_

/* Genode includes */
#include <base/ram_allocator.h>
#include <base/thread.h>
#include <base/trace/types.h>

/* base-internal includes */
#include <base/internal/native_utcb.h>

/* core includes */
#include <address_space.h>
#include <object.h>

/* kernel includes */
#include <kernel/core_interface.h>
#include <kernel/thread.h>

namespace Genode {

	class Pager_object;
	class Thread_state;
	class Rm_client;
	class Platform_thread;
	class Platform_pd;
}


class Genode::Platform_thread : Noncopyable
{
	private:

		/*
		 * Noncopyable
		 */
		Platform_thread(Platform_thread const &);
		Platform_thread &operator = (Platform_thread const &);

		typedef String<32> Label;

		Label              const _label;
		Platform_pd *            _pd;
		Weak_ptr<Address_space>  _address_space  { };
		Pager_object *           _pager;
		Native_utcb *            _utcb_core_addr { }; /* UTCB addr in core */
		Native_utcb *            _utcb_pd_addr;       /* UTCB addr in pd   */
		Ram_dataspace_capability _utcb           { }; /* UTCB dataspace    */
		unsigned                 _priority       {0};
		unsigned                 _quota          {0};

		/*
		 * Wether this thread is the main thread of a program.
		 * This should be used only after 'join_pd' was called
		 * or if this is a core thread. For core threads its save
		 * also without 'join_pd' because '_main_thread' is initialized
		 * with 0 wich is always true as cores main thread has no
		 * 'Platform_thread'.
		 */
		bool _main_thread;

		Affinity::Location _location;

		Kernel_object<Kernel::Thread> _kobj;

		/**
		 * Common construction part
		 */
		void _init();

		/*
		 * Check if this thread will attach its UTCB by itself
		 */
		bool _attaches_utcb_by_itself();

		unsigned _scale_priority(unsigned virt_prio)
		{
			return Cpu_session::scale_priority(Kernel::Cpu_priority::max(),
			                                   virt_prio);
		}

		Platform_pd &_kernel_main_get_core_platform_pd();

	public:

		/**
		 * Constructor for core threads
		 *
		 * \param label       debugging label
		 * \param utcb        virtual address of UTCB within core
		 */
		Platform_thread(Label const &label, Native_utcb &utcb);

		/**
		 * Constructor for threads outside of core
		 *
		 * \param quota      CPU quota that shall be granted to the thread
		 * \param label      debugging label
		 * \param virt_prio  unscaled processor-scheduling priority
		 * \param utcb       core local pointer to userland stack
		 */
		Platform_thread(size_t const quota, Label const &label,
		                unsigned const virt_prio, Affinity::Location,
		                addr_t const utcb);

		/**
		 * Destructor
		 */
		~Platform_thread();

		/**
		 * Return information about current fault
		 */
		Kernel::Thread_fault fault_info() { return _kobj->fault(); }

		/**
		 * Join a protection domain
		 *
		 * \param pd             platform pd object pointer
		 * \param main_thread    wether thread is the first in protection domain
		 * \param address_space  corresponding Genode address space
		 *
		 * This function has no effect when called more twice for a
		 * given thread.
		 */
		void join_pd(Platform_pd *const pd, bool const main_thread,
		             Weak_ptr<Address_space> address_space);

		/**
		 * Run this thread
		 *
		 * \param ip  initial instruction pointer
		 * \param sp  initial stack pointer
		 */
		int start(void * const ip, void * const sp);

		void restart();

		/**
		 * Pause this thread
		 */
		void pause() { Kernel::pause_thread(*_kobj); }

		/**
		 * Enable/disable single stepping
		 */
		void single_step(bool) { }

		/**
		 * Resume this thread
		 */
		void resume() { Kernel::resume_thread(*_kobj); }

		/**
		 * Set CPU quota of the thread to 'quota'
		 */
		void quota(size_t const quota);

		/**
		 * Get raw thread state
		 */
		Thread_state state();

		/**
		 * Override raw thread state
		 */
		void state(Thread_state s);

		/**
		 * Return unique identification of this thread as faulter
		 */
		unsigned long pager_object_badge() { return (unsigned long)this; }

		/**
		 * Set the executing CPU for this thread
		 *
		 * \param location  targeted location in affinity space
		 */
		void affinity(Affinity::Location const & location);

		/**
		 * Get the executing CPU for this thread
		 */
		Affinity::Location affinity() const;

		/**
		 * Return the address space to which the thread is bound
		 */
		Weak_ptr<Address_space>& address_space();

		/**
		 * Return execution time consumed by the thread
		 */
		Trace::Execution_time execution_time() const
		{
			Genode::uint64_t execution_time =
				const_cast<Platform_thread *>(this)->_kobj->execution_time();
			return { execution_time, 0, _quota, _priority }; }


		/***************
		 ** Accessors **
		 ***************/

		Label label() const { return _label; };

		void pager(Pager_object &pager);

		Pager_object &pager();

		Platform_pd * pd() const { return _pd; }

		Ram_dataspace_capability utcb() const { return _utcb; }
};

#endif /* _CORE__PLATFORM_THREAD_H_ */
