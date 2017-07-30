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
#include "Functions.hpp"
#include <cmath>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <duds/time/planetary/Planetary.hpp>

double haversineEarth(const Location &l0, const Location &l1) {
	// need radians
	double lon0r = l0.lon * M_PI / 180.0;
	double lat0r = l0.lat * M_PI / 180.0;
	double lon1r = l1.lon * M_PI / 180.0;
	double lat1r = l1.lat * M_PI / 180.0;
	// will be squared
	double latsin = std::sin((lat1r - lat0r) / 2.0); // squigly
	double lonsin = std::sin((lon1r - lon0r) / 2.0); // upside-down y
	// big equation
	return 2.0 * 6365000.0 * std::asin(  // Earth radius approx 6365000m
		std::sqrt(
			latsin * latsin +
			std::cos(lat0r) * std::cos(lat1r) * lonsin * lonsin
		)
	);
}

// q-like squiggle, or circle with near vertical line through it, is latitude
// Why does phi have two distinctly different lower-case forms, and more
// importantly, why do websites that use phi insist on using both lower-case
// forms on the same page?
void sunPosition(
	double &azimuth,
	double &elevation,
	const Location &loc,
	const duds::time::interstellar::SecondTime &time
) {
	boost::gregorian::date date = duds::time::planetary::earth->date(time);
	int ls = duds::time::planetary::earth->leaps.leapSeconds(time).count();
	// based on https://www.esrl.noaa.gov/gmd/grad/solcalc/solareqns.PDF
	// start of 2017 is at 1483228827s after T0
	// 2017 has no leap seconds or days
	double y = 2.0 * M_PI * (
		(double)(time.time_since_epoch().count() - 1483228827 - ls) /
		(double)(60 * 60 * 24 * 365) // one year
	);
	double eqtime = 229.18 * (0.000075 + 0.001868 * std::cos(y) -
		0.032077 * std::sin(y) - 0.014615 * cos(2.0 * y) -
		0.040849 * std::sin(2.0 * y)
	);
	double decl = 0.006918 - 0.399912 * std::cos(y) + 0.070257 * std::sin(y) -
		0.006758 * std::cos(2.0 * y) + 0.000907 * std::sin(2.0 * y) -
		0.002697 * std::cos(3.0 * y) + 0.00148 * std::sin(3.0 * y);
	// redone to avoid needing a timezone; it isn't like the sun will jump
	// around in the sky if you adjust your clock
	double ha = (  // will be in radians
		// seconds into the current day
		(double)(time.time_since_epoch().count() - (
			(date.day_of_year() - 1) * 60 * 60 * 24
		))
		// modify by longitude; seconds per degree
		+ loc.lon * 60.0 * 60.0 * 24.0 / 360.0 +
		// modify by the eqtime thingy
		eqtime * 60.0  // convert from minutes
	) / (60.0 * 60.0 * 24.0)  // fraction of day
	* 2.0 * M_PI - M_PI;  // angle
	double latrad = loc.lat * M_PI / 180.0;
	double coszen = std::sin(latrad) * std::sin(decl) + std::cos(latrad) *
		std::cos(decl) * std::cos(ha);
	double zenith = std::acos(coszen);
	elevation = ((M_PI / 2.0) - zenith) * 180.0 / M_PI;
	// above results *seem* ok, but next is bad
	azimuth = -(std::acos(
		-(std::sin(latrad) * coszen - std::sin(decl)) /
		(std::cos(latrad) * std::sin(zenith))
	) - M_PI * 2.0) * 180.0 / M_PI;
}
