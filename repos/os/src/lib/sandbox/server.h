/*
 * \brief  Server role of init, forwarding session requests to children
 * \author Norman Feske
 * \date   2017-03-07
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _LIB__SANDBOX__SERVER_H_
#define _LIB__SANDBOX__SERVER_H_

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <os/buffered_xml.h>

/* local includes */
#include <types.h>
#include <service.h>
#include <state_reporter.h>
#include <config_model.h>

namespace Sandbox { class Server; }


class Sandbox::Server : Session_state::Ready_callback,
                        Session_state::Closed_callback,
                        public Service_model::Factory
{
	private:

		struct Route
		{
			Routed_service &service;
			Session_label   label;
		};

		Env &_env;

		Allocator &_alloc;

		/*
		 * ID space of requests originating from the parent
		 */
		Id_space<Parent::Server> _server_id_space { };

		/*
		 * ID space of requests issued to the children
		 */
		Id_space<Parent::Client> _client_id_space { };

		/**
		 * Exception type
		 */
		class Service_not_present : Exception { };

		/**
		 * Meta data of service provided to our parent
		 */
		struct Service;

		Registry<Service> _services { };

		/**
		 * Services provided by our children
		 */
		Registry<Routed_service> &_child_services;

		Report_update_trigger &_report_update_trigger;

		Constructible<Attached_rom_dataspace>  _session_requests        { };
		Constructible<Signal_handler<Server> > _session_request_handler { };

		/**
		 * \throw Service_denied
		 */
		Route _resolve_session_request(Genode::Service::Name const &,
		                               Session_label const &);

		void _handle_create_session_request (Xml_node, Parent::Client::Id);
		void _handle_upgrade_session_request(Xml_node, Parent::Client::Id);
		void _handle_close_session_request  (Xml_node, Parent::Client::Id);

		void _handle_session_request(Xml_node);
		void _handle_session_requests();

		void _close_session(Session_state &, Parent::Session_response response);

		/**
		 * Session_state::Closed_callback interface
		 */
		void session_closed(Session_state &) override;

		/**
		 * Session_state::Ready_callback interface
		 */
		void session_ready(Session_state &) override;

	public:

		/**
		 * Constructor
		 *
		 * \param alloc  allocator used for buffering XML config data and
		 *               for allocating per-service meta data
		 */
		Server(Env &env, Allocator &alloc, Registry<Routed_service> &services,
		       Report_update_trigger &report_update_trigger)
		:
			_env(env), _alloc(alloc), _child_services(services),
			_report_update_trigger(report_update_trigger)
		{ }

		void apply_updated_policy();

		/**
		 * Service_model::Factory
		 */
		Service_model &create_service(Xml_node const &) override;

		/**
		 * Service_model::Factory
		 */
		void destroy_service(Service_model &) override;
};

#endif /* _LIB__SANDBOX__SERVER_H_ */
