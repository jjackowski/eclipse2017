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
#include "InTotalityPage.hpp"
#include <sstream>

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

Page::SelectionResponse InTotalityPage::select(const DisplayInfo &di, SelectionCause) {
	if (di.inTotality && (
		(di.now >= (di.start - 64)) &&
		(di.now <= (di.end + 4))
	)) {
		return SelectPage;
	}
	return SkipPage;
}

void InTotalityPage::rightJustified(
	int t,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	std::ostringstream remain;
	Hms time(t);
	time.writeDuration(remain);
	std::string remstr = remain.str();
	tds << clearTo(19 - remstr.length(), tds.display()->rowPos()) << remstr;
}

void InTotalityPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.start <= di.now) {
		Hms time(di.end);
		tds << "End" << clearTo(11, 1);
		time.writeTime(tds);
		tds << "Remaining";
		rightJustified(di.end - di.now, tds);
		tds << "       Look!\nTotality";
	} else {
		Hms time(di.start);
		tds << "Start" << clearTo(11, 1);
		time.writeTime(tds);
		tds << "Duration";
		rightJustified(di.end - di.start, tds);
		tds << "Wait";
		rightJustified(di.start - di.now, tds);
		tds << "Totality";
	}
}

void InTotalityPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	// lazy solution
	show(di, tds);
	//tds << move(12, 0);
}
