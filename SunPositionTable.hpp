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
#ifndef SUNPOSITIONTABLE_HPP
#define SUNPOSITIONTABLE_HPP

#include <map>
#include <functional>
#include <memory>
#include <boost/exception/info.hpp>

struct SunPositionError : virtual std::exception, virtual boost::exception { };
struct NoSunPositionError : SunPositionError { };
struct SunPositionFileError : SunPositionError { };
typedef boost::error_info<struct Info_SunPositionTime, int>
	SunPositionTime;

class SunPositionTable {
public:
	struct Position {
		float azimuth;
		float elevation;
	};
private:
	typedef std::map< int, Position, std::greater<int> > PositionTable;
	PositionTable postbl;
public:
	/**
	 * Creates the table from the given file.
	 * @param fname      The file to parse. It should have the numeric data
	 *                   (no headers or footers) in the format presented by
	 *                   http://aa.usno.navy.mil/data/docs/AltAz.php
	 * @param utcOffset  The offset in seconds added to the local time in the
	 *                   file to get UTC.
	 */
	SunPositionTable(const std::string &fname, int utcOffset);
	/**
	 * Lookup for the given time.
	 * @param utcTime  The time in seconds past midnight, UTC. If there is no
	 *                 exact match for the time in the table, the next earliest
	 *                 time is used.
	 */
	const Position &lookup(int utcTime) const;
};

typedef std::shared_ptr<SunPositionTable> SunPositionTableShared;

#endif        //  #ifndef SUNPOSITIONTABLE_HPP
