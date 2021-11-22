/*
 * \brief  Base EMAC driver for the Xilinx EMAC PS used on Zynq devices
 * \author Timo Wischer
 * \author Johannes Schlatow
 * \date   2015-03-10
 */

/*
 * Copyright (C) 2015-2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__NIC__XILINX_EMACPS_BASE_H_
#define _INCLUDE__DRIVERS__NIC__XILINX_EMACPS_BASE_H_

/* Genode includes */
#include <os/attached_mmio.h>
#include <nic_session/nic_session.h>
#include <irq_session/connection.h>
#include <timer_session/connection.h>
#include <nic/component.h>

/* NIC driver includes */
#include <drivers/nic/uplink_client_base.h>

/* local includes */
#include "system_control.h"
#include "tx_buffer_descriptor.h"
#include "rx_buffer_descriptor.h"
#include "marvell_phy.h"

namespace Genode
{
	class Cadence_gem_base
	:
		protected Genode::Attached_mmio,
		public Phyio
	{
		private:

			/**
			* Control register
			*/
			struct Control : Register<0x00, 32>
			{
				struct Local_loopback  : Bitfield<1, 1> {};
				struct Rx_en  : Bitfield<2, 1> {};
				struct Tx_en  : Bitfield<3, 1> {};
				struct Mgmt_port_en  : Bitfield<4, 1> {};
				struct Clear_statistics  : Bitfield<5, 1> {};
				struct Start_tx  : Bitfield<9, 1> {};
				struct Tx_pause  : Bitfield<11, 1> {};

				static access_t init() {
					return Mgmt_port_en::bits(1) |
						Tx_en::bits(1) |
						Rx_en::bits(1);
				}

				static access_t start_tx() {
					return (init() | Start_tx::bits(1));
				}
			};

			/**
			* Config register
			*/
			struct Config : Register<0x04, 32>
			{
				struct Speed_100  : Bitfield<0, 1> {};
				struct Full_duplex  : Bitfield<1, 1> {};
				struct Copy_all  : Bitfield<4, 1> {};
				struct No_broadcast  : Bitfield<5, 1> {};
				struct Multi_hash_en  : Bitfield<6, 1> {};
				struct Gige_en  : Bitfield<10, 1> {};
				struct Pause_en  : Bitfield<13, 1> {};
				struct Fcs_remove  : Bitfield<17, 1> {};
				struct Mdc_clk_div  : Bitfield<18, 3> {
					enum {
						DIV_32 = 0b010,
						DIV_224 = 0b111,
					};
				};
				struct Dis_cp_pause   : Bitfield<23, 1> {};
				struct Rx_chksum_en   : Bitfield<24, 1> {};
				struct Ignore_rx_fcs  : Bitfield<26, 1> {};
			};

			/**
			* Status register
			*/
			struct Status : Register<0x08, 32>
			{
				struct Phy_mgmt_idle  : Bitfield<2, 1> {};
			};

			/**
			* DMA Config register
			*/
			struct Dma_config : Register<0x10, 32>
			{
				struct Disc_when_no_ahb : Bitfield<24,1> {};
				struct Rx_pktbuf_memsz_sel  : Bitfield<8, 2> {
					enum {
						SPACE_8KB = 0x3,
					};
				};
				struct Tx_pktbuf_memsz_sel  : Bitfield<10, 1> {
					enum {
						SPACE_4KB = 0x1,
					};
				};
				struct Ahb_mem_rx_buf_size  : Bitfield<16, 8> {
					enum {
						BUFFER_1600B = 0x19,
					};
				};
				struct Csum_gen_en : Bitfield<11, 1> { };
				struct Burst_len : Bitfield<0, 5> {
					enum {
						INCR16 = 0x10,
						INCR8  = 0x08,
						INCR4  = 0x04,
						SINGLE = 0x01
					};
				};

				static access_t init()
				{
					return Ahb_mem_rx_buf_size::bits(Ahb_mem_rx_buf_size::BUFFER_1600B) |
						Rx_pktbuf_memsz_sel::bits(Rx_pktbuf_memsz_sel::SPACE_8KB) |
						Tx_pktbuf_memsz_sel::bits(Tx_pktbuf_memsz_sel::SPACE_4KB) |
						Disc_when_no_ahb::bits(1) |
						Csum_gen_en::bits(1) |
						Burst_len::bits(Burst_len::INCR16);
				}
			};

			/**
			* Tx_Status register
			*/
			struct Tx_status : Register<0x14, 32>
			{
				struct Tx_hresp_nok    : Bitfield<8, 1> {};
				struct Tx_err_underrun : Bitfield<6, 1> {};
				struct Tx_complete     : Bitfield<5, 1> {};
				struct Tx_err_bufexh   : Bitfield<4, 1> {};
				struct Tx_go           : Bitfield<3, 1> {};
			};

			/**
			* Receiving queue base addresse register
			*/
			struct Rx_qbar : Register<0x18, 32>
			{
				struct Addr : Bitfield<0, 32> {};
			};

			/**
			* Transmition queue base addresse register
			*/
			struct Tx_qbar : Register<0x1C, 32>
			{
				struct Addr : Bitfield<0, 32> {};
			};

			/**
			* Receive status register
			*/
			struct Rx_status : Register<0x20, 32>
			{
				struct Rx_hresp_nok   : Bitfield<3, 1> {};
				struct Rx_overrun     : Bitfield<2, 1> {};
				struct Frame_received : Bitfield<1, 1> {};
				struct Buffer_not_available : Bitfield<0, 1> {};
			};

			/**
			* Interrupt status register
			*/
			struct Interrupt_status : Register<0x24, 32>
			{
				struct Rx_used_read   : Bitfield<2, 1> {};
				struct Rx_complete    : Bitfield<1, 1> {};
				struct Pause_zero     : Bitfield<13,1> {};
				struct Pause_received : Bitfield<12,1> {};
				struct Rx_overrun     : Bitfield<10,1> {};
			};

			/**
			* Interrupt enable register
			*/
			struct Interrupt_enable : Register<0x28, 32>
			{
				struct Rx_used_read   : Bitfield<2, 1> {};
				struct Rx_complete    : Bitfield<1, 1> {};
				struct Pause_zero     : Bitfield<13,1> {};
				struct Pause_received : Bitfield<12,1> {};
				struct Rx_overrun     : Bitfield<10,1> {};
			};

			/**
			* Interrupt disable register
			*/
			struct Interrupt_disable : Register<0x2C, 32>
			{
				struct Rx_complete : Bitfield<1, 1> {};
			};

			/**
			* PHY maintenance register
			*/
			struct Phy_maintenance : Register<0x34, 32>
			{
				struct Clause_22 : Bitfield<30, 1> {};
				struct Operation : Bitfield<28, 2> {
					enum Type {
						READ = 0b10,
						WRITE = 0b01
					};
				};
				struct Phy_addr : Bitfield<23, 5> {};
				struct Reg_addr : Bitfield<18, 5> {};
				struct Must_10 : Bitfield<16, 2> {
					enum { INIT = 0b10 };
				};
				struct Data : Bitfield<0, 16> {};
			};

			/**
			* MAC hash register
			*/
			struct Hash_register : Register<0x80, 64>
			{
				struct Low_hash   : Bitfield<0, 32> { };
				struct High_hash   : Bitfield<32, 16> { };
			};

			/**
			* MAC Addresse
			*/
			struct Mac_addr_1 : Register<0x88, 64>
			{
				struct Low_addr   : Bitfield<0, 32> { };
				struct High_addr   : Bitfield<32, 16> { };
			};

			/**
			* Counter for the successfully transmitted frames
			*/
			struct Frames_transmitted : Register<0x108, 32>
			{
				struct Counter : Bitfield<0, 32> { };
			};

			/**
			* Counter for the transmitted pause frames
			*/
			struct Pause_transmitted : Register<0x114, 32>
			{
				struct Counter : Bitfield<0, 16> { };
			};

			/**
			* Counter for tx underrun errors
			*/
			struct Tx_underrun : Register<0x134, 32>
			{
				struct Counter : Bitfield<0, 10> { };
			};

			/**
			* Counter for deferred transmission frames
			*/
			struct Tx_deferred: Register<0x148, 32>
			{
				struct Counter : Bitfield<0, 18> { };
			};

			/**
			* Counter for the successfully received frames
			*/
			struct Frames_received : Register<0x158, 32>
			{
				struct Counter : Bitfield<0, 32> { };
			};

			/**
			* Counter for resource error statistics
			*/
			struct Rx_resource_errors : Register<0x1A0, 32>
			{
				struct Counter : Bitfield<0, 18> { };
			};

			/**
			* Counter for overrun statistics
			*/
			struct Rx_overrun_errors : Register<0x1A4, 32>
			{
				struct Counter : Bitfield<0, 10> { };
			};

			/**
			* Counter for IP checksum errors
			*/
			struct Rx_ip_chksum_errors : Register<0x1A8, 32>
			{
				struct Counter : Bitfield<0, 8> { };
			};

			/**
			* Counter for TCP checksum errors
			*/
			struct Rx_tcp_chksum_errors : Register<0x1AC, 32>
			{
				struct Counter : Bitfield<0, 8> { };
			};

			/**
			* Counter for UDP checksum errors
			*/
			struct Rx_udp_chksum_errors : Register<0x1B0, 32>
			{
				struct Counter : Bitfield<0, 8> { };
			};

			/**
			* Counter for FCS errors
			*/
			struct Rx_fcs_errors : Register<0x190, 32>
			{
				struct Counter : Bitfield<0, 10> { };
			};

			/**
			* Counter for pause frames received
			*/
			struct Pause_received : Register<0x164, 32>
			{
				struct Counter : Bitfield<0, 16> { };
			};

			/**
			 * These two structs help avoiding the following compile errors in
			 * places where the driver has to convert MAC addresses pointers
			 * to integer pointers:
			 *
			 * error: taking address of packed member of ‘Net::Mac_address’
			 *        may result in an unaligned pointer value
			 *
			 * As the MAC address type is packed and therefore has alignment 1,
			 * we have to ensure that the pointer type we convert it to also
			 * has alignment 1, i.e., that it is also packed.
			 */
			struct Packed_uint16 { uint16_t value; } __attribute__((packed));
			struct Packed_uint32 { uint32_t value; } __attribute__((packed));

			class Phy_timeout_for_idle : public Genode::Exception {};
			class Unkown_ethernet_speed : public Genode::Exception {};

			Timer::Connection       _timer;
			System_control          _sys_ctrl;
			Genode::Irq_connection  _irq;
			Marvel_phy              _phy;
			Tx_buffer_sink         &_tx_buffer_sink;
			Tx_buffer_descriptor    _tx_buffer;
			Rx_buffer_descriptor    _rx_buffer;

			void _mdio_wait()
			{
				uint32_t timeout = 200;

				/* Wait till MDIO interface is ready to accept a new transaction. */
				while (!read<Status::Phy_mgmt_idle>()) {
					if (timeout <= 0) {
						warning(__func__, ": Timeout");
						throw Phy_timeout_for_idle();
					}

					_timer.msleep(1);
					timeout--;
				}
			}

			void _phy_setup_op(const uint8_t phyaddr, const uint8_t regnum, const uint16_t data, const Phy_maintenance::Operation::Type op)
			{
				_mdio_wait();

				/* Write mgtcr and wait for completion */
				write<Phy_maintenance>(
							Phy_maintenance::Clause_22::bits(1) |
							Phy_maintenance::Operation::bits(op) |
							Phy_maintenance::Phy_addr::bits(phyaddr) |
							Phy_maintenance::Reg_addr::bits(regnum) |
							Phy_maintenance::Must_10::bits(Phy_maintenance::Must_10::INIT) |
							Phy_maintenance::Data::bits(data) );

				_mdio_wait();
			}


			/***********
			 ** Phyio **
			 ***********/

			void phy_write(const uint8_t phyaddr, const uint8_t regnum, const uint16_t data) override
			{
				_phy_setup_op(phyaddr, regnum, data, Phy_maintenance::Operation::WRITE);
			}

			void phy_read(const uint8_t phyaddr, const uint8_t regnum, uint16_t& data) override
			{
				_phy_setup_op(phyaddr, regnum, 0, Phy_maintenance::Operation::READ);

				data = read<Phy_maintenance::Data>();
			}

		public:

			/**
			 * Constructor
			 *
			 * \param  base       MMIO base address
			 */
			Cadence_gem_base(Genode::Env      &env,
			                 addr_t const      base,
			                 size_t const      size,
			                 int    const      irq,
			                 Tx_buffer_sink   &tx_buffer_sink,
			                 Rx_buffer_source &rx_buffer_source)
			:
				Genode::Attached_mmio(env, base, size),
				_timer(env),
				_sys_ctrl(env, _timer),
				_irq(env, irq),
				_phy(*this, _timer),
				_tx_buffer_sink(tx_buffer_sink),
				_tx_buffer(env, tx_buffer_sink, _timer),
				_rx_buffer(env, rx_buffer_source)
			{ }

			void transmit_packet(Packet_descriptor packet)
			{
				_tx_buffer.add_to_queue(packet);
				write<Control>(Control::start_tx());
			}

			Nic::Mac_address read_mac_address()
			{
				Nic::Mac_address mac;
				Packed_uint32 * const low_addr_pointer  = reinterpret_cast<Packed_uint32 *>(&mac.addr[0]);
				Packed_uint16 * const high_addr_pointer = reinterpret_cast<Packed_uint16 *>(&mac.addr[4]);

				low_addr_pointer->value = read<Mac_addr_1::Low_addr>();
				high_addr_pointer->value = read<Mac_addr_1::High_addr>();

				return mac;
			}

			template <typename RECEIVE_PKT,
			          typename HANDLE_ACKS>
			void handle_irq(RECEIVE_PKT  && receive_pkt,
			                HANDLE_ACKS  && handle_acks)
			{
				/* 16.3.9 Receiving Frames */
				/* read interrupt status, to detect the interrupt reason */
				const Interrupt_status::access_t status = read<Interrupt_status>();
				const Rx_status::access_t rxStatus = read<Rx_status>();
				const Tx_status::access_t txStatus = read<Tx_status>();

				if ( Interrupt_status::Rx_complete::get(status) ) {

					while (_rx_buffer.next_packet()) {

						handle_acks();
						receive_pkt(_rx_buffer.get_packet_descriptor());
					}

					/* reset receive complete interrupt */
					write<Rx_status>(Rx_status::Frame_received::bits(1));
					write<Interrupt_status>(Interrupt_status::Rx_complete::bits(1));
				}
				else {
					handle_acks();
				}

				/* handle Rx/Tx errors */
				if ( Tx_status::Tx_hresp_nok::get(txStatus)
				  || Rx_status::Rx_hresp_nok::get(rxStatus)) {

					write<Control::Tx_en>(0);
					write<Control::Rx_en>(0);

					_tx_buffer.reset(_tx_buffer_sink);
					_rx_buffer.reset();

					write<Control::Tx_en>(1);
					write<Control::Rx_en>(1);

					write<Tx_status>(Tx_status::Tx_hresp_nok::bits(1));
					write<Rx_status>(Rx_status::Rx_hresp_nok::bits(1));
					Genode::error("Rx/Tx error: resetting both");
				}

				/* handle Tx errors */
				if ( Tx_status::Tx_err_underrun::get(txStatus)
				  || Tx_status::Tx_err_bufexh::get(txStatus)) {

					write<Control::Tx_en>(0);
					_tx_buffer.reset(_tx_buffer_sink);
					write<Control::Tx_en>(1);

					Genode::error("Tx error: resetting transceiver");
				}

				/* handle Rx error */
				bool print_stats = false;
				if (Interrupt_status::Rx_overrun::get(status)) {
					write<Control::Tx_pause>(1);
					write<Interrupt_status>(Interrupt_status::Rx_overrun::bits(1));
					write<Rx_status>(Rx_status::Rx_overrun::bits(1));

					/* reset the receiver because this may lead to a deadlock */
					write<Control::Rx_en>(0);
					_rx_buffer.reset();
					write<Control::Rx_en>(1);

					print_stats = true;
					Genode::error("Rx overrun - packet buffer overflow");
				}
				if (Interrupt_status::Rx_used_read::get(status)) {
					/* tried to use buffer descriptor with used bit set */
					/* we sent a pause frame because the buffer appears to
					 * be full
					 */
					write<Control::Tx_pause>(1);
					write<Interrupt_status>(Interrupt_status::Rx_used_read::bits(1));
					write<Rx_status>(Rx_status::Buffer_not_available::bits(1));

					print_stats = true;
					Genode::error("Rx used - the Nic client is not fast enough");
				}
				if (Interrupt_status::Pause_zero::get(status)) {
					Genode::warning("Pause ended.");
					write<Interrupt_status>(Interrupt_status::Pause_zero::bits(1));
					print_stats = true;
				}
				if (Interrupt_status::Pause_received::get(status)) {
					Genode::warning("Pause frame received.");
					write<Interrupt_status>(Interrupt_status::Pause_received::bits(1));
					print_stats = true;
				}

				if (print_stats) {
					/* check, if there was lost some packages */
					const uint32_t received  = read<Frames_received>();
					const uint32_t pause_rx  = read<Pause_received::Counter>();
					const uint32_t res_err   = read<Rx_resource_errors::Counter>();
					const uint32_t overrun   = read<Rx_overrun_errors::Counter>();
					const uint32_t fcs_err   = read<Rx_fcs_errors::Counter>();
					const uint32_t ip_chk    = read<Rx_ip_chksum_errors::Counter>();
					const uint32_t udp_chk   = read<Rx_udp_chksum_errors::Counter>();
					const uint32_t tcp_chk   = read<Rx_tcp_chksum_errors::Counter>();
					const uint32_t transmit  = read<Frames_transmitted>();
					const uint32_t pause_tx  = read<Pause_transmitted::Counter>();
					const uint32_t underrun  = read<Tx_underrun::Counter>();
					const uint32_t deferred  = read<Tx_deferred::Counter>();

					Genode::warning("Received:          ", received);
					Genode::warning("  pause frames:    ", pause_rx);
					Genode::warning("  resource errors: ", res_err);
					Genode::warning("  overrun errors:  ", overrun);
					Genode::warning("  FCS errors:      ", fcs_err);
					Genode::warning("  IP chk failed:   ", ip_chk);
					Genode::warning("  UDP chk failed:  ", udp_chk);
					Genode::warning("  TCP chk failed:  ", tcp_chk);
					Genode::warning("Transmitted:       ", transmit);
					Genode::warning("  pause frames:    ", pause_tx);
					Genode::warning("  underrun:        ", underrun);
					Genode::warning("  deferred:        ", deferred);
				}

				_irq.ack_irq();
			}

			void init(Signal_context_capability irq_handler)
			{
				_irq.sigh(irq_handler);
				_irq.ack_irq();

				/* see 16.3.2 Configure the Controller */

				/* 1. Program the Network Configuration register (gem.net_cfg) */
				write<Config>(
					Config::Gige_en::bits(1) |
					Config::Speed_100::bits(1) |
					Config::Pause_en::bits(1) |
					Config::Full_duplex::bits(1) |
					Config::Multi_hash_en::bits(1) |
					Config::Mdc_clk_div::bits(Config::Mdc_clk_div::DIV_32) |
					Config::Dis_cp_pause::bits(1) |
					Config::Rx_chksum_en::bits(1) |
					Config::Fcs_remove::bits(1)
				);


				write<Rx_qbar>(_rx_buffer.phys_addr());
				write<Tx_qbar>(_tx_buffer.phys_addr());


				/* 3. Program the DMA Configuration register (gem.dma_cfg) */
				write<Dma_config>( Dma_config::init() );


				/*
				 * 4. Program the Network Control Register (gem.net_ctrl)
				 * Enable MDIO, transmitter and receiver
				 */
				write<Control>(Control::init());



				_phy.init();

				/* change emac clocks depending on phy autonegation result */
				uint32_t rclk = 0;
				uint32_t clk = 0;
				switch (_phy.eth_speed()) {
				case SPEED_1000:
					write<Config::Gige_en>(1);
					rclk = (0 << 4) | (1 << 0);
					clk = (1 << 20) | (8 << 8) | (0 << 4) | (1 << 0);
					log("Autonegotiation result: 1Gbit/s");
					break;
				case SPEED_100:
					write<Config::Gige_en>(0);
					write<Config::Speed_100>(1);
					rclk = 1 << 0;
					clk = (5 << 20) | (8 << 8) | (0 << 4) | (1 << 0);
					log("Autonegotiation result: 100Mbit/s");
					break;
				case SPEED_10:
					write<Config::Gige_en>(0);
					write<Config::Speed_100>(0);
					rclk = 1 << 0;
					/* FIXME untested */
					clk = (5 << 20) | (8 << 8) | (0 << 4) | (1 << 0);
					log("Autonegotiation result: 10Mbit/s");
					break;
				default:
					throw Unkown_ethernet_speed();
				}
				_sys_ctrl.set_clk(clk, rclk);


				/* 16.3.6 Configure Interrupts */
				write<Interrupt_enable>(Interrupt_enable::Rx_complete::bits(1) |
				                        Interrupt_enable::Rx_overrun::bits(1) |
				                        Interrupt_enable::Pause_received::bits(1) |
				                        Interrupt_enable::Pause_zero::bits(1) |
				                        Interrupt_enable::Rx_used_read::bits(1));
			}

			void deinit()
			{
				/* 16.3.1 Initialize the Controller */

				/* Disable all interrupts */
				write<Interrupt_disable>(0x7FFFEFF);

				/* Disable the receiver & transmitter */
				write<Control>(0);
				write<Control>(Control::Clear_statistics::bits(1));

				write<Tx_status>(0xFF);
				write<Rx_status>(0x0F);
				write<Phy_maintenance>(0);

				write<Rx_qbar>(0);
				write<Tx_qbar>(0);

				/* Clear the Hash registers for the mac address
				 * pointed by AddressPtr
				 */
				write<Hash_register>(0);
			}

			void write_mac_address(const Nic::Mac_address &mac)
			{
				Packed_uint32 const * const low_addr_pointer  = reinterpret_cast<Packed_uint32 const *>(&mac.addr[0]);
				Packed_uint16 const * const high_addr_pointer = reinterpret_cast<Packed_uint16 const *>(&mac.addr[4]);

				write<Mac_addr_1::Low_addr>(low_addr_pointer->value);
				write<Mac_addr_1::High_addr>(high_addr_pointer->value);
			}

			void rx_buffer_reset_pkt(Nic::Packet_descriptor pkt)
			{
				_rx_buffer.reset_descriptor(pkt);
			}

			void tx_buffer_submit_acks()
			{
				_tx_buffer.submit_acks(_tx_buffer_sink);
			}
	};

