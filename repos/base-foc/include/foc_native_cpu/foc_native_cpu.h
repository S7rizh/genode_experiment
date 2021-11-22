/*
 * \brief  Fiasco.OC-specific part of the CPU session interface
 * \author Stefan Kalkowski
 * \author Norman Feske
 * \date   2011-04-14
 */

/*
 * Copyright (C) 2011-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__FOC_NATIVE_CPU__FOC_NATIVE_CPU_H_
#define _INCLUDE__FOC_NATIVE_CPU__FOC_NATIVE_CPU_H_

#include <base/rpc.h>
#include <cpu_session/cpu_session.h>
#include <foc/thread_state.h>


struct Genode::Cpu_session::Native_cpu : Interface
{
	virtual Native_capability native_cap(Thread_capability) = 0;
	virtual Foc_thread_state thread_state(Thread_capability) = 0;


	/*********************
	 ** RPC declaration **
	 *********************/

	GENODE_RPC(Rpc_native_cap, Native_capability, native_cap, Thread_capability);
	GENODE_RPC(Rpc_thread_state, Foc_thread_state, thread_state, Thread_capability);

	GENODE_RPC_INTERFACE(Rpc_native_cap, Rpc_thread_state);
};

#endif /* _INCLUDE__FOC_NATIVE_CPU__FOC_NATIVE_CPU_H_ */
