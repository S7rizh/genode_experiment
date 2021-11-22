/*
 * \brief  Client-side of the Linux-specific CPU session interface
 * \author Norman Feske
 * \date   2012-08-15
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__LINUX_NATIVE_CPU__CLIENT_H_
#define _INCLUDE__LINUX_NATIVE_CPU__CLIENT_H_

#include <linux_native_cpu/linux_native_cpu.h>
#include <base/rpc_client.h>

namespace Genode { struct Linux_native_cpu_client; }


struct Genode::Linux_native_cpu_client : Rpc_client<Cpu_session::Native_cpu>
{
	explicit Linux_native_cpu_client(Capability<Cpu_session::Native_cpu> cap)
	: Rpc_client<Cpu_session::Native_cpu>(cap) { }

	void thread_id(Thread_capability thread, int pid, int tid) override {
		call<Rpc_thread_id>(thread, pid, tid); }
};

#endif /* _INCLUDE__LINUX_NATIVE_CPU__CLIENT_H_ */
