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
#include "Page.hpp"
#include <map>

class SchedulePage : public Page {
	typedef std::map< int, std::string > EventTable;
	EventTable evtbl;
	int startT, endT, shownT;
	void makeEvents(const DisplayInfo &);
public:
	SchedulePage();
	virtual SelectionResponse select(const DisplayInfo &, SelectionCause);
	virtual void show(
		const DisplayInfo &di,
		duds::hardware::devices::displays::TextDisplayStream &tds
	);
	virtual void update(
		const DisplayInfo &di,
		duds::hardware::devices::displays::TextDisplayStream &tds
	);
};
