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
#include "SchedulePage.hpp"
#include <sstream>

#include <iostream>

using duds::hardware::devices::displays::clearTo;
using duds::hardware::devices::displays::move;
using duds::hardware::devices::displays::startLine;

SchedulePage::SchedulePage() : startT(86400), endT(86400), shownT(86400) { }

Page::SelectionResponse SchedulePage::select(
	const DisplayInfo &di,
	SelectionCause sc
) {
	if ((di.inTotality || (sc == SelectUser)) && (di.now < (di.end + 5149))) {
		return SelectPage;
	}
	return SkipPage;
}

void SchedulePage::makeEvents(const DisplayInfo &di) {
	std::ostringstream oss;
	evtbl.clear();
	double t = double(di.start - 5302);
	// event names limited to 11 chars
	evtbl[(int)t] = "Start pic";
	int cnt = 1; // pic 0 is start
	for (t += 5302.0/8.0; cnt < 8; ++cnt, t += 5302.0/8.0) {
		oss << "Part pic " << cnt;
		evtbl[(int)t] = oss.str();
		oss.str(std::string());
	}
	evtbl[di.start] = "Totality";
	for (t = (double)di.end + 5049.0/8.0; cnt < 15; ++cnt, t += 5049.0/8.0) {
		oss << "Part pic " << cnt;
		evtbl[(int)t] = oss.str();
		oss.str(std::string());
	}
	evtbl[di.end + 5049] = "End pic";
	endT = di.end;
	startT = di.start;
	
	// test code
	std::cout << "Start time: " << (di.start - 5302);
	EventTable::iterator iter = evtbl.begin();
	for (; iter != evtbl.end(); ++iter) {
		std::cout << "\n\t";
		Hms time(iter->first);
		time.writeTime(std::cout);
		std::cout << " - " << iter->second;
	}
	std::cout << "\nEnd time: " << (di.end + 5049) << std::endl;
}

void SchedulePage::show(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (!di.inTotality) {
		tds << "Unknown\n\n\nSchedule";
	} else {
		if ((di.start != startT) || (di.end != endT)) {
			makeEvents(di);
		}
		EventTable::const_iterator iter = evtbl.lower_bound(di.now);
		if (iter == evtbl.end()) {
			tds << "All done.\n\n\nSchedule";
			shownT = 86400;
		} else {
			shownT = iter->first;
			tds << std::left << std::setw(11) << iter->second << ' ';
			Hms time(iter->first);
			time.writeTime(tds);
			tds << "      in ";
			time.set(iter->first - di.now);
			time.writeDuration(tds);
			++iter;
			if (iter != evtbl.end()) {
				tds << startLine << std::left << std::setw(11) <<
				iter->second << ' ';
				time.set(iter->first);
				time.writeTime(tds);
				tds << "Schedule";
			} else {
				tds << "\nAll done.\nSchedule";
			}
		}
	}
}
/*
01234567890123456789
Schedule
Event name  HH:MM:SS
      in HHh MMm SSs
Event name  HH:MM:SS
*/

void SchedulePage::update(
	const DisplayInfo &di,
	duds::hardware::devices::displays::TextDisplayStream &tds
) {
	if (di.now > (shownT + 10)) {
		show(di, tds);
	} else {
		if (di.inTotality && (shownT != 86400)) {
			tds << move(9, 2);
			Hms time(shownT - di.now);
			time.writeDuration(tds);
		}
		tds << move(12, 0);
	}
}
