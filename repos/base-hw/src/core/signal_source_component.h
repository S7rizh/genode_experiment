/*
 * \brief  Signal-delivery mechanism
 * \author Norman Feske
 * \date   2009-08-05
 */

/*
 * Copyright (C) 2009-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__SIGNAL_SOURCE_COMPONENT_H_
#define _CORE__SIGNAL_SOURCE_COMPONENT_H_

/* Genode includes */
#include <base/object_pool.h>

/* core-local includes */
#include <object.h>
#include <kernel/signal_receiver.h>
#include <assertion.h>

namespace Genode {

	class Signal_context_component;
	class Signal_source_component;

	typedef Object_pool<Signal_context_component> Signal_context_pool;
	typedef Object_pool<Signal_source_component>  Signal_source_pool;
}


struct Genode::Signal_context_component : private Kernel_object<Kernel::Signal_context>,
                                          public Signal_context_pool::Entry
{
	friend class Object_pool<Signal_context_component>;

	using Signal_context_pool::Entry::cap;

	inline Signal_context_component(Signal_source_component &s,
	                                addr_t const imprint);

	Signal_source_component *source() { ASSERT_NEVER_CALLED; }
};


struct Genode::Signal_source_component : private Kernel_object<Kernel::Signal_receiver>,
                                         public Signal_source_pool::Entry
{
	friend class Object_pool<Signal_source_component>;
	friend class Signal_context_component;

	using Signal_source_pool::Entry::cap;

	Signal_source_component()
	:
		Kernel_object<Kernel::Signal_receiver>(CALLED_FROM_CORE),
		Signal_source_pool::Entry(Kernel_object<Kernel::Signal_receiver>::cap())
	{ }

	void submit(Signal_context_component *, unsigned long) { ASSERT_NEVER_CALLED; }

	Kernel::Signal_receiver & signal_receiver() {
		return **static_cast<Kernel_object<Kernel::Signal_receiver>*>(this); }
};


Genode::Signal_context_component::Signal_context_component(Signal_source_component &s,
                                                           addr_t const imprint)
:
	Kernel_object<Kernel::Signal_context>(CALLED_FROM_CORE,
	                                      s.signal_receiver(),
	                                      imprint),

	Signal_context_pool::Entry(Kernel_object<Kernel::Signal_context>::_cap)
{ }

#endif /* _CORE__SIGNAL_SOURCE_COMPONENT_H_ */
