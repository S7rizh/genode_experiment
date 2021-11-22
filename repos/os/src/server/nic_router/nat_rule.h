/*
 * \brief  Rule for doing NAT from one given interface to another
 * \author Martin Stein
 * \date   2016-09-13
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _NAT_RULE_H_
#define _NAT_RULE_H_

/* local includes */
#include <port_allocator.h>
#include <l3_protocol.h>
#include <avl_tree.h>

/* Genode includes */
#include <util/avl_string.h>

namespace Net {

	class Domain;
	class Domain_tree;

	class Port_allocator;
	class Nat_rule_base;
	class Nat_rule;
	class Nat_rule_tree;
}


class Net::Nat_rule : public Genode::Avl_node<Nat_rule>
{
	private:

		Domain               &_domain;
		Port_allocator_guard  _tcp_port_alloc;
		Port_allocator_guard  _udp_port_alloc;
		Port_allocator_guard  _icmp_port_alloc;

		static Domain &_find_domain(Domain_tree            &domains,
		                            Genode::Xml_node const  node);

	public:

		struct Invalid : Genode::Exception { };

		Nat_rule(Domain_tree            &domains,
		         Port_allocator         &tcp_port_alloc,
		         Port_allocator         &udp_port_alloc,
		         Port_allocator         &icmp_port_alloc,
		         Genode::Xml_node const  node,
		         bool             const  verbose);

		Nat_rule &find_by_domain(Domain &domain);

		Port_allocator_guard &port_alloc(L3_protocol const prot);


		/*********
		 ** log **
		 *********/

		void print(Genode::Output &output) const;


		/**************
		 ** Avl_node **
		 **************/

		bool higher(Nat_rule *rule);


		/***************
		 ** Accessors **
		 ***************/

		Domain               &domain()    const { return _domain; }
		Port_allocator_guard &tcp_port_alloc()  { return _tcp_port_alloc; }
		Port_allocator_guard &udp_port_alloc()  { return _udp_port_alloc; }
		Port_allocator_guard &icmp_port_alloc() { return _icmp_port_alloc; }
};


struct Net::Nat_rule_tree : Avl_tree<Nat_rule>
{
	struct No_match : Genode::Exception { };

	Nat_rule &find_by_domain(Domain &domain);
};

#endif /* _NAT_RULE_H_ */
