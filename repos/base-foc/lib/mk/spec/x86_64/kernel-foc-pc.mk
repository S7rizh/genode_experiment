override BOARD := pc
KERNEL_CONFIG  := $(REP_DIR)/config/x86_64.kernel

include $(REP_DIR)/lib/mk/kernel-foc.inc
