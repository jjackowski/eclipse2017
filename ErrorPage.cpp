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
#include "ErrorPage.hpp"
#include <sstream>

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

extern DisplayStuff displaystuff;

Page::SelectionResponse ErrorPage::select(
	const DisplayInfo &di,
	SelectionCause sc
) {
	if ((sc == SelectUser) || (di.errcnt > 0)) {
		return SelectPage;
	} return SkipPage;
}

void ErrorPage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.errormsg.empty()) {
		tds << "No error.\n\n\nError";
	} else {
		displaystuff.decError();  // shown once
		tds << "Time occur  ";
		Hms time(di.errtime);
		time.writeTime(tds);
		std::string::const_iterator iter = di.errormsg.cbegin();
		for (int cnt = 0; (cnt < 40) && (iter != di.errormsg.cend()); ++cnt, ++iter) {
			tds << *iter;
		}
		tds << clearTo(19, 3);
		tds << "Error";
	}
}

void ErrorPage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.errchg) {
		show(di, tds);
	} else {
		tds << move(12, 0);
	}
}
