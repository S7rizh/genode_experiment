/*
 * \brief  Low-level page-fault handling
 * \author Norman Feske
 * \date   2009-10-02
 */

/*
 * Copyright (C) 2009-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__INCLUDE__IPC_PAGER_H_
#define _CORE__INCLUDE__IPC_PAGER_H_

/* Genode includes */
#include <base/cache.h>
#include <base/ipc.h>
#include <base/stdint.h>

/* core-local includes */
#include <mapping.h>

/* NOVA includes */
#include <nova/syscalls.h>

namespace Genode { class Ipc_pager; }


namespace Genode { enum { PAGE_SIZE_LOG2 = 12 }; }


static inline Nova::Rights nova_map_rights(Genode::Mapping const &mapping)
{
	return Nova::Rights(true, mapping.writeable, mapping.executable);
}


static inline Nova::Mem_crd nova_src_crd(Genode::Mapping const &mapping)
{
	return Nova::Mem_crd(mapping.src_addr >> Genode::PAGE_SIZE_LOG2,
	                     mapping.size_log2 - Genode::PAGE_SIZE_LOG2,
	                     nova_map_rights(mapping));
}


static inline Nova::Mem_crd nova_dst_crd(Genode::Mapping const &mapping)
{
	return Nova::Mem_crd(mapping.dst_addr >> Genode::PAGE_SIZE_LOG2,
	                     mapping.size_log2 - Genode::PAGE_SIZE_LOG2,
	                     nova_map_rights(mapping));
}


class Genode::Ipc_pager
{
	private:

		addr_t  _pd_dst;
		addr_t  _pd_core;
		addr_t  _fault_ip;
		addr_t  _fault_addr;
		addr_t  _sp;
		uint8_t _fault_type;
		uint8_t _syscall_res;
		uint8_t _normal_ipc;

	public:

		Ipc_pager (Nova::Utcb &, addr_t pd_dst, addr_t pd_core);

		/*
		 * Intel manual: 6.15 EXCEPTION AND INTERRUPT REFERENCE
		 *                    Interrupt 14—Page-Fault Exception (#PF)
		 */
		enum {
			ERR_I = 1 << 4,
			ERR_R = 1 << 3,
			ERR_U = 1 << 2,
			ERR_W = 1 << 1,
			ERR_P = 1 << 0,
		};

		/**
		 * Answer current page fault
		 */
		void reply_and_wait_for_fault(addr_t sm = 0UL);

		/**
		 * Request instruction pointer of current fault
		 */
		addr_t fault_ip() { return _fault_ip; }

		/**
		 * Request page-fault address of current fault
		 */
		addr_t fault_addr() { return _fault_addr; }

		/**
		 * Set page-fault reply parameters
		 */
		void set_reply_mapping(Mapping m);

		/**
		 * Return true if fault was a write fault
		 */
		bool write_fault() const { return _fault_type & ERR_W; }

		/**
		 * Return true if fault was a non-executable fault
		 */
		bool exec_fault() const {
			return _fault_type & ERR_P && _fault_type & ERR_I; }

		/**
		 * Return result of delegate syscall
		 */
		uint8_t syscall_result() const { return _syscall_res; }

		/**
		 * Return low level fault type info
		 * Intel manual: 6.15 EXCEPTION AND INTERRUPT REFERENCE
		 *                    Interrupt 14—Page-Fault Exception (#PF)
		 */
		addr_t fault_type() { return _fault_type; }

		/**
		 * Return stack pointer address valid during page-fault
		 */
		addr_t sp() { return _sp; }
};

#endif /* _CORE__INCLUDE__IPC_PAGER_H_ */
