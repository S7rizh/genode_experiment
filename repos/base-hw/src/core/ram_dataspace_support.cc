/*
 * \brief  Export RAM dataspace as shared memory object (dummy)
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

/* Genode includes */
#include <base/log.h>

/* core includes */
#include <ram_dataspace_factory.h>
#include <platform.h>
#include <map_local.h>

using namespace Genode;


void Ram_dataspace_factory::_export_ram_ds(Dataspace_component &) { }


void Ram_dataspace_factory::_revoke_ram_ds(Dataspace_component &) { }


void Ram_dataspace_factory::_clear_ds (Dataspace_component &ds)
{
	size_t page_rounded_size = (ds.size() + get_page_size() - 1) & get_page_mask();

	/* allocate range in core's virtual address space */
	void *virt_addr;
	if (!platform().region_alloc().alloc(page_rounded_size, &virt_addr)) {
		error("could not allocate virtual address range in core of size ",
		      page_rounded_size);
		return;
	}

	/* map the dataspace's physical pages to corresponding virtual addresses */
	size_t num_pages = page_rounded_size >> get_page_size_log2();
	if (!map_local(ds.phys_addr(), (addr_t)virt_addr, num_pages)) {
		error("core-local memory mapping failed");
		return;
	}

	/* dependent on the architecture, cache maintainance might be necessary */
	Cpu::clear_memory_region((addr_t)virt_addr, page_rounded_size,
	                         ds.cacheability() != CACHED);

	/* unmap dataspace from core */
	if (!unmap_local((addr_t)virt_addr, num_pages))
		error("could not unmap core-local address range at ", virt_addr);

	/* free core's virtual address space */
	platform().region_alloc().free(virt_addr, page_rounded_size);
}