	class Nic_rx_buffer_source : public Rx_buffer_source
	{
		private:

			Nic::Session::Tx::Source &_source;

		public:

			Nic_rx_buffer_source(Nic::Session::Tx::Source &source)
			:
				_source { source }
			{ }


			/**********************
			 ** Rx_buffer_source **
			 **********************/

			Dataspace_capability dataspace() override
			{
				return _source.dataspace();
			};

			Packet_descriptor alloc_packet(size_t size) override
			{
				return _source.alloc_packet(size);
			}
	};

	class Nic_tx_buffer_sink : public Tx_buffer_sink
	{
		private:

			Nic::Session::Rx::Sink &_sink;

		public:

			Nic_tx_buffer_sink(Nic::Session::Rx::Sink &sink)
			:
				_sink { sink }
			{ }


			/********************
			 ** Tx_buffer_sink **
			 ********************/

			Dataspace_capability dataspace() override
			{
				return _sink.dataspace();
			};

			void acknowledge_packet(Packet_descriptor packet) override
			{
				_sink.acknowledge_packet(packet);
			}

			bool packet_valid(Packet_descriptor packet) override
			{
				return _sink.packet_valid(packet);
			}
	};

	/**
	 * Base driver Xilinx EMAC PS module
	 */
	class Cadence_gem : public Nic::Session_component
	{
		private:

