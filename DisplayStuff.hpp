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
#ifndef DISPLAYSTUFF_HPP
#define DISPLAYSTUFF_HPP

#include <duds/general/Spinlock.hpp>
#include "Functions.hpp"

struct DisplayInfo {
	Location chkloc;  // for checked position
	Location curloc;
	std::string errormsg;
	std::string noticemsg;
	int now;    // seconds since midnight UTC
	int errtime;
	int notetime;
	int errcnt; // number of times error will be shown
	int locerr;
	int start;
	int end;
	int sats;
	union {
		std::uint8_t chgflgs;
		struct {
			int totchg : 1; // totality change; start, end, and inTotality changed
			int poschg : 1;
			int chkchg : 1;
			int errchg : 1;
			int notchg : 1;
		};
	};
	bool inTotality;
	bool goodfix;
	DisplayInfo() : start(86400), end(86400), chgflgs(0), inTotality(false),
	goodfix(false), chkloc(0, 0), curloc(0, 0)
	{ }
};

class DisplayStuff {
	DisplayInfo info;
	duds::general::Spinlock block;
public:
	void setTime(int time);
	void setCheckLoc(const Location &l);
	void setCurrLoc(const Location &l, int le, int su);
	void badFix();
	void updateTotality(int s, int e, bool i);
	void setError(const std::string msg, int cnt);
	void setNotice(const std::string msg);
	void clearError();
	void decError();
	DisplayInfo getInfo();
};

#endif        //  #ifndef DISPLAYSTUFF_HPP
