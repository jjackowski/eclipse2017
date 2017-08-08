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
#include "SunPage.hpp"

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

SunPage::SunPage(const SunPositionTableShared &s) : spt(s) { }

Page::SelectionResponse SunPage::select(const DisplayInfo &, SelectionCause) {
	return SelectPage;
}

void SunPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	SunPositionTable::Position n, s, e, m;
	try {
		n = spt->lookup(di.now);
		s = spt->lookup(di.start - 5302);
		e = spt->lookup(di.end + 5049);
		m = spt->lookup(di.start + (di.end - di.start) / 2);
	} catch (...) {
		tds << "Sun not visible\n\n\nSun now";
		return;
	}
	tds << "Az " << std::setprecision(1) << std::setw(5) << std::right <<
	n.azimuth << "   Elev " << std::setprecision(1) << std::setw(4) <<
	std::right << n.elevation << "Mid-total elev  " << std::setprecision(1) <<
	std::setw(4) << std::right << m.elevation << "Elev travel     " <<
	std::setprecision(1) << std::setw(4) << std::right <<
	(s.elevation - e.elevation) << "Sun now";
}
/*
01234567890123456789
Az 123.4   Elev 12.3
Mid-total elev  12.3
Elev travel     12.3
*/

void SunPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	// update values at the begining of a minute
	if ((di.now % 60) == 0) {
		SunPositionTable::Position n;
		try {
			n = spt->lookup(di.now);
		} catch (...) {
			tds << "Sun not visible\n\n\nSun now";
			return;
		}
		tds << move(3, 1) << std::setprecision(1) << std::setw(5) << std::right <<
		n.azimuth << move(16, 1) << std::setprecision(1) << std::setw(4) <<
		std::right << n.elevation;
	}
	tds << move(12, 0);
}
