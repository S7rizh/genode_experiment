#ifndef PHANTOM_ENV_DISK_BACKEND_H
#define PHANTOM_ENV_DISK_BACKEND_H

#include "phantom_env.h"

#include <base/stdint.h>
#include <base/component.h>
#include <base/heap.h>
#include <block_session/connection.h>
#include <base/allocator.h>
#include <base/allocator_avl.h>
#include <base/log.h>

/**
 * Block session connection
 * 
 * Imported from repos/dde_rump/src/lib/rump/io.cc
 * 
 */

class Phantom::Disk_backend
{
private:
    // Genode::Env &_env;
    Genode::Heap &_heap;
    Genode::Allocator_avl _block_alloc;
    Block::Connection<> _session;
    Block::Session::Info _info;
    Genode::Mutex _session_mutex;

    void _sync()
    {
        using Block::Session;

        Session::Tag const tag{0};
        _session.tx()->submit_packet(Session::sync_all_packet_descriptor(_info, tag));
        _session.tx()->get_acked_packet();
    }

public:
    enum class Operation
    {
        READ,
        WRITE,
        SYNC
    };

    Disk_backend(Genode::Env &env, Genode::Heap &heap) : _heap(heap), _block_alloc(&_heap),
                                                         _session(env, &_block_alloc), _info(_session.info()),
                                                         _session_mutex()
    {
    }

    uint64_t block_count() const { return _info.block_count; }
    size_t block_size() const { return _info.block_size; }
    bool writable() const { return _info.writeable; }

    void sync()
    {
        Genode::Mutex::Guard guard(_session_mutex);
        _sync();
    }

    bool submit(Operation op, bool sync_req, int64_t offset, size_t length, void *data)
    {
        using namespace Block;

        Genode::Mutex::Guard guard(_session_mutex);

        Block::Packet_descriptor::Opcode opcode;

        switch (op)
        {
        case Operation::READ:
            opcode = Block::Packet_descriptor::READ;
            break;
        case Operation::WRITE:
            opcode = Block::Packet_descriptor::WRITE;
        default:
            return false;
            break;
        }

        /* allocate packet */
        try
        {
            Block::Packet_descriptor packet(_session.alloc_packet(length),
                                            opcode, offset / _info.block_size,
                                            length / _info.block_size);

            /* out packet -> copy data */
            if (opcode == Block::Packet_descriptor::WRITE)
                Genode::memcpy(_session.tx()->packet_content(packet), data, length);

            _session.tx()->submit_packet(packet);
        }
        catch (Block::Session::Tx::Source::Packet_alloc_failed)
        {
            Genode::error("I/O back end: Packet allocation failed!");
            return false;
        }

        /* wait and process result */
        Block::Packet_descriptor packet = _session.tx()->get_acked_packet();

        /* in packet */
        if (opcode == Block::Packet_descriptor::READ)
            Genode::memcpy(data, _session.tx()->packet_content(packet), length);

        bool succeeded = packet.succeeded();
        _session.tx()->release_packet(packet);

        /* sync request */
        if (sync_req)
        {
            _sync();
        }

        return succeeded;
    }
};

extern "C"
{
}

#endif