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
#include "DisplayStuff.hpp"

void DisplayStuff::setTime(int time) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.now = time;
}

void DisplayStuff::setCheckLoc(const Location &l) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.chkloc.lon = l.lon;
	info.chkloc.lat = l.lat;
	info.chkchg = true;
	info.goodfix = true;
}

void DisplayStuff::setCurrLoc(const Location &l, int le, int su) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.curloc.lon = l.lon;
	info.curloc.lat = l.lat;
	info.locerr = le;
	info.sats = su;
	info.poschg = true;
	info.goodfix = true;
}

void DisplayStuff::badFix() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.poschg = true;
	info.goodfix = false;
}

void DisplayStuff::updateTotality(int s, int e, bool i) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	// only record as change if the results are different
	if ((i != info.inTotality) || (s != info.start) || (e != info.end)) {
		info.inTotality = i;
		info.start = s;
		info.end = e;
		info.totchg = true;
	}
}

void DisplayStuff::setNotice(const std::string msg) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.notetime = info.now;
	info.noticemsg = msg;
	info.notchg = true;
}

void DisplayStuff::setError(const std::string msg, int cnt) {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.errtime = info.now;
	info.errcnt = cnt;
	if (info.errormsg != msg) {
		info.errormsg = msg;
		info.errchg = true;
	}
}

void DisplayStuff::clearError() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	info.errcnt = 0;
	info.errormsg.clear();
}

void DisplayStuff::decError() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	if (--info.errcnt <= 0) {
		info.errcnt = 0;
		//info.errormsg.clear();
	}
}

DisplayInfo DisplayStuff::getInfo() {
	std::lock_guard<duds::general::Spinlock> lock(block);
	DisplayInfo copy = info;
	info.chgflgs = 0;
	return copy;
}
