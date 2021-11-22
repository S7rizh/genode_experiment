TARGET   = menu_view
SRC_CC   = main.cc
LIBS     = base libc libm vfs libpng zlib blit file
INC_DIR += $(PRG_DIR)

CUSTOM_TARGET_DEPS += menu_view_styles.tar

menu_view_styles.tar:
	$(VERBOSE)cd $(PRG_DIR); tar cf $(PWD)/bin/$@ styles
