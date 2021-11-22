/*
 * \brief  Genode XML nodes plus local utilities
 * \author Martin Stein
 * \date   2016-08-19
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _XML_NODE_H_
#define _XML_NODE_H_

/* Genode includes */
#include <util/xml_node.h>
#include <base/duration.h>


namespace Genode {

	Microseconds read_sec_attr(Xml_node const  node,
	                           char     const *name,
	                           uint64_t const  default_sec);

	template <typename FUNC>
	void xml_node_with_attribute(Xml_node const  node,
	                             char     const *name,
	                             FUNC        &&  func)
	{
		if (node.has_attribute(name)) {
			func(node.attribute(name));
		}
	}
}

#endif /* _XML_NODE_H_ */
