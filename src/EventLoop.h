#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include <stdio.h>
#include <event.h>
#include <list>
#include <memory>
#include <algorithm>

#include "Log.h"
#include "Protocol.h"

typedef void (*EventCallBack)(evutil_socket_t fd, short events, void *user_data);
typedef void (*EventReadCallBack)(struct bufferevent *bev, void *user_data);
typedef void (*EventWriteCallBack)(struct bufferevent *bev, void *user_data);

typedef struct wx_channel
{
    wx_channel();
    wx_channel(struct bufferevent *&bev1);

    void clear();

    struct bufferevent *bev;
    struct MessageHead head;
    char read_buf[MESSAGE_BUF_SIZE];
    char write_buf[MESSAGE_BUF_SIZE];
    MessageRead mr;
    MessageWrite mw;

    std::string host;
    int port;
} Channel;

typedef struct wx_event
{
    wx_event() : ev(NULL) {}
    wx_event(struct event *&ev1) : ev(ev1) {}

    struct event *ev;
} Event;

typedef std::shared_ptr<Channel> ConnPtr;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

    struct event_base *getBase() { return base; }

    bool addChannel(ConnPtr &conn);
    ConnPtr getConnPtr(struct bufferevent *bev);
    bool removeAndFreeChannel(ConnPtr &conn);

    bool addEvent(struct event *ev, struct timeval *tv);
    bool removeEvent(struct event *ev);
    bool freeEvent(struct event *ev);
    bool removeAndFreeEvent(struct event *ev) { return removeEvent(ev) && freeEvent(ev); }

    void run();
    void clear();

private:
    struct event_base *base;
    std::list<std::shared_ptr<Channel>> channels;
    std::list<std::shared_ptr<Event>> events;

    //BufferEventCallBack bevCb;//struct bufferevent 出错回调
private:

    //用于从底层数据结构中查找连接、事件
    struct EventCmp
    {
        EventCmp(struct event *&ev) : ev_(ev) {}
        bool operator()(const std::shared_ptr<Event> &l)
        {
            return l->ev == ev_;
        }
        struct event *ev_;
    };

    struct ChannelCmp
    {
        ChannelCmp(struct bufferevent *&bev) : bev_(bev) {}
        bool operator()(const std::shared_ptr<Channel> &l)
        {
            return l->bev == bev_;
        }
        struct bufferevent *bev_;
    };
};

#endif
