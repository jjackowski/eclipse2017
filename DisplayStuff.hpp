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
#include <duds/general/Spinlock.hpp>
#include "Functions.hpp"

struct DisplayInfo {
	Location chkloc;  // for checked position
	Location curloc;
	std::string errormsg;
	int errcnt; // number of times error will be shown
	int locerr;
	int start;
	int end;
	union {
		std::uint8_t chgflgs;
		struct {
			int totchg : 1; // totality change; start, end, and inTotality changed
			int poschg : 1;
			int chkchg : 1;
			int errchg : 1;
		};
	};
	bool inTotality;
	bool goodfix;
	DisplayInfo() : start(86400), end(86400), chgflgs(0), inTotality(false),
	goodfix(false), chkloc(0, 0), curloc(0, 0)
	{ }
};

class DisplayStuff {  // primitive
	DisplayInfo info;
	duds::general::Spinlock block;
public:
	void setCheckLoc(const Location &l);
	void setCurrLoc(const Location &l, int le);
	void badFix();
	void updateTotality(int s, int e, bool i);
	DisplayInfo getInfo();
	void setError(const std::string msg, int cnt);
	void clearError();
	void decError();
};
