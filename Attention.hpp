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
#ifndef ATTENTION_HPP
#define ATTENTION_HPP

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/tag.hpp>
#include <duds/hardware/interface/DigitalPin.hpp>
#include <thread>

class Attention {
public:
	enum Audible {
		NoSound,
		Notice,
		Time,
		Warning,
		Total
	};
private:
	struct Record {
		int time;
		int priority;
		int page;
		Audible sound;
		Record() = default;
		Record(int t, int pri, int p, Audible aud);
	};
	typedef boost::multi_index::multi_index_container<
		Record,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<struct index_time>,
				boost::multi_index::member<
					Record, int, &Record::time
				>
			>,
			/*
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<struct index_priority>,
				boost::multi_index::member<
					Record, int, &Record::priority
				>
			>,
			*/
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<struct index_page>,
				boost::multi_index::member<
					Record, int, &Record::page
				>
			>
		>
	> RecordContainer;
	RecordContainer records;
	duds::hardware::interface::DigitalPin &buzzer;
	std::thread running;
	std::mutex block;
	std::condition_variable change;
	int page;
	void run(int testTimeOffset);
public:
	Attention(duds::hardware::interface::DigitalPin &buz, int testTimeOffset);
	~Attention();
	void add(int time, int priority, int page, Audible sound);
	void remove(int page);
	int changeToPage();
};

#endif        //  #ifndef ATTENTION_HPP