			Nic_rx_buffer_source        _rx_buffer_source;
			Nic_tx_buffer_sink          _tx_buffer_sink;
			Cadence_gem_base            _cadence_gem;
			Signal_handler<Cadence_gem> _irq_handler;

			void _handle_acks()
			{
				while (_rx.source()->ack_avail()) {
					_cadence_gem.rx_buffer_reset_pkt(
						_rx.source()->get_acked_packet());
				}
			}

			void _handle_irq()
			{
				_cadence_gem.handle_irq(
					[&] (Nic::Packet_descriptor pkt)
				{
					if (_rx.source()->packet_valid(pkt))
						_rx.source()->submit_packet(pkt);
					else
						error(
							"invalid packet descriptor ", Hex(pkt.offset()),
							" size ", Hex(pkt.size()));
				},
					[&] ()
				{
					_handle_acks();
				});
			}

		public:

			/**
			 * Constructor
			 *
			 * \param  base       MMIO base address
			 */
			Cadence_gem(size_t const  tx_buf_size,
			            size_t const  rx_buf_size,
			            Allocator    &rx_block_md_alloc,
			            Env          &env,
			            addr_t const  base,
			            size_t const  size,
			            int    const  irq)
			:
				Session_component(tx_buf_size, rx_buf_size, Genode::UNCACHED,
				                  rx_block_md_alloc, env),
				_rx_buffer_source(*_rx.source()),
				_tx_buffer_sink(*_tx.sink()),
				_cadence_gem(env, base, size, irq, _tx_buffer_sink, _rx_buffer_source),
				_irq_handler(env.ep(), *this, &Cadence_gem::_handle_irq)
			{
				_cadence_gem.deinit();
				_cadence_gem.init(_irq_handler);
			}

