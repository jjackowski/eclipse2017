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
#include "SystemPage.hpp"

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

SystemPage::SystemPage(duds::hardware::devices::instruments::INA219 &m) :
meter(m), upt("/proc/uptime"){ }

Page::SelectionResponse SystemPage::select(const DisplayInfo &, SelectionCause) {
	return SelectPage;
}

void SystemPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	volts = 0;
	watts = 0;
	tds << "Up\nCPU idle time\nBatt\nSystem    ";
	update(di, tds);
}

void SystemPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	tds << move(3, 1);
	upt.seekg(0);
	double up, idle;
	upt >> up >> idle;
	Hms updur((int)up);
	updur.writeDuration(tds);
	idle = idle / up * 100.0;
	tds << move(14, 2);
	tds << std::setw(4) << std::setprecision(1) << std::fixed << idle << '%';
	double v = meter.busVoltage().value;
	if (v != volts) {
		volts = v;
		tds << move(5, 3) << std::fixed << std::right << std::setw(6) <<
		std::setprecision(3) << volts << 'V';
	}
	double w = meter.busPower().value;
	if (w != watts) {
		watts = w;
		tds << move(13, 3) << std::fixed << std::right << std::setw(5) <<
		std::setprecision(3) << watts << 'W';
	}
	tds << move(12, 0);

}
