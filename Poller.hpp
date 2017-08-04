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
