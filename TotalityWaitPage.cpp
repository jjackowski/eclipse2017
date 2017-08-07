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
#include "TotalityWaitPage.hpp"

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

Page::SelectionResponse TotalityWaitPage::select(
	const DisplayInfo &di,
	SelectionCause sc
) {
	if (di.inTotality && ((sc == SelectUser) || (di.now < di.start))) {
		return SelectPage;
	}
	return SkipPage;
}

void TotalityWaitPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	Hms time(di.start);
	tds << "Start" << clearTo(11, 1);
	time.writeTime(tds);
	if (di.start > di.now) {
		tds << "Wait     "; 
		time.set(di.start - di.now);
		time.writeDuration(tds);
	} else {
		tds << "Over     "; 
		time.set(di.now - di.end);
		time.writeDuration(tds);
	}
	tds << startLine << "Duration ";
	time.set(di.end - di.start);
	time.writeDuration(tds);
	tds << "\nTotal ";
	dist = -1.0;
	if (di.goodfix) {
		showdist(di, tds);
	}
}

void TotalityWaitPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.totchg) {
		show(di, tds);
	} else {
		tds << move(9, 2);
		Hms time;
		if (di.start > di.now) {
			time.set(di.start - di.now);
		} else {
			time.set(di.now - di.end);
		}
		time.writeDuration(tds);
		tds << startLine;
		if (di.goodfix) {
			tds << move(6, 0);
			showdist(di, tds);
		}
		tds << move(12, 0);
	}
}