			~Cadence_gem()
			{
				// TODO disable tx and rx and clean up irq registration
				_cadence_gem.deinit();
			}

			using Nic::Session_component::cap;


			bool _send()
			{
				/* first, see whether we can acknowledge any
				 * previously sent packet */
				_cadence_gem.tx_buffer_submit_acks();

				if (!_tx.sink()->ready_to_ack())
					return false;

				if (!_tx.sink()->packet_avail())
					return false;

				Packet_descriptor packet = _tx.sink()->get_packet();
				if (!packet.size()) {
					Genode::warning("Invalid tx packet");
					return true;
				}

				try {
					_cadence_gem.transmit_packet(packet);
				} catch (Tx_buffer_descriptor::Package_send_timeout) {
					Genode::warning("Package Tx timeout");
					return false;
				}

				return true;
			}


			/**************************************
			 ** Nic::Session_component interface **
			 **************************************/

			virtual Nic::Mac_address mac_address() override
			{
				return _cadence_gem.read_mac_address();
			}

			virtual bool link_state() override
			{
				/* XXX return always true for now */
				return true;
			}

			void _handle_packet_stream() override
			{
				_handle_acks();

				while (_send());
			}

			void mac_address(const Nic::Mac_address &mac)
			{
				_cadence_gem.write_mac_address(mac);
			}
	};
}


