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
#include "NoticePage.hpp"
#include <sstream>

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

//NoticePage::NoticePage(const std::string &n) : name(n) { }

Page::SelectionResponse NoticePage::select(const DisplayInfo &di, SelectionCause) {
	if (di.noticemsg.empty()) {
		return SkipPage;
	}
	return SelectPage;
}

void NoticePage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.noticemsg.empty()) {
		tds << "No notice.\n\n\nError";
	} else {
		tds << "Time issue  ";
		Hms time(di.notetime);
		time.writeTime(tds);
		std::string::const_iterator iter = di.noticemsg.cbegin();
		for (int cnt = 0; (cnt < 40) && (iter != di.noticemsg.cend()); ++cnt, ++iter) {
			tds << *iter;
		}
		tds << clearTo(19, 3);
		tds << "Notice";
	}
}

void NoticePage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	tds << move(12, 0);
}
