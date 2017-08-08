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
#include "Umbra.hpp"
#include "Functions.hpp"
#include "SunPositionTable.hpp"
//#include "BuildConfig.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/exception/diagnostic_information.hpp>
#include <duds/time/planetary/Planetary.hpp>

void check(Umbra &umbra, const SunPositionTable &spt, const std::string &input) {
	std::istringstream is(input);
	double lon, lat;
	is >> lon >> lat;
	if (is.fail()) {
		std::cerr << "Failed to parse input: \"" << input << '\"' << std::endl;
	} else {
		if (umbra.check(lon, lat)) {
			int h, m, s;
			h = umbra.startTime() / 3600;
			m = (umbra.startTime() / 60) - (h * 60);
			s = umbra.startTime() % 60;
			std::cout << "Total eclipse starts at " << std::right <<
			std::setfill('0') << std::setw(2) << h << ':' <<
			std::setw(2) << m << ':' << std::setw(2) << s << " and ends at ";
			h = umbra.endTime() / 3600;
			m = (umbra.endTime() / 60) - (h * 60);
			s = umbra.endTime() % 60;
			std::cout << std::setw(2) << h << ':' << std::setw(2) << m << ':'
			<< std::setw(2) << s << " for a duration of " <<
			(umbra.endTime() - umbra.startTime()) << " seconds." << std::endl;
			#ifdef HAVE_LIBDUDS
			/*
			double a, e, ab, ae, eb, ee;
			Location l(lon, lat);
			duds::time::interstellar::SecondTime t;
			duds::time::planetary::earth->date(
				t,
				boost::gregorian::date(2017, 8, 21)
			);
			t += std::chrono::seconds(
				umbra.startTime() + (umbra.endTime() - umbra.startTime()) / 2
			);
			sunPosition(a, e, l, t);
			sunPosition(ab, eb, l, t - std::chrono::minutes(90));
			sunPosition(ae, ee, l, t + std::chrono::minutes(90));
			std::cout << "\tSun at mid-totality: azimuth " << a << "  elevation "
			<< e << "\n\tChange from -90 to +90 minutes:  azimuth " <<
			(ae - ab) << "  elevation " << (ee - eb) << std::endl;
			*/
			#endif
			SunPositionTable::Position start, mid, end;
			mid = spt.lookup(
				umbra.startTime() + (umbra.endTime() - umbra.startTime()) / 2
			);
			start = spt.lookup(umbra.startTime() - 5302);
			end = spt.lookup(umbra.endTime() + 5049);
			std::cout << "\tSun at mid-totality: azimuth " << mid.azimuth <<
			"  elevation " << mid.elevation << "\n\tChange from start to end: "
			" azimuth " << (end.azimuth - start.azimuth) << "  elevation " <<
			(end.elevation - start.elevation) << std::endl;
		} else {
			std::cout << "Total eclipse not visible." << std::endl;
		}
	}
}

int main(int argc, char *argv[])
try {
	#ifdef HAVE_LIBDUDS
	//duds::time::planetary::Earth::make();
	#endif
	SunPositionTable spt("Alt_Az_Table", 5 * 3600);
	std::string path;
	if ((argc == 2) || (argc == 4)) { // 1 or 3 arguments
		path = argv[1];
	} else {
		path = "nasashapes/umbra17_1s.shp";
	}
	Umbra umbra(path);
	if ((argc == 3) || (argc == 4)) { // 2 or 3 arguments
		std::string lonlat = argv[argc - 2];
		lonlat += std::string(" ") + argv[argc - 1];
		check(umbra, spt, lonlat);
	} else {
		std::cout << "Enter longitude (negative) followed by latitude "
		"(positive) separated by spaces.\nEnter an empty line or 'q' to quit."
		<< std::endl;
		do {
			std::string input;
			std::getline(std::cin, input);
			if (input.empty() || (input[0] == 'q')) {
				break;
			}
			check(umbra, spt, input);
		} while (true);
	}
	return 0;
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
