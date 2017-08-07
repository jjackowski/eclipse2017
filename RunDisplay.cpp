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
#include <duds/time/planetary/Planetary.hpp>
#include "RunDisplay.hpp"
#include "Input.hpp"
#include "GpsPage.hpp"
#include "TotalityWaitPage.hpp"
#include "InTotalityPage.hpp"
#include "SystemPage.hpp"
#include "ErrorPage.hpp"
#include "NoticePage.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <csignal>
#include <boost/exception/diagnostic_information.hpp>

extern DisplayStuff displaystuff;

extern std::sig_atomic_t quit;

namespace displays = duds::hardware::devices::displays;

std::int32_t inputRotor;
std::int32_t inputButton = 0;

void rotaryInput(EventTypeCode, std::int32_t val) {
	//std::cout << "rotaryInput(), val = " << val << std::endl;
	inputRotor -= val;
}

void buttonInput(EventTypeCode, std::int32_t val) {
	std::cout << "buttonInput(), val = " << val << std::endl;
	inputButton = val;
}

void configureInput(const EvdevShared &ev) {
	EventTypeCode etc(EV_REL, REL_X);
	if (ev->hasEvent(etc)) {
		ev->inputConnect(etc, &rotaryInput);
	} else {
		etc.type = EV_KEY;
		etc.code = KEY_F24;
		if (ev->hasEvent(etc)) {
			ev->inputConnect(etc, &buttonInput);
		}
	}
}

RunDisplay::RunDisplay(
	const std::shared_ptr<displays::HD44780> &hd,
	duds::hardware::devices::instruments::INA219 &batt,
	duds::hardware::interface::DigitalPin &buz,
	int toff
) : disp(hd), batmon(batt), buzzer(buz), testTimeOffset(toff), tds(disp),
pagechange(true), syncdClock(false), page(System), timer(pagetime) { }

void RunDisplay::incPage(const DisplayInfo &di, Page::SelectionCause sc) {
	Page::SelectionResponse sr;
	do {
		if (++page >= PageCycle) {
			page = GPS_Status;
		}
		sr = pages[page]->select(di, sc);
	} while (sr == Page::SkipPage);
	changePage(page);
}

void RunDisplay::decPage(const DisplayInfo &di, Page::SelectionCause sc) {
	Page::SelectionResponse sr;
	do {
		if (--page < GPS_Status) {
			page = PageCycle - 1;
		}
		sr = pages[page]->select(di, sc);
	} while (sr == Page::SkipPage);
	changePage(page);
}

void RunDisplay::changePage(int p) {
	assert(!pagechange);
	page = p;
	timer = pagetime;
	pagechange = true;
}

constexpr int RunDisplay::pagetime;

