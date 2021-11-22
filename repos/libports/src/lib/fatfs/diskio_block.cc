/*
 * \brief   Low level disk I/O module using a Block session
 * \author  Christian Prochaska
 * \date    2011-05-30
 *
 * See doc/en/appnote.html in the FatFS source.
 */

/*
 * Copyright (C) 2011-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <block_session/connection.h>
#include <base/allocator_avl.h>
#include <base/log.h>

/* Genode block backend */
#include <fatfs/block.h>


namespace Fatfs {

extern "C" {
/* fatfs includes */
#include <fatfs/diskio.h>
}

	using namespace Genode;

	struct Drive;
	struct Platform
	{
		Genode::Env &env;
		Genode::Allocator &alloc;
		Genode::Allocator_avl tx_alloc { &alloc };

		enum { MAX_DEV_NUM = 8 };

		/* XXX: could make a tree... */
		Drive* drives[MAX_DEV_NUM];

		Platform(Genode::Env &env, Genode::Allocator &alloc)
		: env(env), alloc(alloc)
		{
			for (int i = 0; i < MAX_DEV_NUM; ++i)
				drives[i] = nullptr;
		}
	};

	static Platform *_platform;

	void block_init(Genode::Env &env, Genode::Allocator &alloc)
	{
		static Platform platform { env, alloc };
		_platform = &platform;
	}

	struct Drive : private Block::Connection<>
	{
		Info const info = Block::Connection<>::info();

		using Block::Connection<>::tx;
		using Block::Connection<>::alloc_packet;

		void sync()
		{
			/*
			 * We don't need to distinguish tags because there can only be one
			 * outstanding request.
			 */
			Block::Session::Tag const tag { 0 };
			tx()->submit_packet(sync_all_packet_descriptor(info, tag));
			tx()->get_acked_packet();
		}

		Drive(Platform &platform, char const *label)
		: Block::Connection<>(platform.env, &platform.tx_alloc, 128*1024, label)
		{ }
	};
}


using namespace Fatfs;


extern "C" Fatfs::DSTATUS disk_initialize (BYTE drv)
{
	if (drv >= Platform::MAX_DEV_NUM) {
		Genode::error("only ", (int)Platform::MAX_DEV_NUM," supported");
		return STA_NODISK;
	}

	if (_platform->drives[drv]) {
		destroy(_platform->alloc, _platform->drives[drv]);
		_platform->drives[drv] = nullptr;
	}

	try {
		String<2> label(drv);
		_platform->drives[drv] = new (_platform->alloc) Drive(*_platform, label.string());
	} catch(Service_denied) {
		Genode::error("could not open block connection for drive ", drv);
		return STA_NODISK;
	}

	Drive &drive = *_platform->drives[drv];

	/* check for write-capability */
	if (!drive.info.writeable)
		return STA_PROTECT;

	return 0;
}


extern "C" DSTATUS disk_status (BYTE drv)
{
	if (_platform->drives[drv]) {
		if (_platform->drives[drv]->info.writeable)
			return 0;
		return STA_PROTECT;
	}

	return STA_NOINIT;
}


extern "C" DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
	if (!_platform->drives[pdrv])
		return RES_NOTRDY;

	Drive &drive = *_platform->drives[pdrv];

	Genode::size_t const op_len = drive.info.block_size*count;

	/* allocate packet-descriptor for reading */
	Block::Packet_descriptor p(drive.alloc_packet(op_len),
	                           Block::Packet_descriptor::READ, sector, count);
	drive.tx()->submit_packet(p);
	p = drive.tx()->get_acked_packet();

	DRESULT res;
	if (p.succeeded() && p.size() >= op_len) {
		Genode::memcpy(buff, drive.tx()->packet_content(p), op_len);
		res = RES_OK;
	} else {
		Genode::error(__func__, " failed at sector ", sector, ", count ", count);
		res = RES_ERROR;
	}

	drive.tx()->release_packet(p);
	return res;
}


#if _READONLY == 0
extern "C" DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
	if (!_platform->drives[pdrv])
		return RES_NOTRDY;

	Drive &drive = *_platform->drives[pdrv];

	Genode::size_t const op_len = drive.info.block_size*count;

	/* allocate packet-descriptor for writing */
	Block::Packet_descriptor p(drive.alloc_packet(op_len),
	                           Block::Packet_descriptor::WRITE, sector, count);

	Genode::memcpy(drive.tx()->packet_content(p), buff, op_len);

	drive.tx()->submit_packet(p);
	p = drive.tx()->get_acked_packet();

	DRESULT res;
	if (p.succeeded()) {
		res = RES_OK;
	} else {
		Genode::error(__func__, " failed at sector ", sector, ", count ", count);
		res = RES_ERROR;
	}

	drive.tx()->release_packet(p);
	return res;
}
#endif /* _READONLY */


extern "C" DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
	if (!_platform->drives[pdrv])
		return RES_NOTRDY;

	Drive &drive = *_platform->drives[pdrv];

	switch (cmd) {
	case CTRL_SYNC:
		drive.sync();
		return RES_OK;

	case GET_SECTOR_COUNT:
		*((DWORD*)buff) = drive.info.block_count;
		return RES_OK;

	case GET_SECTOR_SIZE:
		*((WORD*)buff) = drive.info.block_size;
		return RES_OK;

	case GET_BLOCK_SIZE:
		*((DWORD*)buff) = 1;
		return RES_OK;

	default:
		return RES_PARERR;
	}
}
