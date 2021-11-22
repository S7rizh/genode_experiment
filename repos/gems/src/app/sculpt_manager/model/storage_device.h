/*
 * \brief  Common representation of all storage devices
 * \author Norman Feske
 * \date   2018-05-02
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _MODEL__STORAGE_DEVICE_H_
#define _MODEL__STORAGE_DEVICE_H_

#include "types.h"
#include "partition.h"
#include "capacity.h"
#include "xml.h"

namespace Sculpt { struct Storage_device; };


struct Sculpt::Storage_device
{
	enum State {
		UNKNOWN,  /* partition information not yet known */
		USED,     /* part_block is running and has reported partition info */
		RELEASED, /* partition info is known but part_block is not running */
		FAILED    /* driver failed to access the device */
	};

	Allocator &_alloc;

	typedef String<32> Label;

	Label const label;

	Capacity capacity; /* non-const because USB storage devices need to update it */

	State state { UNKNOWN };

	bool whole_device = false;

	Reconstructible<Partition> whole_device_partition {
		Partition::Args::whole_device(capacity) };

	Partitions partitions { };

	Attached_rom_dataspace _partitions_rom;

	unsigned _part_block_version = 0;

	/**
	 * Trigger the rediscovery of the device, e.g., after partitioning of after
	 * formatting the whole device.
	 */
	void rediscover()
	{
		state = UNKNOWN;
		_part_block_version++;

		Partition_update_policy policy(_alloc);
		partitions.update_from_xml(policy, Xml_node("<partitions/>"));
	}

	void process_part_block_report()
	{
		_partitions_rom.update();

		Xml_node const report = _partitions_rom.xml();
		if (!report.has_type("partitions"))
			return;

		whole_device = (report.attribute_value("type", String<16>()) == "disk");

		Partition_update_policy policy(_alloc);
		partitions.update_from_xml(policy, report);

		/*
		 * Import whole-device partition information.
		 *
		 * Ignore reports that come in while the device is in use. Otherwise,
		 * the reconstruction of 'whole_device_partition' would wrongly reset
		 * the partition state such as the 'file_system.inspected' flag.
		 */
		if (!whole_device_partition.constructed() || whole_device_partition->idle()) {
			whole_device_partition.construct(Partition::Args::whole_device(capacity));
			report.for_each_sub_node("partition", [&] (Xml_node partition) {
				if (partition.attribute_value("number", Partition::Number()) == "0")
					whole_device_partition.construct(Partition::Args::from_xml(partition)); });
		}

		/* finish initial discovery phase */
		if (state == UNKNOWN)
			state = RELEASED;
	}

	/**
	 * Constructor
	 *
	 * \param label  label of block device
	 * \param sigh   signal handler to be notified on partition-info updates
	 */
	Storage_device(Env &env, Allocator &alloc, Label const &label,
	               Capacity capacity, Signal_context_capability sigh)
	:
		_alloc(alloc), label(label), capacity(capacity),
		_partitions_rom(env, String<80>("report -> runtime/", label, ".part_block/partitions").string())
	{
		_partitions_rom.sigh(sigh);
		process_part_block_report();
	}

	~Storage_device()
	{
		/* release partition info */
		rediscover();
	}

	bool part_block_needed_for_discovery() const
	{
		return state == UNKNOWN;
	}

	bool part_block_needed_for_access() const
	{
		bool needed_for_access = false;
		partitions.for_each([&] (Partition const &partition) {
			needed_for_access |= partition.check_in_progress;
			needed_for_access |= partition.format_in_progress;
			needed_for_access |= partition.file_system.inspected;
			needed_for_access |= partition.fs_resize_in_progress;
		});

		if (whole_device_partition->format_in_progress
		 || whole_device_partition->check_in_progress) {
			needed_for_access = false;
		}

		return needed_for_access;
	}

	/**
	 * Generate content of start node for part_block
	 *
	 * \param service_name  name of server that provides the block device, or
	 *                      if invalid, request block device from parent.
	 */
	inline void gen_part_block_start_content(Xml_generator &xml, Label const &server_name) const;

	template <typename FN>
	void for_each_partition(FN const &fn) const
	{
		fn(*whole_device_partition);
		partitions.for_each([&] (Partition const &partition) { fn(partition); });
	}

	template <typename FN>
	void for_each_partition(FN const &fn)
	{
		fn(*whole_device_partition);
		partitions.for_each([&] (Partition &partition) { fn(partition); });
	}

	bool all_partitions_idle() const
	{
		bool idle = true;
		partitions.for_each([&] (Partition const &partition) {
			idle &= partition.idle(); });
		return idle;
	}

	bool relabel_in_progress() const
	{
		bool result = false;
		partitions.for_each([&] (Partition const &partition) {
			result |= partition.relabel_in_progress(); });
		return result;
	}

	bool gpt_expand_in_progress() const
	{
		bool result = false;
		partitions.for_each([&] (Partition const &partition) {
			result |= partition.gpt_expand_in_progress; });
		return result;
	}

	bool fs_resize_in_progress() const
	{
		bool result = false;
		partitions.for_each([&] (Partition const &partition) {
			result |= partition.fs_resize_in_progress; });
		return result;
	}

	bool expand_in_progress() const
	{
		return gpt_expand_in_progress() || fs_resize_in_progress();
	}

	bool discovery_in_progress() const { return state == UNKNOWN; }

	Start_name part_block_start_name() const { return Start_name(label, ".part_block"); }
	Start_name relabel_start_name()    const { return Start_name(label, ".relabel"); }
	Start_name expand_start_name()     const { return Start_name(label, ".expand");  }
};


void Sculpt::Storage_device::gen_part_block_start_content(Xml_generator &xml,
                                                          Label const &server_name) const
{
	xml.attribute("version", _part_block_version);

	gen_common_start_content(xml, Label(label, ".part_block"),
	                         Cap_quota{100}, Ram_quota{8*1024*1024},
	                         Priority::STORAGE);

	gen_named_node(xml, "binary", "part_block");

	xml.node("heartbeat", [&] () { });

	xml.node("config", [&] () {
		xml.node("report", [&] () { xml.attribute("partitions", "yes"); });

		for (unsigned i = 1; i < 10; i++) {
			xml.node("policy", [&] () {
				xml.attribute("label",     i);
				xml.attribute("partition", i);
				xml.attribute("writeable", "yes");
			});
		}
	});

	gen_provides<Block::Session>(xml);

	xml.node("route", [&] () {

		gen_service_node<Block::Session>(xml, [&] () {
			if (server_name.valid())
				gen_named_node(xml, "child", server_name);
			else
				xml.node("parent", [&] () {
					xml.attribute("label", label); }); });

		gen_parent_rom_route(xml, "part_block");
		gen_parent_rom_route(xml, "ld.lib.so");
		gen_parent_route<Cpu_session> (xml);
		gen_parent_route<Pd_session>  (xml);
		gen_parent_route<Log_session> (xml);

		gen_service_node<Report::Session>(xml, [&] () {
			xml.attribute("label", "partitions");
			xml.node("parent", [&] () { }); });
	});
}

#endif /* _MODEL__STORAGE_DEVICE_H_ */
