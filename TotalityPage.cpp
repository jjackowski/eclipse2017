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
#include "TotalityPage.hpp"

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

Page::SelectionResponse TotalityPage::select(
	const DisplayInfo &di,
	SelectionCause sc
) {
	if ((sc == SelectUser) || (di.now < di.start)) {
		return SelectPage;
	}
	return SkipPage;
}

void TotalityPage::showdist(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	double d = haversineEarth(di.chkloc, di.curloc);
	if (d != dist) {
		dist = d;
		if (dist < 999.5) {
			tds << std::setw(3) << std::right << std::setprecision(0) <<
			std::fixed << dist << std::left << 'm';
		} else if (dist < 99499.0) {
			tds << std::setw(2) << std::right << std::setprecision(0) <<
			std::fixed << (dist / 1000.0) << std::left << "km";
		} else {
			tds << "+++m";
		}
	}
}

void TotalityPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.inTotality) {
		Hms time(di.start);
		tds << "Start" << clearTo(11, 1);
		time.writeTime(tds);
		tds << "End" << clearTo(11, 2);
		time.set(di.end);
		time.writeTime(tds);
		tds << "Duration " << (di.end - di.start) << "s\nTotal ";
		dist = -1.0;
		if (di.goodfix) {
			showdist(di, tds);
		}
	} else {
		tds << "Outside totality\n\n\nTotal";
	}
}

void TotalityPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.totchg) {
		show(di, tds);
	} else {
		if (di.goodfix) {
			tds << move(6, 0);
			showdist(di, tds);
		}
		tds << move(12, 0);
	}
}
