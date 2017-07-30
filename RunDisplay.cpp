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
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/devices/clocks/LinuxClockDriver.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include "DisplayStuff.hpp"
#include <iostream>
#include <chrono>
#include <csignal>
#include <boost/exception/diagnostic_information.hpp>

extern DisplayStuff displaystuff;

extern std::sig_atomic_t quit;

namespace displays = duds::hardware::devices::displays;

/**
 * Writes the time of day of an eclipse totality event. This avoids the need
 * to convert the time provided by NASA's shapefiles into something else
 * before output.
 */
void writeEclipseTime(std::ostream &os, int time) {
	int h, m, s;
	h = time / 3600;
	m = (time / 60) - (h * 60);
	s = time % 60;
	os << std::right << std::setw(2) << h << ':' << std::setfill('0') <<
	std::setw(2) << m << ':' << std::setw(2) << s << std::setfill(' ');
}

// maybe should make a framework for this; it'll probably get messy if user
// input is added
enum DisplayPage {
	GPS_Status,
	GPS_Longitutde,
	GPS_Latitude,
	Totality_Duration,
	Totality_Start,    // skip these after totality
	Totality_End,
	Totality_Wait,     // maybe record location during totality to a file
	Error,
	PageCycle,         // if here, cycle to first page
	Totality_Remain,  // special-ness
};
constexpr int pagetime = 8;
void runDisplay(
	const std::shared_ptr<displays::TextDisplay> &tmd,
	int testTimeOffset
) try {
	displays::TextDisplayStream tds(tmd);
	// the clock for the current time
	duds::hardware::devices::clocks::LinuxClockDriver lcd;
	// a sample of the current time
	duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample ts;
	boost::posix_time::time_duration time;
	// when to advance the current display page
	int timer = pagetime;
	// information currently displayed
	int page = 0;//Error;
	bool pagechange = true;
	bool syncdClock = false;

	while (!quit) {
		// get stuff to display
		DisplayInfo info(displaystuff.getInfo());
		// advance display
		if (!--timer) {
			bool totality = false;
			// totality logic; use only before and during totality
			if (time.total_seconds() <= (info.end + 4)) {
				// totality is now  (time is 1s in past)
				if (time.total_seconds() >= (info.start - 1)) {
					if (page != Totality_Remain) {
						page = Totality_Remain;
						pagechange = true;
					}
					// run page change logic next time through the loop
					//timer = 1;
					totality = true;
					// maybe add mid-way attention sound?
				}
				// impending start?
				else if (time.total_seconds() >= (info.start - 32)) {
					if (page != Totality_Wait) {
						page = Totality_Wait;
						pagechange = true;
					}
					// run page change logic next time through the loop
					//timer = 1;
					totality = true;
					// do something for attention
				}
			}
			// totality end is in the past (checking previous page)
			else if ((page == Totality_Remain) || ((page >= Totality_Duration - 1)
				&& (page <= Totality_Wait - 1))
			) {
				// advance to the error page
				page = Error - 1;
			}
			// do not advance page or change timer if using in-totality logic
			if (!totality) {
				++page;
				// advance past pages without good info
				if (
					// advancing to lon & lat pages, but no position fix
					(!info.goodfix &&
					((page == GPS_Longitutde) || (page == GPS_Latitude)))
				) {
					page = Totality_Duration;
				} else if (
					// advancing to second or third totality page, but not in totality
					(!info.inTotality &&
					(page >= Totality_Start) && (page <= Totality_Wait))
				) {
					// advance to the error page
					page = Error;
				}
				// advanced to the end, or to the error page without an error
				if (
					// at end of pages
					(page == PageCycle) ||
					// at error page, but no error
					((page == Error) && info.errormsg.empty())
				) {
					// back to the start
					page = GPS_Status;
				}
				// show the page for 8 seconds
				//showuntil = std::chrono::steady_clock::now() + std::chrono::seconds(8);
				timer = pagetime;
				pagechange = true;
			}
		}
		// output status: have new result?
		if (info.totchg) {
			// show new result immediately
			page = Totality_Duration;
			//showuntil = std::chrono::steady_clock::now() + std::chrono::seconds(8);
			timer = pagetime;
			pagechange = true;
		} else if (info.errchg) {
			// show new errors immediately
			page = Error;
			//showuntil = std::chrono::steady_clock::now() + std::chrono::seconds(8);
			timer = pagetime;
			pagechange = true;
		}
		// ensure quick switch to totality in & near totality
		if ((time.total_seconds() >= (info.start - 32)) &&
			(time.total_seconds() <= (info.end + 4))
		) {
			timer = 1;
		}

		// first line; limit to 7 characters on left; clock is on right
		switch (page) {
			case GPS_Status:
			case GPS_Longitutde:
			case GPS_Latitude:
				if (pagechange) {
					tds << "GPS";
				}
				break;
			case Totality_Duration:
			case Totality_Start:
			case Totality_End:
			case Totality_Wait:
				if (pagechange) {
					tds << "Tot ";
				} else {
					tds << displays::move(4, 0);
				}
				if (info.goodfix) {
					double dist = haversineEarth(info.chkloc, info.curloc);
					if (dist < 99.5) {
						tds << std::setfill(' ') << std::setw(2) << std::right <<
						std::setprecision(0) << std::fixed << dist << std::left <<
						'm';
					} else {
						tds << "++m";
					}
				}
				break;
			case Error:
				if (pagechange) {
					tds << "ERROR";
				}
				break;
			case Totality_Remain:
				if (pagechange) {
					tds << "Tot now";
				}
		}
		// get the current time
		lcd.sampleTime(ts);
		if (!syncdClock  && (ts.accuracy !=
			duds::data::unspecified<
				duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample::Quality
			>()
		)) {
			syncdClock = true;
		}
		// used for time display and computing the delay through this loop (UTC)
		time = duds::time::planetary::earth->posix(ts.value).time_of_day();
		if (testTimeOffset) {
			time += boost::posix_time::seconds(testTimeOffset);
		}
		// only output time after the clock has been sync'd once
		if (syncdClock) {
			if (pagechange) {
				tmd->clearTo(7, 0);
			} else {
				tmd->move(8, 0);
			}
			char sep;
			if (time.seconds() & 1) {
				sep = ' ';
			} else {
				sep = ':';
			}
			tds << std::right << std::setw(2) << time.hours() << std::setfill('0')
			<< sep << std::setw(2) << time.minutes()
			<< sep << std::setw(2) << time.seconds()
			<< std::left << std::setfill(' ');

			// test output
			/*
			std::cout << '\r' << std::setfill('0') <<
			std::right << std::setw(2) << time.hours()
			<< sep << std::setw(2) << time.minutes()
			<< sep << std::setw(2) << time.seconds()
			<< std::left << std::setfill(' ');
			std::cout.flush();
			*/
		} else {
			// position for second line
			tds << displays::move(0, 1);
		}

		// second line; already positioned

		switch (page) {
			case GPS_Status:
				if (info.goodfix) { // show position error each time
					tds << "Within " << info.locerr << 'm';
				} else if (pagechange) {  // show lack of fix just once
					tds << "No fix";
				}
				break;
			case GPS_Longitutde:
				if (pagechange) {
					tds << "Lon ";
				} else {
					tds << displays::move(4, 1);
				}
				tds << std::setprecision(7) << std::setw(12) <<
				std::fixed << info.curloc.lon;
				break;
			case GPS_Latitude:
				if (pagechange) {
					tds << "Lat ";
				} else {
					tds << displays::move(4, 1);
				}
				tds << std::setprecision(7) << std::setw(12) <<
				std::fixed << info.curloc.lat;
				break;
			case Totality_Duration:
				if (pagechange) {
					if (info.inTotality) {
						tds << "Duration " << (info.end - info.start) << 's';
					} else {
						tds << "Outside totality";
					}
				}
				break;
			case Totality_Start:
				if (pagechange) {
					tds << "Start" << displays::clearTo(7, 1);
					writeEclipseTime(tds, info.start);
				}
				break;
			case Totality_End:
				if (pagechange) {
					tds << "End  " << displays::move(8, 1);
					writeEclipseTime(tds, info.end);
				}
				break;
			case Totality_Wait:
				if (pagechange) {
					tds << "Wait ";
				} else {
					tds << displays::move(5, 1);
				} {
					int diff = info.start - time.total_seconds();
					int h, m, s;
					h = diff / 3600;
					m = (diff / 60) - (h * 60);
					s = diff % 60;
					tds << std::setw(2) << std::right << h << "h " <<
					std::setfill('0') << std::setw(2) << m << "m " << std::setw(2)
					<< s << std::left << std::setfill(' ') << 's';
				}
				break;
			case Error:
				if (pagechange) {
					tds << info.errormsg;
					displaystuff.decError();
				}
				break;
			case Totality_Remain:
				if (pagechange) {
					tds << "Remaining ";
				} else {
					tds << displays::move(10, 1);
				}
				tds << (info.end - time.total_seconds()) << "s  ";
		}
		// if the cursor didn't get past the end of the line when new text output
		if (((page < Totality_Duration) || pagechange) && (tmd->columnPos() != 0)) {
			assert(tmd->rowPos() == 1);
			// clear the rest of the line
			tmd->clearTo(tmd->columns() - 1, 1);
			assert(tmd->rowPos() == 0);
		} else {
			// assure cursor in the right spot
			tmd->move(0, 0);
		}
		pagechange = false;
		// compute time to begining of next second according to the time sample
		std::chrono::milliseconds delay(
			1000 - (time.total_milliseconds() % 1000)
		);
		// if short, increase delay by a second
		if (delay < std::chrono::milliseconds(16)) {
			delay += std::chrono::milliseconds(1000);
		}
		// delay for the computed time based on the steady clock, not the clock
		// used for the time sample
		std::this_thread::sleep_until(std::chrono::steady_clock::now() + delay);
	}
} catch (...) {
	std::cerr << "Program failed in display thread:\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}
