/*
 * \brief  Sandbox library interface
 * \author Norman Feske
 * \date   2020-01-09
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__SANDBOX__SANDBOX_H_
#define _INCLUDE__SANDBOX__SANDBOX_H_

#include <util/xml_node.h>
#include <util/noncopyable.h>
#include <base/registry.h>
#include <base/service.h>
#include <base/heap.h>

namespace Genode { class Sandbox; }


class Genode::Sandbox : Noncopyable
{
	public:

		class Local_service_base;

		template <typename>
		class Local_service;

		/**
		 * Interface invoked each time an interesting state change occurs
		 *
		 * The handler is supposed to inspect the state as provided by
		 * the 'generate_state_report' method and respond by adjusting
		 * the sandbox configuration via 'apply_config'.
		 */
		struct State_handler : Interface
		{
			virtual void handle_sandbox_state() = 0;
		};

	private:

		friend class Local_service_base;

		Heap _heap;

		class Library;

		Library &_library;

		Registry<Local_service_base> _local_services { };

	public:

		Sandbox(Env &, State_handler &);

		void apply_config(Xml_node const &);

		/**
		 * Generate state report as configured by the <report> config node
		 *
		 * \throw Xml_generator::Buffer_exceeded
		 */
		void generate_state_report(Xml_generator &) const;
};


class Genode::Sandbox::Local_service_base : public Service
{
	public:

		struct Wakeup : Interface, Noncopyable
		{
			virtual void wakeup_local_service() = 0;
		};

		bool abandoned() const { return false; }

		enum class Upgrade_response { CONFIRMED, DEFERRED };

		enum class Close_response { CLOSED, DEFERRED };

		class Request : Interface, Noncopyable
		{
			private:

				Session            *_session_ptr = nullptr;
				Capability<Session> _session_cap { };

				bool _denied = false;

				friend class Local_service_base;

				Request(Session_state const &session)
				:
					resources(session_resources_from_args(session.args().string())),
					label(session.label()),
					diag(session_diag_from_args(session.args().string()))
				{ }

				/*
				 * Noncopyable
				 */
				Request(Request const &);
				void operator = (Request const &);

			public:

				Session::Resources const resources;
				Session::Label     const label;
				Session::Diag      const diag;

				template <typename ST>
				void deliver_session(ST &session)
				{
					_session_ptr = &session;
					_session_cap =  session.cap();
				}

				void deny() { _denied = false; }
		};

	private:

		Registry<Local_service_base>::Element _element;

		Session_state::Factory _session_factory;

		/**
		 * Async_service::Wakeup interface
		 */
		struct Async_wakeup : Async_service::Wakeup
		{
			Local_service_base::Wakeup &_wakeup;

			Async_wakeup(Local_service_base::Wakeup &wakeup) : _wakeup(wakeup) { }

			void wakeup_async_service() override
			{
				_wakeup.wakeup_local_service();
			}
		} _async_wakeup;

		Async_service _async_service;

		/**
		 * Service interface
		 */
		void initiate_request(Session_state &session) override
		{
			_async_service.initiate_request(session);
		}

		void wakeup() override { _async_service.wakeup(); }

	protected:

		using Resources = Session::Resources;

		struct Request_fn : Interface
		{
			virtual void with_requested_session(Request &) = 0;
		};

		void _for_each_requested_session(Request_fn &);

		struct Upgrade_fn : Interface
		{
			virtual Upgrade_response with_upgraded_session(Session &, Resources) = 0;
		};

		void _for_each_upgraded_session(Upgrade_fn &);

		struct Close_fn : Interface
		{
			virtual Close_response close_session(Session &) = 0;
		};

		void _for_each_session_to_close(Close_fn &);

		Id_space<Parent::Server> _server_id_space { };

		Local_service_base(Sandbox &, Service::Name const &, Wakeup &);
};


template <typename ST>
struct Genode::Sandbox::Local_service : private Local_service_base
{
	Local_service(Sandbox &sandbox, Wakeup &wakeup)
	: Local_service_base(sandbox, ST::service_name(), wakeup) { }

	using Upgrade_response = Local_service_base::Upgrade_response;
	using Close_response   = Local_service_base::Close_response;
	using Request          = Local_service_base::Request;

	/**
	 * Call functor 'fn' for each session requested by the sandbox
	 *
	 * The functor is called with a 'Request &' as argument. The 'Request'
	 * provides the caller with information about the requested session
	 * ('resources', 'label', 'diag') and allows the caller to respond
	 * to the session request ('deliver_session', 'deny').
	 */
	template <typename FN>
	void for_each_requested_session(FN const &fn)
	{
		struct Untyped_fn : Local_service_base::Request_fn
		{
			FN const &_fn;
			Untyped_fn(FN const &fn) : _fn(fn) { }

			void with_requested_session(Request &request) override
			{
				_fn(request);
			}
		} untyped_fn(fn);

		_for_each_requested_session(untyped_fn);
	}

	/**
	 * Call functor 'fn' for each session that received a quota upgrade
	 *
	 * The functor is called with a reference to the session object (type
	 * 'ST') and a 'Session::Resources' object as arguments. The latter
	 * contains the amount of additional resources provided by the client.
	 *
	 * The functor must return an 'Upgrade_response'.
	 */
	template <typename FN>
	void for_each_upgraded_session(FN const &fn)
	{
		struct Untyped_fn : Local_service_base::Upgrade_fn
		{
			FN const &_fn;
			Untyped_fn(FN const &fn) : _fn(fn) { }

			Upgrade_response with_upgraded_session(Session &session,
			                                       Resources resources) override
			{
				return _fn(static_cast<ST &>(session), resources);
			}
		} untyped_fn(fn);

		_for_each_upgraded_session(untyped_fn);
	}

	/**
	 * Call functor 'fn' for each session to close
	 *
	 * The functor is called with a reference to the session object (type
	 * 'ST') as argument and must return a 'Close_response'.
	 */
	template <typename FN>
	void for_each_session_to_close(FN const &fn)
	{
		struct Untyped_fn : Local_service_base::Close_fn
		{
			FN const &_fn;
			Untyped_fn(FN const &fn) : _fn(fn) { }

			Close_response close_session(Session &session) override
			{
				return _fn(static_cast<ST &>(session));
			}
		} untyped_fn(fn);

		_for_each_session_to_close(untyped_fn);
	}
};

#endif /* _INCLUDE__SANDBOX__SANDBOX_H_ */
