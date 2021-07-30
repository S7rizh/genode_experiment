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
protected:
    Genode::Env &_env;
    Genode::Heap &_heap;

    Genode::Allocator_avl _block_alloc{&_heap};
    Block::Connection<> _session{_env, &_block_alloc};
    Block::Session::Info _info{_session.info()};
    Genode::Mutex _session_mutex{};

    // Seem to be required by block server. Probably not needed in VFS block server

    void _ack_avail() {}
    void _ready_to_submit() {}
    void _timeout() {}

    Genode::Entrypoint &_ep{_env.ep()};
    Genode::Io_signal_handler<Disk_backend> _disp_ack{_env.ep(), *this, &Disk_backend::_ack_avail};
    Genode::Io_signal_handler<Disk_backend> _disp_submit{_env.ep(), *this, &Disk_backend::_ready_to_submit};
    Genode::Io_signal_handler<Disk_backend> _disp_timeout{_env.ep(), *this, &Disk_backend::_timeout};

    // Wait for a packet on EP

    bool _handle = false;

    void _handle_signal()
    {
        _handle = true;
        while (_handle)
            _ep.wait_and_dispatch_one_io_signal();
    }

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

    Disk_backend(Genode::Env &env, Genode::Heap &heap) : _env(env), _heap(heap)
    {
        // IO event handlers
        // _session.tx_channel()->sigh_ack_avail(_disp_ack);
        // _session.tx_channel()->sigh_ready_to_submit(_disp_submit);

        log("block device with block size ", _info.block_size, " block count ",
            _info.block_count, " writeable=", _info.writeable);
        log("");
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

        // while (!_session.tx()->ready_to_submit())
        //     _handle_signal();

        Block::Packet_descriptor::Opcode opcode;

        switch (op)
        {
        case Operation::READ:
            opcode = Block::Packet_descriptor::READ;
            break;
        case Operation::WRITE:
            opcode = Block::Packet_descriptor::WRITE;
            break;
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

            log("Block: cnt=", packet.block_count());
            log("       num=", packet.block_number());
            log("       off=", packet.offset());

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
        // while (!_session.tx()->ack_avail())
        // {
        // }

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