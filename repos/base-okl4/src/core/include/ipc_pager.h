/*
 * \brief  OKL4 pager support for Genode
 * \author Norman Feske
 * \date   2009-03-31
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
#include <base/ipc.h>

/* base-internal includes */
#include <base/internal/okl4.h>

/* core-local includes */
#include <mapping.h>

namespace Genode { class Ipc_pager; }


class Genode::Ipc_pager
{
	private:

		Okl4::L4_MsgTag_t   _faulter_tag { 0 }; /* fault flags                    */
		Okl4::L4_ThreadId_t _last        { 0 }; /* faulted thread                 */
		Okl4::L4_Word_t     _last_space  { 0 }; /* space of faulted thread        */
		Okl4::L4_Word_t     _fault_addr  { 0 }; /* page-fault address             */
		Okl4::L4_Word_t     _fault_ip    { 0 }; /* instruction pointer of faulter */
		Mapping             _reply_mapping { }; /* page-fault answer              */

	protected:

		/**
		 * Wait for short-message (register) IPC -- fault
		 */
		void _wait();

		/**
		 * Send short flex page and
		 * wait for next short-message (register) IPC -- fault
		 */
		void _reply_and_wait();

	public:

		/**
		 * Wait for a new fault received as short message IPC
		 */
		void wait_for_fault();

		/**
		 * Reply current fault and wait for a new one
		 *
		 * Send short flex page and wait for next short-message (register)
		 * IPC -- pagefault
		 */
		void reply_and_wait_for_fault();

		/**
		 * Request instruction pointer of current fault
		 */
		addr_t fault_ip() { return _fault_ip; }

		/**
		 * Request fault address of current fault
		 */
		addr_t fault_addr() { return _fault_addr & ~3; }

		/**
		 * Set parameters for next reply
		 */
		void set_reply_mapping(Mapping m) { _reply_mapping = m; }

		/**
		 * Set destination for next reply
		 */
		void set_reply_dst(Native_capability pager_object) {
			_last.raw = pager_object.local_name(); }

		/**
		 * Answer call without sending a flex-page mapping
		 *
		 * This function is used to acknowledge local calls from one of
		 * core's region-manager sessions.
		 */
		void acknowledge_wakeup();

		/**
		 * Returns true if the last request was send from a core thread
		 */
		bool request_from_core() const
		{
			enum { CORE_SPACE = 0 };
			return _last_space == CORE_SPACE;
		}

		/**
		 * Return badge for faulting thread
		 *
		 * Because OKL4 has no server-defined badges for fault messages, we
		 * interpret the sender ID as badge.
		 */
		unsigned long badge() const { return _last.raw; }

		/**
		 * Return true if last fault was a write fault
		 */
		bool write_fault() const { return L4_Label(_faulter_tag) & 2; }

		/**
		 * Return true if last fault was a executable fault
		 */
		bool exec_fault() const { return false; }

		/**
		 * Return true if last fault was an exception
		 */
		bool exception() const
		{
			/*
			 * A page-fault message has one of the op bits (lower 3 bits of the
			 * label) set. If those bits are zero, we got an exception message.
			 * If the label is zero, we got an IPC wakeup message from within
			 * core.
			 */
			return L4_Label(_faulter_tag) && (L4_Label(_faulter_tag) & 0xf) == 0;
		}
};

#endif /* _CORE__INCLUDE__IPC_PAGER_H_ */