void RunDisplay::run()
try {
	// initialize input
	EvdevShared in0 = std::make_shared<Evdev>("/dev/input/event0");
	EvdevShared in1 = std::make_shared<Evdev>("/dev/input/event1");
	// configure input response
	configureInput(in0);
	configureInput(in1);
	in0->usePoller(poller);
	in1->usePoller(poller);
	// try to eat up waiting input events
	{ // the button event is causing lots of trouble
		int cnt = 0, prev;
		do {
			inputRotor = 0;
			prev = inputButton;
			inputButton = 0;
			// not sure why/if this needs a delay
			poller.wait(std::chrono::milliseconds(2));
			if (inputButton != prev) {
				cnt = 0;
			} else {
				++cnt;
			}
		} while (inputRotor || inputButton || prev || (cnt < 64));
	}
	bool lowBatt = false;
	bool critBatt = false;
	
	// create Page objects
	pages[GPS_Status] = std::unique_ptr<Page>(new GpsPage);
	pages[Totality_Times] = std::unique_ptr<Page>(new TotalityPage);
	pages[Totality_Wait] = std::unique_ptr<Page>(new TotalityWaitPage);
	pages[System] = std::unique_ptr<Page>(new SystemPage(batmon));
	pages[Error] = std::unique_ptr<Page>(new ErrorPage);
	pages[Notice] = std::unique_ptr<Page>(new NoticePage);
	pages[InTotality] = std::unique_ptr<Page>(new InTotalityPage);
	
	// start on second line
	disp->move(0, 1);

	// big loop for display output and user input
	while (!quit) {
		// sample battery status
		batmon.sample();
		// get the current time
		lcd.sampleTime(ts);
		// reports a synced clock even before GPS has a fix
		if (!syncdClock  && (ts.accuracy !=
			duds::data::unspecified<
				duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample::Quality
			>()
		)) {
			syncdClock = true;
		}
		// used for time display and computing the delay through this loop (UTC)
		time = duds::time::planetary::earth->posix(ts.value).time_of_day() +
			// be 64ms in the future!
			boost::posix_time::milliseconds(64);
		if (testTimeOffset) {
			time += boost::posix_time::seconds(testTimeOffset);
		}
		displaystuff.setTime(time.total_seconds());
		
		// check for low voltage
		if (!lowBatt && (batmon.busVoltage().value < 8.9)) {
			lowBatt = true;
			displaystuff.setNotice("Low battery voltage");
		} else if (!critBatt && (batmon.busVoltage().value < 8.5)) {
			critBatt = true;
			displaystuff.setNotice("Very low battery\nvoltage");
		}
		// get stuff to display
		DisplayInfo info(displaystuff.getInfo());
		
		// in-totality is the most important page
		if (pages[InTotality]->select(info, Page::SelectAuto) == Page::SelectPage) {
			if (page != InTotality) {
				changePage(InTotality);
			}
			// prevent page changes during totality
			pagechange = true;
		} else if (page == InTotality) {
			// see if notice page has something interesting
			if (pages[Notice]->select(info, Page::SelectAuto) == Page::SelectPage) {
				changePage(Notice);
			} else {
				// force an automatic page advance later
				timer = 1;
			}
		}
		
		// second most impotant page is the notice -- shows low battery messages
		if (!pagechange && info.notchg) {
			changePage(Notice);
		}
		
		// change on user input; pre-empted by above conditions
		if (!pagechange) {
			// display change on input
			if (inputRotor) {
				if (inputRotor > 0) {
					incPage(info, Page::SelectUser);
				} else {
					decPage(info, Page::SelectUser);
				}
				// show the page longer than after automatic advance
				timer *= 2;
				inputRotor = 0;
			}
		}

		// output status: have new result?
		if (!pagechange) {
			if (info.errchg) {
				// show new errors immediately
				changePage(Error);
			} else if (info.totchg) {
				// show new result immediately
				changePage(Totality_Times);
			}
		}
		
		// automatic advance logic
		if (!pagechange && !--timer) {
			incPage(info, Page::SelectAuto);
		}
		
		// show page
		if (pagechange) {
			if (page == GPS_Status) {
				// re-init the display in case of bad communications
				disp->initialize();
				disp->move(0, 1);
			}
			pages[page]->show(info, tds);
		} else {
			pages[page]->update(info, tds);
		}

		// only output time after the clock has been sync'd once
		if (syncdClock) {
			if (disp->rowPos() != 0) {
				std::cerr << "Bad row position for clock: " << disp->rowPos() <<
				std::endl;
				disp->move(10, 0);
			}
			if (disp->columnPos() != 12) {
				disp->clearTo(11, 0);
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
		} else {
			disp->clearTo(0, 1);
		}

		assert(disp->rowPos() == 1);
		assert(disp->columnPos() == 0);
		// second line; positioned for page show/update
		
		// clear page change
		pagechange = false;
		// compute time to begining of next second according to the time sample
		std::chrono::milliseconds delay(
			1000 - (time.total_milliseconds() % 1000)
		);
		// wait idle and check for input
		if (delay.count() > 0) {
			poller.wait(delay);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			poller.check();
		}
		// respond to input
		if (inputButton || // could change this to a menu if time permits
		(batmon.busVoltage().value < 8.3)) {
			int prev = 0, cnt = 0;
			while ((inputButton != prev) && (cnt < 64)) {
				if (prev == inputButton) {
					++cnt;
				} else {
					cnt = 0;
				}
				prev = inputButton;
				inputButton = 0;
				// not sure why this needs a delay
				poller.wait(std::chrono::milliseconds(2));
			}
			quit = 1;
			int q = system("/sbin/shutdown -h now");
			// won't get here
			return;
		}
		// user requested display page change handled with auto-advance of page
	}
} catch (...) {
	std::cerr << "Program failed in display thread:\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}
