/*
 * \brief  RM- and pager implementations specific for base-hw and core
 * \author Martin Stein
 * \author Stefan Kalkowski
 * \date   2012-02-12
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* base-hw Core includes */
#include <pager.h>
#include <platform_pd.h>
#include <platform_thread.h>

using namespace Genode;


/**********************
 ** Pager_entrypoint **
 **********************/

void Pager_entrypoint::entry()
{
	Untyped_capability cap;

	while (1) {

		if (cap.valid()) Kernel::ack_signal(Capability_space::capid(cap));

		/* receive fault */
		if (Kernel::await_signal(Capability_space::capid(_kobj.cap()))) continue;

		Pager_object *po = *(Pager_object**)Thread::myself()->utcb()->data();
		cap = po->cap();

		if (!po) continue;

		/* fetch fault data */
		Platform_thread * const pt = (Platform_thread *)po->badge();
		if (!pt) {
			Genode::warning("failed to get platform thread of faulter");
			continue;
		}

		_fault = pt->fault_info();

		/* try to resolve fault directly via local region managers */
		if (po->pager(*this)) continue;

		/* apply mapping that was determined by the local region managers */
		{
			Locked_ptr<Address_space> locked_ptr(pt->address_space());
			if (!locked_ptr.valid()) continue;

			Hw::Address_space * as = static_cast<Hw::Address_space*>(&*locked_ptr);

			Hw::Page_flags const flags {
				.writeable  = _mapping.writeable  ? Hw::RW   : Hw::RO,
				.executable = _mapping.executable ? Hw::EXEC : Hw::NO_EXEC,
				.privileged = Hw::USER,
				.global     = Hw::NO_GLOBAL,
				.type       = _mapping.io_mem ? Hw::DEVICE : Hw::RAM,
				.cacheable  = _mapping.cached ? Genode::CACHED : Genode::UNCACHED
			};

			as->insert_translation(_mapping.dst_addr, _mapping.src_addr,
			                       1UL << _mapping.size_log2, flags);
		}

		/* let pager object go back to no-fault state */
		po->wake_up();
	}
}


void Mapping::prepare_map_operation() const { }
