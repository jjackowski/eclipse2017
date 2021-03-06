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
#include <libevdev.h>
#include <boost/signals2/signal.hpp>
#include "Poller.hpp"

struct EvdevError : virtual std::exception, virtual boost::exception { };
struct EvdevFileOpenError : EvdevError { };
struct EvdevInitError : EvdevError { };
struct EvdevUnsupportedEvent : EvdevError { };

typedef boost::error_info<struct Info_EvdevEventType, unsigned int>
	EvdevEventType;
typedef boost::error_info<struct Info_EvdevEventType, unsigned int>
	EvdevEventCode;

union EventTypeCode {
	struct {
		std::uint16_t type;
		std::uint16_t code;
	};
	std::uint32_t typecode;
	EventTypeCode() = default;
	EventTypeCode(std::uint16_t t, std::uint16_t c) : type(t), code(c) { }
};

inline bool operator<(EventTypeCode etc0, EventTypeCode etc1) {
	return etc0.typecode < etc1.typecode;
}

class Evdev :
	boost::noncopyable,
	public PollResponse,
	public std::enable_shared_from_this<Evdev>
{
public:
	typedef boost::signals2::signal< void(EventTypeCode, std::int32_t)>
		InputSignal;
protected:
	typedef std::map<EventTypeCode, InputSignal>  InputMap;
	InputMap receivers;
	libevdev *dev;
	int fd;
public:
	Evdev(const std::string &path);
	Evdev(Evdev &&e);
	~Evdev();
	Evdev &operator=(Evdev &&old);
	virtual void respond(int fd);
	std::string name() const;
	bool hasEventType(unsigned int et) const;
	bool hasEventCode(unsigned int et, unsigned int ec) const;
	bool hasEvent(EventTypeCode etc) const;
	int value(unsigned int et, unsigned int ec) const;
	void usePoller(Poller &p);
	boost::signals2::connection inputConnect(
		EventTypeCode etc,
		const typename InputSignal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return receivers[etc].connect(slot, at);
	}
};

typedef std::shared_ptr<Evdev>  EvdevShared;
