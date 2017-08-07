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
#ifndef POLLER_HPP
#define POLLER_HPP

#include <sys/epoll.h>
#include <boost/exception/info.hpp>
#include <boost/noncopyable.hpp>
#include <mutex>
#include <map>
#include <memory>

struct PollerError : virtual std::exception, virtual boost::exception { };
struct PollerCreateError : PollerError { };

class PollResponse {
public:
	virtual void respond(int fd) = 0;
};

typedef std::shared_ptr<PollResponse>  PollResponseShared;

/**
 * A simple C++ interface to using Linux's epoll() function.
 * @author  Jeff Jackowski
 */
class Poller : boost::noncopyable {
	std::map<int, PollResponseShared> things;
	mutable std::mutex block;
	int epfd;
public:
	Poller();
	~Poller();
	void add(const PollResponseShared &prs, int fd, int events = EPOLLIN);
	PollResponseShared get(int fd) const;
	PollResponseShared remove(int fd);
	void wait(std::chrono::milliseconds timeout);
	void wait() {  // indefinite
		wait(std::chrono::milliseconds(-1));
	}
	void check() { // no block
		wait(std::chrono::milliseconds(0));
	}
};

#endif        //  #ifndef POLLER_HPP
