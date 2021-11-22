/*
 * \brief  Timer session interface
 * \author Norman Feske
 * \author Markus Partheymueller
 * \date   2006-08-15
 */

/*
 * Copyright (C) 2006-2017 Genode Labs GmbH
 * Copyright (C) 2012 Intel Corporation
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__TIMER_SESSION__TIMER_SESSION_H_
#define _INCLUDE__TIMER_SESSION__TIMER_SESSION_H_

#include <base/signal.h>
#include <session/session.h>

namespace Timer {

	using Genode::uint64_t;

	struct Session;
}


struct Timer::Session : Genode::Session
{
	typedef Genode::Signal_context_capability Signal_context_capability;

	/**
	 * \noapi
	 */
	static const char *service_name() { return "Timer"; }

	enum { CAP_QUOTA = 2 };

	virtual ~Session() { }

	/**
	 * Program single timeout (relative from now in microseconds)
	 */
	virtual void trigger_once(uint64_t us) = 0;

	/**
	 * Program periodic timeout (in microseconds)
	 *
	 * The first period will be triggered after 'us' at the latest,
	 * but it might be triggered earlier as well. The 'us' value 0
	 * disables periodic timeouts.
	 */
	virtual void trigger_periodic(uint64_t us) = 0;

	/**
	 * Register timeout signal handler
	 */
	virtual void sigh(Genode::Signal_context_capability sigh) = 0;

	/**
	 * Return number of elapsed milliseconds since session creation
	 */
	virtual uint64_t elapsed_ms() const = 0;

	virtual uint64_t elapsed_us() const = 0;

	/**
	 * Client-side convenience method for sleeping the specified number
	 * of milliseconds
	 */
	virtual void msleep(uint64_t ms) = 0;

	/**
	 * Client-side convenience method for sleeping the specified number
	 * of microseconds
	 */
	virtual void usleep(uint64_t us) = 0;


	/*********************
	 ** RPC declaration **
	 *********************/

	GENODE_RPC(Rpc_trigger_once, void, trigger_once, uint64_t);
	GENODE_RPC(Rpc_trigger_periodic, void, trigger_periodic, uint64_t);
	GENODE_RPC(Rpc_sigh, void, sigh, Genode::Signal_context_capability);
	GENODE_RPC(Rpc_elapsed_ms, uint64_t, elapsed_ms);
	GENODE_RPC(Rpc_elapsed_us, uint64_t, elapsed_us);

	GENODE_RPC_INTERFACE(Rpc_trigger_once, Rpc_trigger_periodic,
	                     Rpc_sigh, Rpc_elapsed_ms, Rpc_elapsed_us);
};

#endif /* _INCLUDE__TIMER_SESSION__TIMER_SESSION_H_ */
