/*
 * \brief  Helper utilities for Broadwell GPU mutliplexer
 * \author Josef Soentgen
 * \data   2017-03-15
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _UTILS_H_
#define _UTILS_H_

/* Genode includes */
#include <util/interface.h>
#include <base/ram_allocator.h>

namespace Utils {

	using size_t   = Genode::size_t;
	using uint32_t = Genode::uint32_t;
	using Ram      = Genode::Ram_dataspace_capability;

	/*
	 * Backend allocator interface
	 */
	struct Backend_alloc : Genode::Interface
	{
		virtual Ram alloc(Genode::size_t) = 0;
		virtual Ram alloc(Genode::size_t, Genode::Cap_quota_guard &, Genode::Ram_quota_guard&) = 0;
		virtual void free(Ram) = 0;
	};

	template <unsigned int ELEMENTS> class Address_map;

	inline void clflush(volatile void *addr)
	{
		asm volatile("clflush %0" : "+m" (*(volatile char *)addr));
	}

}

template <unsigned int ELEMENTS>
class Utils::Address_map
{
	public:

		struct Element
		{
			Ram      ds_cap { };
			void    *va     { nullptr };
			void    *pa     { nullptr };
			uint32_t index  { 0 };

			Element() { }

			Element(uint32_t index, Ram ds_cap, void *va)
			:
				ds_cap(ds_cap),
				va(va),
				pa((void *)Genode::Dataspace_client(ds_cap).phys_addr()),
				index(index)
			{ }

			Element(Element const &other)
			:
				ds_cap(other.ds_cap), va(other.va), pa(other.pa),
				index(other.index)
			{ }

			Element &operator = (Element const &other)
			{
				ds_cap = other.ds_cap;
				va     = other.va;
				pa     = other.pa;
				index  = other.index;
				return *this;
			}

			bool valid() { return va && pa; }
		};

	private:

		Element _map[ELEMENTS];

	public:

		Address_map()
		{
			Genode::memset(&_map, 0, sizeof(_map));
		}

		~Address_map()
		{
			for (uint32_t i = 0; i < ELEMENTS; i++) {
				if (!_map[i].valid()) { continue; }

				Genode::error("Address_map entry ", Genode::Hex(i), " ",
				              "still valid (", _map[i].va, "/", _map[i].pa, ")");
			}
		}

		bool add(Ram ds_cap, void *va)
		{
			for (uint32_t i = 0; i < ELEMENTS; i++) {
				if (_map[i].valid()) { continue; }

				_map[i] = Element(i, ds_cap, va);
				return true;
			}
			return false;
		}

		Ram remove(void *va)
		{
			Ram cap;

			for (uint32_t i = 0; i < ELEMENTS; i++) {
				if (_map[i].va != va) { continue; }

				cap = _map[i].ds_cap;
				_map[i] = Element();
				break;
			}

			return cap;
		}

		Element *phys_addr(void *va)
		{
			for (uint32_t i = 0; i < ELEMENTS; i++) {
				if (_map[i].va == va) { return &_map[i]; }
			}
			return nullptr;
		}

		Element *virt_addr(void *pa)
		{
			for (uint32_t i = 0; i < ELEMENTS; i++) {
				if (_map[i].pa == pa) { return &_map[i]; }
			}
			return nullptr;
		}
};

#endif /* _UTILS_H_ */
