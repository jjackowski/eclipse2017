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
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/general/IntegerBiDirIterator.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include "DisplayStuff.hpp"
#include "RunDisplay.hpp"
#include "Umbra.hpp"
#include <iostream>
#include <chrono>
#include <future>
#include <sstream>
#include <csignal>
#include <libgpsmm.h>
#include <boost/exception/diagnostic_information.hpp>

namespace displays = duds::hardware::devices::displays;

DisplayStuff displaystuff;

std::sig_atomic_t quit = 0;

void signalHandler(int) {
	quit = 1;
}

void check(Umbra &umbra, const Location &loc)
try {
	bool res = umbra.check(loc.lon, loc.lat);
	displaystuff.updateTotality(umbra.startTime(), umbra.endTime(), res);
} catch (...) {
	std::cerr << "Program failed in umbra check thread:\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(int argc, char *argv[])
try {
	// ----- umbra -----
	std::string path;
	if ((argc > 1) && (argv[1][0] != '!')) {
		path = argv[1];
	} else {
		path = "nasashapes/umbra17_1s.shp";
	}
	Umbra umbra(path); // errors here are fatal
	// ----- time & leap seconds -----
	if ((argc > 2) && (argv[2][0] != '!')) {
		path = argv[2];
	} else {
		path = "/usr/share/zoneinfo-leaps/UTC";
	}
	try {
		duds::time::planetary::Earth::make(path);
	} catch (...) {
		std::cerr << "Failed to read " << path << ":\n" <<
		boost::current_exception_diagnostic_information() <<
		"\nUser should verify that time is UTC. It may be offset, "
		"or may be just fine." << std::endl;
		// error message for display must be short to fit
		displaystuff.setError("No leaps; UTC?", 64);
	}
	// ----- test values -----
	double testLatOffset = 0;
	if ((argc > 3) && (argv[3][0] != '!')) {
		std::istringstream is(argv[3]);
		is >> testLatOffset;
		if (is.fail()) {
			std::cerr << "Failed to parse testing latitude offset value: " <<
			argv[3] << std::endl;
			return 1;
		}
	}
	int testTimeOffset = 0;  // in seconds
	if ((argc > 4) && (argv[4][0] != '!')) {
		std::istringstream is(argv[4]);
		is >> testTimeOffset;
		if (is.fail()) {
			std::cerr << "Failed to parse testing time offset value: " <<
			argv[4] << std::endl;
			return 1;
		}
	}
	// ----- signal handler -----
	std::signal(SIGINT, &signalHandler);
	std::signal(SIGTERM, &signalHandler);
	// ----- display -----
	//                       LCD pins:  4  5   6   7  RS   E
	std::vector<unsigned int> gpios = { 5, 6, 19, 26, 20, 21 };
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		std::make_shared<duds::hardware::interface::linux::SysFsPort>(
			gpios, 0
		);
	// select pin
	std::unique_ptr<duds::hardware::interface::DigitalPinAccess> selacc =
		port->access(5); // gpio 21
	std::shared_ptr<duds::hardware::interface::ChipPinSelectManager> selmgr =
		std::make_shared<duds::hardware::interface::ChipPinSelectManager>(
			selacc
		);
	duds::hardware::interface::ChipSelect lcdsel(selmgr, 1);
	// set for LCD data
	gpios.clear();
	gpios.insert(gpios.begin(), uintIterator(0), uintIterator(5));
	duds::hardware::interface::DigitalPinSet lcdset(port, gpios);
	// LCD driver
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			lcdset, lcdsel, 16, 2
		);
	tmd->initialize();

	// start LCD output thread
	std::thread displayThread(&runDisplay, tmd, testTimeOffset);

	// ----- GPS -----
	// attempt to connect to GPSD
	std::unique_ptr<gpsmm> gps(new gpsmm("localhost", DEFAULT_GPSD_PORT));
	// failed?
	if (!gps->stream(WATCH_ENABLE|WATCH_JSON)) {
		// fatal error on the first attempt
		std::cerr << "Failed to contact gpsd." << std::endl;
		return 1;
	}
	// distant initial location helps ensure an early totality check
	Location prev(0.0, 0.0), curr;
	// claim that the last totality check was 2 minutes ago gets past
	// minimum time check
	auto lastCheck = std::chrono::system_clock::now() - std::chrono::minutes(2);
	std::future<void> eclipseCalc;

	// until signal requests termination
	while (!quit) {
		// up to 5 second delay
		if (!gps->waiting(5000000)) {
			continue;
		}
		gps_data_t *gpsInfo = gps->read();
		// failed?
		if (!gpsInfo) {
			// attempt to re-establish contact with gpsd
			gps.reset();
			do {
				displaystuff.setError("Lost gpsd connec", 8);
				gps = std::unique_ptr<gpsmm>(
					new gpsmm("localhost", DEFAULT_GPSD_PORT)
				);
				if (!gps->stream(WATCH_ENABLE|WATCH_JSON)) {
					gps.reset();
					std::this_thread::sleep_for(std::chrono::seconds(4));
				}
			} while (!gps);
			displaystuff.clearError();
		} else if (gpsInfo->set & LATLON_SET) {
			if (gpsInfo->fix.mode != MODE_3D) {
				displaystuff.badFix();
			} else {
				curr.lon = gpsInfo->fix.longitude;
				curr.lat = gpsInfo->fix.latitude + testLatOffset;
				displaystuff.setCurrLoc(
					curr ,
					(int)std::max(gpsInfo->fix.epy, gpsInfo->fix.epx)
				);
				/** @todo  Do not check for totality after totality. */
				auto now = std::chrono::system_clock::now();
				auto diff = now - lastCheck;
				// do not recompute too often
				if (diff > std::chrono::seconds(96)) {
					double dist = haversineEarth(prev, curr);
					// if the distance has changed by more than 32m, or
					// the distance has changed by at least 5m and half the
					// distance minus (minutes since last recompute) is
					// negative, then . . .
					if ((dist > 32.0) || ((dist >= 5.0) &&
						((16.0 - dist * 0.5 - std::chrono::duration_cast<std::chrono::minutes>(diff).count()) < 0)
					)) {
						lastCheck = now;
						prev = curr;
						// start computing total eclipse length
						eclipseCalc = std::async(
							std::launch::async,
							&check,
							std::ref(umbra),
							std::ref(prev)
						);
						displaystuff.setCheckLoc(prev);
						/*
						std::cout << "Starting check " <<
						std::chrono::duration_cast<std::chrono::seconds>(diff).count()
						<< "s after last, dist = " << dist << std::endl;
						*/
					}
				}
			}
		} else if (~gpsInfo->status & STATUS_FIX) {
			displaystuff.badFix();
		}
	}
	// wait for threads to end
	eclipseCalc.get();
	displayThread.join();
	std::cout << std::endl;
	return 0;
} catch (...) {
	std::cerr << "Program failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}