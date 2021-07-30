/*
* Envrionment for Phantom OS
* Based on rm_nested example and rump block device backend
*/

#include <base/component.h>
#include <rm_session/connection.h>
#include <region_map/client.h>
#include <dataspace/client.h>

#include <base/allocator_avl.h>
#include <block_session/connection.h>

// Disk backend
#include "disk_backend.h"

using namespace Genode;

enum
{
	OBJECT_SPACE_SIZE = 0x80000000,
	OBJECT_SPACE_START = 0x80000000,
	PAGE_SIZE = 4096,
};

/**
 * Region-manager fault handler resolves faults by attaching new dataspaces
 */
class Phantom::Local_fault_handler : public Genode::Entrypoint
{
private:
	Env &_env;
	Region_map &_region_map;
	Signal_handler<Local_fault_handler> _handler;
	volatile unsigned _fault_cnt{0};

	void _handle_fault()
	{
		Region_map::State state = _region_map.state();

		_fault_cnt++;

		log("region-map state is ",
			state.type == Region_map::State::READ_FAULT ? "READ_FAULT" : state.type == Region_map::State::WRITE_FAULT ? "WRITE_FAULT"
																	 : state.type == Region_map::State::EXEC_FAULT	  ? "EXEC_FAULT"
																													  : "READY",
			", pf_addr=", Hex(state.addr, Hex::PREFIX));

		log("allocate dataspace and attach it to sub region map");
		Dataspace_capability ds = _env.ram().alloc(PAGE_SIZE);
		_region_map.attach_at(ds, state.addr & ~(PAGE_SIZE - 1));

		log("returning from handle_fault");
	}

public:
	Local_fault_handler(Genode::Env &env, Region_map &region_map)
		: Entrypoint(env, sizeof(addr_t) * 2048, "local_fault_handler",
					 Affinity::Location()),
		  _env(env),
		  _region_map(region_map),
		  _handler(*this, *this, &Local_fault_handler::_handle_fault)
	{
		region_map.fault_handler(_handler);

		log("fault handler: waiting for fault signal");
	}

	void dissolve() { Entrypoint::dissolve(_handler); }

	unsigned fault_count()
	{
		asm volatile("" ::
						 : "memory");
		return _fault_cnt;
	}
};

void Phantom::test_obj_space(addr_t const addr_obj_space)
{

	// Reading from mem

	log("Reading from obj.space");

	addr_t read_addr = addr_obj_space;

	log("  read     mem                         ",
		(sizeof(void *) == 8) ? "                " : "",
		Hex_range<addr_t>(OBJECT_SPACE_START, OBJECT_SPACE_SIZE), " value=",
		Hex(*(unsigned *)(read_addr)));

	// Writing to mem

	log("Writing to obj.space");
	*((unsigned *)read_addr) = 256;

	log("    wrote    mem   ",
		Hex_range<addr_t>(OBJECT_SPACE_START, OBJECT_SPACE_SIZE), " with value=",
		Hex(*(unsigned *)read_addr));

	// Reading again

	log("Reading from obj.space");
	log("  read     mem                         ",
		(sizeof(void *) == 8) ? "                " : "",
		Hex_range<addr_t>(OBJECT_SPACE_START, OBJECT_SPACE_SIZE), " value=",
		Hex(*(unsigned *)(read_addr)));
}

struct Phantom::Main
{
	Env &_env;
	Heap _heap{_env.ram(), _env.rm()};

	Phantom::Disk_backend _disk{_env, _heap};

	Main(Env &env) : _env(env)
	{
	}
};

// Block test function

void Phantom::test_block_device(Phantom::Disk_backend &disk)
{
	char buffer[512] = {0};
	char test_word[] = "Hello, World!";
	bool success = false;

	// Write then read

	memcpy(buffer, test_word, strlen(test_word));

	log("Writing to the disk");
	success = disk.submit(Disk_backend::Operation::WRITE, true, 1024, 512, buffer);
	log("Competed write (", success, ")");

	memset(buffer, 0x0, 512);

	log("Reading from the disk");
	success = disk.submit(Disk_backend::Operation::READ, false, 1024, 512, buffer);
	log("Competed read (", success, ")");

	log("Comparing results");
	if (strcmp(buffer, test_word) == 0)
	{
		log("Single write-read test was successfully passed!");
	}
	else
	{
		log("Single write-read test was failed!");
	}

	log("Done!");
}

void Component::construct(Genode::Env &env)
{
	log("--- nested region map test ---");

	{
		/*
		 * Initialize object space region.
		 */
		Rm_connection rm(env);

		Region_map_client rm_obj_space(rm.create(OBJECT_SPACE_SIZE));
		Dataspace_client rm_obj_space_client(rm_obj_space.dataspace());

		Phantom::Local_fault_handler fault_handler(env, rm_obj_space);

		void *ptr_obj_space = env.rm().attach(rm_obj_space.dataspace(), 0, 0, true, OBJECT_SPACE_START, false, true);
		addr_t const addr_obj_space = reinterpret_cast<addr_t>(ptr_obj_space);
		log("Addr ", addr_obj_space);
		// log("Size ", rm_obj_space_client.size());
		// log(" region top        ",
		// 	Hex_range<addr_t>(addr_obj_space, rm_obj_space_client.size()));

		Phantom::test_obj_space(addr_obj_space);

		/*
		 * Setup Main object and disk backend
		 */
		try
		{

			static Phantom::Main main(env);
			Phantom::test_block_device(main._disk);
		}
		catch (Genode::Service_denied)
		{
			error("opening block session was denied!");
		}

		/*
		 * Tests
		 */
	}

	log("--- finished nested region map test ---");
}
