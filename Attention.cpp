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
#include "Attention.hpp"
#include <duds/hardware/devices/clocks/LinuxClockDriver.hpp>
#include <duds/time/planetary/Planetary.hpp>
#include <system_error>
#include <iostream>
#include <csignal>
#include <boost/exception/diagnostic_information.hpp>

extern std::sig_atomic_t quit;

Attention::Record::Record(int t, int pri, int p, Audible aud) :
time(t), priority(pri), page(p), sound(aud) { }

Attention::Attention(duds::hardware::interface::DigitalPin &buz, int testTimeOffset) :
buzzer(buz), page(-1) {
	running = std::thread(&Attention::run, this, testTimeOffset);
}

Attention::~Attention() {
	{
		std::lock_guard<std::mutex> lock(block);
		records.clear();
		page = 42;
	}
	change.notify_one();
	try {
		running.join();
	} catch (const std::system_error& e) {
		// invalid_argument may occur if thread has terminated
		if (e.code() != std::errc::invalid_argument) {
			// something else
			throw;
		}
	}
}

// in milliseconds
static int buzlen[Attention::Total] = {
	0,
	450,
	3100,
	400
};

void Attention::run(int testTimeOffset)
try {
	duds::hardware::interface::DigitalPinAccess buz;
	buzzer.access(&buz);
	// start by waiting
	std::unique_lock<std::mutex> lock(block);
	while (records.empty() && !quit) {
		change.wait(lock);
	}
	if (quit) {
		return;
	}
	duds::hardware::devices::clocks::LinuxClockDriver lcd;
	duds::hardware::devices::clocks::LinuxClockDriver::Measurement::TimeSample ts;
	boost::posix_time::time_duration time;
	// begin operating loop
	do {
		// get the current time
		lcd.sampleTime(ts);
		// find seconds since midnight UTC
		time = duds::time::planetary::earth->posix(ts.value).time_of_day();
		if (testTimeOffset) {
			time += boost::posix_time::seconds(testTimeOffset);
		}
		// find next item time-wise
		RecordContainer::index<index_time>::type &timeIdx =
			records.get<index_time>();
		RecordContainer::index<index_time>::type::iterator iter = timeIdx.begin();
		// it may be well in the past
		while ((iter != timeIdx.end()) && (iter->time - time.total_seconds()) < -4) {
			iter = timeIdx.erase(iter);			
		}
		// that may have eliminated everything
		if (iter == timeIdx.end()) {
			// wait until something happens
			change.wait(lock);
		} else {
			// find highest priority item; there may be a better way
			RecordContainer::index<index_time>::type::iterator next = iter;
			for (++next; (next != timeIdx.end()) && (next->time == iter->time); ++next) {
				if (next->priority < iter->priority) {
					iter = next;
				}
			}
			// work out time to wait
			std::chrono::milliseconds delay(
				iter->time * 1000 - time.total_milliseconds() - buzlen[iter->sound]
			);
			// time to make noise?
			if (delay.count() < 16) {
				page = iter->page;
				int snd = iter->sound;
				// remove all records for this time
				auto range = timeIdx.equal_range(iter->time);
				timeIdx.erase(range.first, range.second);
				// after unlocking, member variables must not be used
				lock.unlock();
				try {
					switch (snd) {
						case NoSound:
							break;
						case Notice:
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(150));
							buz.output(false);
							std::this_thread::sleep_for(std::chrono::milliseconds(150));
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(150));
							buz.output(false);
							break;
						case Time:
							// one beep
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(200));
							buz.output(false);
							std::this_thread::sleep_for(std::chrono::milliseconds(800));
							// two beeps
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(200));
							buz.output(false);
							std::this_thread::sleep_for(std::chrono::milliseconds(800));
							// three beeps
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(200));
							buz.output(false);
							std::this_thread::sleep_for(std::chrono::milliseconds(800));
							// long tone
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(1000));
							buz.output(false);
							break;
						case Warning:
							buz.output(true);
							std::this_thread::sleep_for(std::chrono::milliseconds(400));
							buz.output(false);
					}
				} catch (...) {
					std::cerr << "Attention thread error during buzzer output:\n" <<
					boost::current_exception_diagnostic_information()
					<< std::endl;
				}
				lock.lock();
			} else {
				// wait
				change.wait_for(lock, delay);
			}
		}
	} while ((page != 42) && !quit);
} catch (...) {
	std::cerr << "Program failed in attention thread:\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

void Attention::add(int time, int priority, int page, Audible sound) {
	std::lock_guard<std::mutex> lock(block);
	records.emplace(time, priority, page, sound);
	change.notify_one();
}

void Attention::remove(int page) {
	std::lock_guard<std::mutex> lock(block);
	RecordContainer::index<index_page>::type &pageIdx = records.get<index_page>();
	//std::pair<RecordContainer::index<index_page>::type::iterator
	auto range = pageIdx.equal_range(page);
	pageIdx.erase(range.first, range.second);
	change.notify_one();
}

int Attention::changeToPage() {
	std::lock_guard<std::mutex> lock(block);
	int ret = page;
	// reset attention page
	page = -1;
	return ret;
}
