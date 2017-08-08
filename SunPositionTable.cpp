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
#include <fstream>
#include "SunPositionTable.hpp"
#include <boost/exception/errinfo_file_name.hpp>

SunPositionTable::SunPositionTable(const std::string &fname, int utcOffset) {
	std::ifstream tblf(fname);
	if (tblf.fail()) {
		BOOST_THROW_EXCEPTION(SunPositionFileError() <<
			boost::errinfo_file_name(fname)
		);
	}
	Position pos;
	int h, m;
	char colon;
	while (!tblf.eof() && !tblf.fail()) {
		tblf >> h >> colon >> m >> pos.elevation >> pos.azimuth;
		if (!tblf.fail()) {
			postbl[h * 3600 + m * 60 + utcOffset] = pos;
		}
	}
}

const SunPositionTable::Position &SunPositionTable::lookup(int utcTime) const {
	PositionTable::const_iterator iter = postbl.lower_bound(utcTime);
	if (iter == postbl.end()) {
		BOOST_THROW_EXCEPTION(NoSunPositionError() << SunPositionTime(utcTime));
	}
	return iter->second;
}
