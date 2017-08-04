/*
 * This file is part of the Eclipse2017 project. It is subject to the GPLv3
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at
 * https://github.com/jjackowski/eclipse2017/blob/master/LICENSE.
 * No part of the Eclipse2017 project, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/instruments/INA219.hpp>

/**
 * Handles operating the LCD; intended to run on its own thread.
 */
void runDisplay(
	const std::shared_ptr<duds::hardware::devices::displays::TextDisplay> &tmd,
	duds::hardware::devices::instruments::INA219 &batmon,
	int testTimeOffset
);
