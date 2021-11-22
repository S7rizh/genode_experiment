/*
 * \brief  PPGTT translation table allocator
 * \author Josef Soentgen
 * \data   2017-03-15
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _PPGTT_ALLOCATOR_H_
#define _PPGTT_ALLOCATOR_H_

/* local includes */
#include <types.h>
#include <utils.h>


namespace Igd {

	class Ppgtt_allocator;
}


class Igd::Ppgtt_allocator : public Genode::Translation_table_allocator
{
	private:

		Genode::Region_map      &_rm;
		Utils::Backend_alloc    &_backend;

		enum { ELEMENTS = 256, };
		Utils::Address_map<ELEMENTS> _map { };

		Genode::Cap_quota_guard &_caps_guard;
		Genode::Ram_quota_guard &_ram_guard;

	public:

		Ppgtt_allocator(Genode::Region_map      &rm,
		                Utils::Backend_alloc    &backend,
		                Genode::Cap_quota_guard &caps_guard,
		                Genode::Ram_quota_guard &ram_guard)
		:
			_rm         { rm },
			_backend    { backend },
			_caps_guard { caps_guard },
			_ram_guard  { ram_guard }
		{ }

		/*************************
		 ** Allocator interface **
		 *************************/

		bool alloc(size_t size, void **out_addr) override
		{
			Genode::Ram_dataspace_capability ds =
				_backend.alloc(size, _caps_guard, _ram_guard);

			*out_addr = _rm.attach(ds);
			return _map.add(ds, *out_addr);
		}

		void free(void *addr, size_t) override
		{
			if (addr == nullptr) { return; }

			Genode::Ram_dataspace_capability cap = _map.remove(addr);
			if (!cap.valid()) {
				Genode::error("could not lookup capability for addr: ", addr);
				return;
			}

			_rm.detach(addr);
			_backend.free(cap);
		}

		bool   need_size_for_free() const override { return false; }
		size_t overhead(size_t)     const override { return 0; }

		/*******************************************
		 ** Translation_table_allocator interface **
		 *******************************************/

		void *phys_addr(void *va) override
		{
			if (va == nullptr) { return nullptr; }
			typename Utils::Address_map<ELEMENTS>::Element *e = _map.phys_addr(va);
			return e ? (void*)e->pa : nullptr;
		}

		void *virt_addr(void *pa) override
		{
			if (pa == nullptr) { return nullptr; }
			typename Utils::Address_map<ELEMENTS>::Element *e = _map.virt_addr(pa);
			return e ? (void*)e->va : nullptr;
		}
};

#endif /* _PPGTT_ALLOCATOR_H_ */
