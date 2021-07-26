#ifndef PHANTOM_ENV_H
#define PHANTOM_ENV_H

#include <base/component.h>

namespace Phantom
{
    using namespace Genode;

    class Local_fault_handler;
    class Disk_backend;

    struct Main;

    void test_obj_space(addr_t const addr_obj_space);
    void test_block_device(Disk_backend &disk);
}

#endif