SRC_PHANTOM_GENODE_ENV =    genode_env/main.cc \
                            genode_env/phantom_threads.cc \
                            genode_env/phantom_vmem.cc \
                            genode_env/phantom_disk.cc \
                            genode_env/phantom_sync.cc \
                            genode_env/phantom_unsorted.cc

SRC_CC += $(addprefix $(call select_from_ports,phantom)/src/app/phantom/, $(SRC_PHANTOM_GENODE_ENV))