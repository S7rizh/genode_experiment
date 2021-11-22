/*
 * \brief  Event-tracing support
 * \author Norman Feske
 * \date   2013-08-09
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/env.h>
#include <base/thread.h>
#include <base/trace/policy.h>
#include <dataspace/client.h>
#include <util/construct_at.h>
#include <cpu_thread/client.h>

/* local includes */
#include <base/internal/trace_control.h>

using namespace Genode;


namespace Genode { bool inhibit_tracing = true; /* cleared by '_main' */ }

static Env *_env_ptr;

namespace Genode { void init_tracing(Env &env) { _env_ptr = &env; } }

static Env &_env()
{
	if (_env_ptr)
		return *_env_ptr;

	struct Missing_call_of_init_tracing { };
	throw  Missing_call_of_init_tracing();
}


/*******************
 ** Trace::Logger **
 *******************/

bool Trace::Logger::_evaluate_control()
{
	/* check process-global and thread-specific tracing condition */
	if (inhibit_tracing || !control || control->tracing_inhibited())
		return false;

	if (control->state_changed()) {

		/* suppress tracing during initialization */
		Control::Inhibit_guard guard(*control);

		if (control->to_be_disabled()) {

			/* unload policy */
			if (policy_module) {
				_env().rm().detach(policy_module);
				policy_module = 0;
			}

			/* unmap trace buffer */
			if (buffer) {
				_env().rm().detach(buffer);
				buffer = 0;
			}

			/* inhibit generation of trace events */
			enabled = false;
			control->acknowledge_disabled();
		}

		else if (control->to_be_enabled()) {
			control->acknowledge_enabled();
			enabled = true;
		}
	}

	bool const new_policy = policy_version != control->policy_version();
	if (enabled && (new_policy || policy_module == 0)) {

		/* suppress tracing during policy change */
		Control::Inhibit_guard guard(*control);

		/* obtain policy */
		Dataspace_capability policy_ds = Cpu_thread_client(thread_cap).trace_policy();

		if (!policy_ds.valid()) {
			warning("could not obtain trace policy");
			control->error();
			enabled = false;
			return false;
		}

		try {
			max_event_size = 0;
			policy_module  = 0;

			enum {
				MAX_SIZE = 0, NO_OFFSET = 0, ANY_LOCAL_ADDR = false,
				EXECUTABLE = true
			};

			policy_module = _env().rm().attach(policy_ds, MAX_SIZE, NO_OFFSET,
			                                   ANY_LOCAL_ADDR, nullptr, EXECUTABLE);

			/* relocate function pointers of policy callback table */
			for (unsigned i = 0; i < sizeof(Trace::Policy_module)/sizeof(void *); i++) {
				((addr_t *)policy_module)[i] += (addr_t)(policy_module);
			}

			max_event_size = policy_module->max_event_size();

		} catch (...) { }

		/* obtain buffer */
		buffer = 0;
		Dataspace_capability buffer_ds = Cpu_thread_client(thread_cap).trace_buffer();

		if (!buffer_ds.valid()) {
			warning("could not obtain trace buffer");
			control->error();
			enabled = false;
			return false;
		}

		try {
			buffer = _env().rm().attach(buffer_ds);
			buffer->init(Dataspace_client(buffer_ds).size());
		} catch (...) { }

		policy_version = control->policy_version();
	}

	return enabled && policy_module;
}


__attribute__((optimize("-fno-delete-null-pointer-checks")))
void Trace::Logger::log(char const *msg, size_t len)
{
	if (!this || !_evaluate_control()) return;

	memcpy(buffer->reserve(len), msg, len);
	buffer->commit(len);
}


__attribute__((optimize("-fno-delete-null-pointer-checks")))
bool Trace::Logger::log_captured(char const *msg, size_t len)
{
	if (!this || !_evaluate_control()) return false;

	len = policy_module->log_output(buffer->reserve(len), msg, len);
	buffer->commit(len);

	return len != 0;
}


void Trace::Logger::init(Thread_capability thread, Cpu_session *cpu_session,
                         Trace::Control *attached_control)
{
	if (!attached_control)
		return;

	thread_cap = thread;
	cpu        = cpu_session;

	unsigned const index    = Cpu_thread_client(thread).trace_control_index();
	Dataspace_capability ds = cpu->trace_control();
	size_t size             = Dataspace_client(ds).size();
	if ((index + 1)*sizeof(Trace::Control) > size) {
		error("thread control index is out of range");
		return;
	}

	control = attached_control + index;
}


Trace::Logger::Logger() { }


/************
 ** Thread **
 ************/

/**
 * return logger instance for the main thread **
 */
static Trace::Logger &main_trace_logger()
{
	static Trace::Logger logger;
	return logger;
}


static Trace::Control *main_trace_control;


Trace::Logger *Thread::_logger()
{
	if (inhibit_tracing)
		return nullptr;

	Thread * const myself = Thread::myself();

	Trace::Logger &logger = myself ? myself->_trace_logger
	                               : main_trace_logger();

	/* logger is already being initialized */
	if (logger.init_pending())
		return &logger;

	/* lazily initialize trace object */
	if (!logger.initialized()) {
		logger.init_pending(true);

		Thread_capability thread_cap = myself ? myself->_thread_cap
		                                      : _env().parent().main_thread_cap();

		Cpu_session &cpu = myself ? *myself->_cpu_session : _env().cpu();

		if (!myself)
			if (!main_trace_control) {
				Dataspace_capability ds = _env().cpu().trace_control();
				if (ds.valid())
					main_trace_control = _env().rm().attach(ds);
			}

		logger.init(thread_cap, &cpu,
		            myself ? myself->_trace_control : main_trace_control);
	}

	return &logger;
}