namespace Genode {

	class Uplink_client;

	class Uplink_rx_buffer_source : public Rx_buffer_source
	{
		private:

			Uplink::Session::Tx::Source &_source;

		public:

			Uplink_rx_buffer_source(Uplink::Session::Tx::Source &source)
			:
				_source { source }
			{ }


			/**********************
			 ** Rx_buffer_source **
			 **********************/

			Dataspace_capability dataspace() override
			{
				return _source.dataspace();
			};

			Packet_descriptor alloc_packet(size_t size) override
			{
				return _source.alloc_packet(size);
			}
	};

	class Uplink_tx_buffer_sink : public Tx_buffer_sink
	{
		private:

			Uplink::Session::Rx::Sink &_sink;

		public:

			Uplink_tx_buffer_sink(Uplink::Session::Rx::Sink &sink)
			:
				_sink { sink }
			{ }


			/********************
			 ** Tx_buffer_sink **
			 ********************/

			Dataspace_capability dataspace() override
			{
				return _sink.dataspace();
			};

			void acknowledge_packet(Packet_descriptor packet) override
			{
				_sink.acknowledge_packet(packet);
			}

			bool packet_valid(Packet_descriptor packet) override
			{
				return _sink.packet_valid(packet);
			}
	};
}


class Genode::Uplink_client : public Uplink_client_base
{
	private:

