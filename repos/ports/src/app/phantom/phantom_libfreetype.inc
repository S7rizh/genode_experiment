SRC_LIBFREETYPE = phantom/libfreetype/sfnt.c \
									phantom/libfreetype/ttgload.c \
									phantom/libfreetype/ftutil.c \
									phantom/libfreetype/ftoutln.c \
									phantom/libfreetype/ttsbit.c \
									phantom/libfreetype/ftbitmap.c \
									phantom/libfreetype/ttpost.c \
									phantom/libfreetype/ftstream.c \
									phantom/libfreetype/ftsystem.c \
									phantom/libfreetype/smooth.c \
									phantom/libfreetype/ttgxvar.c \
									phantom/libfreetype/ftinit.c \
									phantom/libfreetype/ftrfork.c \
									phantom/libfreetype/truetype.c \
									phantom/libfreetype/ftdebug.c \
									phantom/libfreetype/ftsmooth.c \
									phantom/libfreetype/ftobjs.c \
									phantom/libfreetype/ftgloadr.c \
									phantom/libfreetype/ftbase.c \
									phantom/libfreetype/ftdbgmem.c \
									phantom/libfreetype/autofit.c \
									phantom/libfreetype/fttrigon.c \
									phantom/libfreetype/ftnames.c \
									phantom/libfreetype/ttobjs.c \
									phantom/libfreetype/ftcalc.c \
									phantom/libfreetype/ftgrays.c \
									phantom/libfreetype/ttdriver.c \
									phantom/libfreetype/ftglyph.c \
									phantom/libfreetype/ttpload.c

# sfnt.c includes
#include "ttload.c"
#include "ttcmap.c"
#include "ttkern.c"
#include "sfobjs.c"
#include "sfdriver.c"

# autofit.c includes
#include "afangles.c"
#include "afglobal.c"
#include "afhints.c"
#include "afdummy.c"
#include "aflatin.c"
#include "afloader.c"
#include "afmodule.c"

# SRC_LIBFREETYPE = phantom/libfreetype/sfnt.c

SRC_C += $(SRC_LIBFREETYPE)