#
# \brief  Build config for Genodes core process
# \author Stefan Kalkowski
# \author Josef Söntgen
# \author Martin Stein
# \date   2014-02-25
#

# add include paths
REP_INC_DIR += src/core/board/nit6_solox

# add C++ sources
SRC_CC += platform_services.cc

NR_OF_CPUS = 1

# include less specific configuration
include $(call select_from_repositories,lib/mk/spec/cortex_a9/core-hw.inc)