		Signal_handler<Uplink_client>          _irq_handler;
		Constructible<Uplink_rx_buffer_source> _rx_buffer_source { };
		Constructible<Uplink_tx_buffer_sink>   _tx_buffer_sink   { };
		Constructible<Cadence_gem_base>        _cadence_gem      { };

		bool _send()
		{
			/* first, see whether we can acknowledge any
			 * previously sent packet */
			_cadence_gem->tx_buffer_submit_acks();

			if (!_conn->rx()->ready_to_ack())
				return false;

			if (!_conn->rx()->packet_avail())
				return false;

			Packet_descriptor packet = _conn->rx()->get_packet();
			if (!packet.size()) {
				Genode::warning("Invalid tx packet");
				return true;
			}

			try {
				_cadence_gem->transmit_packet(packet);
			} catch (Tx_buffer_descriptor::Package_send_timeout) {
				Genode::warning("Package Tx timeout");
				return false;
			}

			return true;
		}

		void _handle_acks()
		{
			while (_conn->tx()->ack_avail()) {

				_cadence_gem->rx_buffer_reset_pkt(
					_conn->tx()->get_acked_packet());
			}
		}

		void _handle_irq()
		{
			if (!_conn.constructed()) {

				class No_connection { };
				throw No_connection { };
			}
			_cadence_gem->handle_irq(
				[&] (Nic::Packet_descriptor pkt)
			{
				if (_conn->tx()->packet_valid(pkt))
					_conn->tx()->submit_packet(pkt);
				else
					error(
						"invalid packet descriptor ", Hex(pkt.offset()),
						" size ", Hex(pkt.size()));
			},
				[&] ()
			{
				_handle_acks();
			});
		}


