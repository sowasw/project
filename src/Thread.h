#ifndef _THREAD_H_
#define _THREAD_H_

#include "Log.h"
#include "EventLoop.h"
#include <sys/eventfd.h>
#include <functional>
#include <pthread.h>

class Thread
{
public:
	typedef std::function<void()> ThreadFunc;

	explicit Thread(ThreadFunc threadFunc);
	Thread();
	~Thread();

	EventLoop *getLoop() { return loop_.get(); }
	struct event_base *getBase() { return loop_->getBase(); }

	void start();
	void join();
	void setAutoDelete(bool autoDelete);

private:
	void run();

	static void *threadMain(void *arg);
	static void wakeup_cb(int fd, short event, void *arg);
	void thread_cb() { LOG_INFO("Thread %ld created\n", threadId_); }

	pthread_t threadId_;
	bool autoDelete_;

	std::shared_ptr<EventLoop> loop_;

	int wakefd_;
	uint64_t wakecount_;

	ThreadFunc threadCallBack_;//线程启动时的回调
};

#endif
