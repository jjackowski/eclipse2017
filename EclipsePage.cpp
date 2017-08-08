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
#include "EclipsePage.hpp"
#include <sstream>

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

Page::SelectionResponse EclipsePage::select(
	const DisplayInfo &di,
	SelectionCause sc
) {
	if (di.inTotality && ((sc == SelectUser) || (di.now < (di.end + 5049)))) {
		return SelectPage;
	}
	return SkipPage;
}

void EclipsePage::timeTillEnd(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	Hms time;
	int when = di.end + 5049 - di.now;
	if (when >= 0) {
		time.set(when);
		tds << "End in ";
	} else {
		time.set(-when);
		tds << "Over ";
	}
	time.writeDuration(tds);
}

void EclipsePage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	Hms time(di.start - 5302);
	tds << "Start" << clearTo(11, 1);
	time.writeTime(tds);
	tds << "End" << clearTo(11, 2);
	time.set(di.end + 5049);
	time.writeTime(tds);
	if (di.now < (di.start - 5302)) {
		tds << "Start in ";
		time.set(di.start - 5302 - di.now);
		time.writeDuration(tds);
	} else {
		timeTillEnd(di, tds);
	}
	tds << startLine << "Eclipse";
}

void EclipsePage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.totchg) {
		show(di, tds);
	} else {
		tds << move(0, 3);
		if (di.now < (di.start - 5302)) {
			Hms time(di.start - 5302 - di.now);
			tds << "Start in ";
			time.writeDuration(tds);
		} else {
			timeTillEnd(di, tds);
		}
		tds << startLine << move(12, 0);
	}
}
