/*
 * \brief  Utility to convert key names into their corresponding key codes
 * \author Norman Feske
 * \date   2017-02-01
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _EVENT_FILTER__KEY_CODE_BY_NAME_H_
#define _EVENT_FILTER__KEY_CODE_BY_NAME_H_

/* Genode includes */
#include <base/exception.h>
#include <input/keycodes.h>

namespace Event_filter {

	struct Unknown_key : Genode::Exception { };

	typedef Genode::String<20> Key_name;

	/*
	 * \throw Unknown_key
	 */
	Input::Keycode key_code_by_name(Key_name const &name)
	{
		for (unsigned i = 0; i < Input::KEY_MAX; i++) {
			Input::Keycode const code = Input::Keycode(i);
			if (name == Input::key_name(code))
				return code;
		}
		throw Unknown_key();
	}

	/*
	 * \throw Unknown_key
	 */
	template<typename FN>
	void for_each_key_with_name(Key_name const &name, FN const &fn)
	{
		bool found = false;
		for (unsigned i = 0; i < Input::KEY_MAX; i++) {
			Input::Keycode const code = Input::Keycode(i);
			if (name == Input::key_name(code)) {
				fn(code);
				found = true;
			}
		}
		if (!found)
			throw Unknown_key();
	}
}

#endif /* _EVENT_FILTER__KEY_CODE_BY_NAME_H_ */