		/************************
		 ** Uplink_client_base **
		 ************************/

		void _custom_conn_rx_handle_packet_avail() override
		{
			_handle_acks();

			while (_send());
		}

		bool _custom_conn_rx_packet_avail_handler() override
		{
			return true;
		}

		Transmit_result
		_drv_transmit_pkt(const char *,
		                  size_t      ) override
		{
			class Unexpected_call { };
			throw Unexpected_call { };
		}

	public:

		Uplink_client(Env                    &env,
		              Allocator              &alloc,
		              addr_t           const  base,
		              size_t           const  size,
		              int              const  irq,
		              Net::Mac_address const  mac_addr)
		:
			Uplink_client_base { env, alloc, mac_addr },
			_irq_handler       { env.ep(), *this, &Uplink_client::_handle_irq }
		{
			_drv_handle_link_state(true);
			_rx_buffer_source.construct(*_conn->tx());
			_tx_buffer_sink.construct(*_conn->rx()),
			_cadence_gem.construct(
				env, base, size, irq, *_tx_buffer_sink, *_rx_buffer_source);

			_cadence_gem->deinit();
			_cadence_gem->init(_irq_handler);

			/* set mac address */
			_cadence_gem->write_mac_address(mac_addr);
		}
};

#endif /* _INCLUDE__DRIVERS__NIC__XILINX_EMACPS_BASE_H_ */

