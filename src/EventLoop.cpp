#include "EventLoop.h"

Channel::wx_channel() : bev(NULL), head({0, 0}), mr(read_buf), mw(write_buf)
{
    memset(read_buf, 0, MESSAGE_BUF_SIZE);
    memset(write_buf, 0, MESSAGE_BUF_SIZE);
}

Channel::wx_channel(struct bufferevent *&bev1) : bev(bev1), head({0, 0}), mr(read_buf), mw(write_buf)
{
    memset(read_buf, 0, MESSAGE_BUF_SIZE);
    memset(write_buf, 0, MESSAGE_BUF_SIZE);
}

void Channel::clear()
{
    memset(read_buf, 0, MESSAGE_BUF_SIZE);
    memset(write_buf, 0, MESSAGE_BUF_SIZE);
    memset(&head, 0, sizeof(head));
    mr.clear();
    mw.clear();
}

EventLoop::EventLoop()
{
    base = event_base_new();
}

EventLoop::~EventLoop()
{
    clear();

    if (base)
        event_base_free(base);
}

bool EventLoop::addChannel(ConnPtr &conn)
{
    if (!conn->bev)
        return false;
    channels.push_back(conn);
    return true;
}

ConnPtr EventLoop::getConnPtr(struct bufferevent *bev)
{
    auto it = std::find_if(channels.begin(), channels.end(), ChannelCmp(bev));
    return *it;
}

bool EventLoop::removeAndFreeChannel(ConnPtr &conn)
{
    if (!conn->bev)
        return false;

    channels.remove_if(ChannelCmp(conn->bev));
    bufferevent_free(conn->bev);
    return true;
}

bool EventLoop::addEvent(struct event *ev, struct timeval *tv)
{
    if (!ev)
        return false;

    std::shared_ptr<Event> evsh = std::make_shared<Event>();
    evsh->ev = ev;
    events.push_back(evsh);
    //event_add(events.front()->ev, tv);
    return true;
}

bool EventLoop::removeEvent(struct event *ev)
{
    if (!ev)
        return false;

    //auto eviter = std::find_if(events.begin(), events.end(), EventCmp(ev));
    //std::shared_ptr<Event> evsh = *eviter;
    //events.remove(evsh);

    events.remove_if(EventCmp(ev));

    //event_del(evsh->ev);
    //event_free(evsh->ev);

    return true;
}

bool EventLoop::freeEvent(struct event *ev)
{
    if (!ev)
        return false;

    event_free(ev);
    return true;
}

void EventLoop::run()
{
    event_base_dispatch(base);
}

void EventLoop::clear()
{
    while (!channels.empty())
    {
        std::shared_ptr<Channel> chn = channels.front();
        if (chn->bev)
            bufferevent_free(chn->bev);
        channels.pop_front();
    }

    while (!events.empty())
    {
        std::shared_ptr<Event> ev = events.front();
        if (ev->ev)
            event_free(ev->ev);
        events.pop_front();
    }
}
