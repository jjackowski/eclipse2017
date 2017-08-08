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
#include "SunAzimuthPage.hpp"

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

SunAzimuthPage::SunAzimuthPage(const SunPositionTableShared &s) :
spt(s) { }

Page::SelectionResponse SunAzimuthPage::select(const DisplayInfo &, SelectionCause) {
	return SelectPage;
}

void SunAzimuthPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	SunPositionTable::Position n, b, e, m;
	try {
		n = spt->lookup(di.now);
		b = spt->lookup(di.start - 5302);
		e = spt->lookup(di.end + 5049);
		m = spt->lookup(di.start + (di.end - di.start) / 2);
	} catch (...) {
		tds << "Lookup failure\n\n\nSun azimuth";
		return;
	}
	tds << "Beg " << std::setprecision(1) << std::setw(5) << std::right <<
	b.azimuth << "  Mid " << std::setprecision(1) << std::setw(5) <<
	std::right << m.azimuth << "End " << std::setprecision(1) << std::setw(5) <<
	std::right << e.azimuth << "  Trv " << std::setprecision(1) <<
	std::setw(5) << std::right << (e.azimuth - b.azimuth) << "Now " <<
	std::setprecision(1) << std::setw(5) << std::right << n.azimuth <<
	"\nSun azimuth";
}
/*
01234567890123456789
Beg 123.4  Mid 123.4
End 123.4  Trv 123.4
Now 123.4
*/

void SunAzimuthPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	// update values at the begining of a minute
	if ((di.now % 60) == 0) {
		SunPositionTable::Position n;
		try {
			n = spt->lookup(di.now);
		} catch (...) {
			tds << "Lookup failure\n\n\nSun azimuth";
			return;
		}
		tds << move(4, 3) << std::setprecision(1) << std::setw(5) <<
		std::right << n.azimuth << ' ';
	}
	tds << move(12, 0);
}
