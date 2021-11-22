/*
 * \brief  Board driver
 * \author Stefan Kalkowski
 * \author Martin Stein
 * \date   2014-02-25
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__SPEC__WAND_QUAD__BOARD_H_
#define _CORE__SPEC__WAND_QUAD__BOARD_H_

/* base-hw internal includes */
#include <hw/spec/arm/gicv2.h>
#include <hw/spec/arm/wand_quad_board.h>

/* base-hw Core includes */
#include <spec/arm/cortex_a9_private_timer.h>
#include <spec/cortex_a9/cpu.h>

namespace Board {

	using namespace Hw::Wand_quad_board;

	using L2_cache = Hw::Pl310;

	class Global_interrupt_controller { };
	class Pic : public Hw::Gicv2 { public: Pic(Global_interrupt_controller &) { } };

	L2_cache & l2_cache();

	enum {
		CORTEX_A9_PRIVATE_TIMER_CLK = 500000000, /* timer clk runs half the CPU freq */
		CORTEX_A9_PRIVATE_TIMER_DIV = 100,
	};
}

#endif /* _CORE__SPEC__WAND_QUAD__BOARD_H_ */
