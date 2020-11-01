#include "Thread.h"

Thread::Thread(ThreadFunc threadFunc)
	: autoDelete_(false), threadCallBack_(threadFunc)
{
	loop_ = std::make_shared<EventLoop>();

	wakefd_ = eventfd(0, EFD_NONBLOCK); //
	if (wakefd_ < 0)
	{
		LOG_SYSERROR("Thread() error: eventfd() error\n");
		exit(1);
	}
	wakecount_ = 0;

	struct event *ev = event_new(loop_->getBase(), wakefd_, EV_READ | EV_PERSIST, wakeup_cb, this);
	struct timeval tv = {0, 500000};
	event_add(ev, &tv);
	loop_->addEvent(ev, &tv);
}

Thread::Thread() : autoDelete_(false)
{
	loop_ = std::make_shared<EventLoop>();

	threadCallBack_ = std::bind(&Thread::thread_cb, this);

	wakefd_ = eventfd(0, EFD_NONBLOCK); //
	if (wakefd_ < 0)
	{
		LOG_SYSERROR("Thread() error: eventfd() error\n");
		exit(1);
	}
	wakecount_ = 0;

	struct event *ev = event_new(loop_->getBase(), wakefd_, EV_READ | EV_PERSIST, wakeup_cb, this);
	struct timeval tv = {0, 1000};
	event_add(ev, &tv);
	loop_->addEvent(ev, &tv);
}

Thread::~Thread()
{
	LOG_WARNING("end thread: %ld\n", threadId_);
}

void Thread::start()
{

	pthread_create(&threadId_, NULL, &threadMain, this);
}

void Thread::join()
{
	pthread_join(threadId_, NULL);
}

void Thread::setAutoDelete(bool autoDelete)
{
	this->autoDelete_ = autoDelete;
}

void Thread::run()
{
	//LOG_INFO("Thread %ld created\n", threadId_);
	threadCallBack_();
	loop_->run();
}

void *Thread::threadMain(void *arg)
{
	Thread *thread = static_cast<Thread *>(arg); //静态成员函数不能使用非静态成员函数
	thread->run();

	if (thread->autoDelete_) //线程结束，自动销毁线程对象
		delete thread;

	return NULL;
}

void Thread::wakeup_cb(int fd, short event, void *arg)
{
	Thread *thread = static_cast<Thread *>(arg);
	eventfd_read(thread->wakefd_, &thread->wakecount_);
	//LOG_DEBUG("Thread %ld wakeup\n", pthread_self());
}
