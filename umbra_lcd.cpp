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
#include <duds/hardware/interface/DigitalPin.hpp>
#include <duds/general/IntegerBiDirIterator.hpp>
#include <duds/time/planetary/Planetary.hpp>
// would have been used if backlight control was integrated into this program
//#include <duds/hardware/devices/instruments/TSL2591.hpp>
//#include <duds/hardware/interface/linux/SysPwm.hpp>
//#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <duds/hardware/devices/instruments/INA219.hpp>
#include <duds/hardware/interface/linux/DevSmbus.hpp>
#include "DisplayStuff.hpp"
#include "RunDisplay.hpp"
#include "Umbra.hpp"
#include <iostream>
#include <chrono>
#include <future>
#include <sstream>
#include <csignal>
#include <system_error>
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
		displaystuff.setError("No leaps; time may\nbe off; likely good", 64);
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
	// ----- battery monitor -----
	std::unique_ptr<duds::hardware::interface::Smbus> smbus(
		new duds::hardware::interface::linux::DevSmbus(
			"/dev/i2c-1",
			0x40,
			duds::hardware::interface::Smbus::NoPEC()
		)
	);
	duds::hardware::devices::instruments::INA219 batmon(smbus, 0.1);
	// ----- display -----
	//                       LCD pins:  4  5   6   7  RS   E  buzzer
	std::vector<unsigned int> gpios = { 5, 6, 19, 26, 20, 21, 0 };
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
			lcdset, lcdsel, 20, 4
		);
	tmd->initialize();

	duds::hardware::interface::DigitalPin buzzer(port, 6);

	RunDisplay ui(tmd, batmon, buzzer, testTimeOffset);
	// start LCD output thread
	std::thread displayThread(&RunDisplay::run, std::ref(ui));

	// ----- GPS -----
	// attempt to connect to GPSD
	std::unique_ptr<gpsmm> gps(new gpsmm("localhost", DEFAULT_GPSD_PORT));
	// failed?
	if (!gps->stream(WATCH_ENABLE|WATCH_JSON)) {
		gps.reset();
	}
	// distant initial location helps ensure an early totality check
	Location prev(0.0, 0.0), curr;
	// claim that the last totality check was 1 minute ago
	auto lastCheck = std::chrono::system_clock::now() - std::chrono::minutes(1);
	std::future<void> eclipseCalc;
	gps_data_t *gpsInfo;
	double speed = 0;  // in m/s

	// until signal requests termination
	while (!quit) {
		if (gps) {
			// up to 5 second delay
			if (!gps->waiting(5000000)) {
				continue;
			}
			gpsInfo = gps->read();
		}
		// failed?
		if (!gps || !gpsInfo) {
			speed = 0;
			// attempt to re-establish contact with gpsd
			gps.reset();
			do {
				displaystuff.setError("Lost gpsd connection", 8);
				gps = std::unique_ptr<gpsmm>(
					new gpsmm("localhost", DEFAULT_GPSD_PORT)
				);
				if (!gps->stream(WATCH_ENABLE|WATCH_JSON)) {
					gps.reset();
					std::this_thread::sleep_for(std::chrono::seconds(4));
				}
			} while (!gps);
			displaystuff.clearError();
		} else if (gps) {
			if (                                  // with no satellite signals,
				(gpsInfo->status == 0) ||         // the first two conditionals
				(gpsInfo->fix.mode != MODE_3D) || // usually are false, so 3D
				(gpsInfo->satellites_used == 0)   // fix with no satillites is
			) {                                   // possible?
				displaystuff.badFix();
			}
			// keep exponential moving average of speed for deciding when to
			// recalculate times of totality
			if (gpsInfo->set & SPEED_SET) {
				speed = 0.8 * speed + 0.2 * gpsInfo->fix.speed;
			}
			if (gpsInfo->set & LATLON_SET) {
				auto now = std::chrono::system_clock::now();
				auto diff = now - lastCheck;
				if (!displaystuff.wasGood()) {
					curr.lon = gpsInfo->fix.longitude - testLatOffset / 2;
					curr.lat = gpsInfo->fix.latitude + testLatOffset;
				} else {
					curr.lon = curr.lon * (1.0 - 0.5 / gpsInfo->fix.epx) +
						0.5 / gpsInfo->fix.epx *
						(gpsInfo->fix.longitude - testLatOffset / 2);
					curr.lat = curr.lat * (1.0 - 0.5 / gpsInfo->fix.epy) +
						0.5 / gpsInfo->fix.epy *
						(gpsInfo->fix.latitude + testLatOffset);
				}
				displaystuff.setCurrLoc(
					curr ,
					(int)std::max(gpsInfo->fix.epy, gpsInfo->fix.epx),
					gpsInfo->satellites_used
				);
				/** @todo  Do not check for totality after totality. */
				// do not recompute too often
				if (diff > std::chrono::seconds(128)) {
					double dist = haversineEarth(prev, curr);
					// if the distance has changed by more than 32m, or
					// the distance has changed by at least 5m and half the
					// distance minus (minutes since last recompute) is
					// negative, then . . .
					if ((speed < 2.5) && ((dist > 32.0) || ((dist >= 5.0) &&
						((20.0 - dist * 0.5 - std::chrono::duration_cast<std::chrono::minutes>(diff).count()) < 0)
					))) {
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
		} else {
			displaystuff.badFix();
		}
	}
	// wait for threads to end
	eclipseCalc.get();
	try {
		displayThread.join();
	} catch (const std::system_error& e) {
		// invalid_argument may occur if thread has terminated
		if (e.code() != std::errc::invalid_argument) {
			// something else
			throw;
		}
	}
	std::cout << std::endl;
	return 0;
} catch (...) {
	std::cerr << "Program failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
