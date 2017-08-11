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
#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/instruments/INA219.hpp>
#include <duds/hardware/interface/DigitalPin.hpp>
#include <duds/hardware/devices/clocks/LinuxClockDriver.hpp>
#include "Poller.hpp"
#include "Page.hpp"

class RunDisplay : boost::noncopyable {
	/**
	 * Seconds to spend on a page after automatically advancing to it.
	 */
	static constexpr int pagetime = 10;
	const std::shared_ptr<duds::hardware::devices::displays::HD44780> &disp;
	duds::hardware::devices::displays::TextDisplayStream tds;
	duds::hardware::devices::instruments::INA219 &batmon;
	duds::hardware::interface::DigitalPin &buzzer;
	/**
	 * Used to get input while waiting for it with epoll().
	 */
	Poller poller;
	/**
	 * The clock used to sample the current time.
	 */
	duds::hardware::devices::clocks::LinuxClockDriver lcd;
	/**
	 * The sample of the current time.
	 */
	duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample ts;
	/**
	 * Seconds since the last midnight, UTC.
	 */
	boost::posix_time::time_duration time;
	/**
	 * Page ordering.
	 */
	enum DisplayPage {
		GPS_Status,
		Elcipse_Times,
		Totality_Times,
		Totality_Wait,
		Sun_Azimuth,
		Sun_Now,
		Schedule,
		System,
		Error,
		Notice,
		InTotality,
		PageCycle         // if here, cycle to first page
	};
	/**
	 * The page objects.
	 */
	std::unique_ptr<Page> pages[PageCycle];
	/**
	 * Index of the current page.
	 */
	int page;
	int testTimeOffset;
	/**
	 * Seconds remaining before automatically advancing to the next page.
	 */
	int timer;
	bool pagechange;
	bool syncdClock;

	void incPage(const DisplayInfo &di, Page::SelectionCause sc);
	void decPage(const DisplayInfo &di, Page::SelectionCause sc);
	void changePage(int p);

public:
	RunDisplay(
		const std::shared_ptr<duds::hardware::devices::displays::HD44780> &hd,
		duds::hardware::devices::instruments::INA219 &batt,
		duds::hardware::interface::DigitalPin &buz,
		int toff
	);
	void run();
};
