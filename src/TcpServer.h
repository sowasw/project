#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <unistd.h>
#include <event.h>

#include "Log.h"
#include "EventLoop.h"
#include "Thread.h"

#include <functional>
#include <vector>

typedef void (*EventCallBack)(evutil_socket_t fd, short events, void *user_data);
typedef void (*EventReadCallBack)(struct bufferevent *bev, void *user_data);
typedef void (*EventWriteCallBack)(struct bufferevent *bev, void *user_data);

typedef void (*BufferEventCallBack)(struct bufferevent *bev, short events, void *user_data);

typedef void (*ListenCallBack)(struct evconnlistener *, evutil_socket_t,
                               struct sockaddr *, int socklen, void *);

typedef std::function<void(ConnPtr &)> ConnectionCallBack;
typedef std::function<void(ConnPtr &)> MessageCallBack;
typedef std::function<void(ConnPtr &)> WriteCompleteCallBack;
typedef std::function<void(ConnPtr &)> CloseCallBack;
typedef std::function<void(ConnPtr &)> ErrorCallBack;
typedef std::function<void(ConnPtr &)> TimeOutCallBack;

//如果服务器用到线程池，则把接受的连接添加到线程池中进行处理，
//服务器主线程只负责接受连接，然后分发到各个线程
class TcpServer
{
public:
    TcpServer(EventLoop *loop, short port);
    ~TcpServer();

    void threadPoolInit(int nthread); //创建线程池，需要在主函数开始时就调用evthread_use_pthreads();

    //服务器主要实现这几个回调函数
    void setConnectionCallBack(const ConnectionCallBack &cb) { connCb_ = cb; }
    void setMessageCallBack(const MessageCallBack &cb) { messageCb_ = cb; }
    void setWriteCompleteCallBack(const WriteCompleteCallBack &cb) { writeCompleteCb_ = cb; }
    void setCloseCallBack(const CloseCallBack &cb) { closeCb_ = cb; }
    void setErrorCallBack(const ErrorCallBack &cb) { errorCb_ = cb; }
    void setTimeOutCallBack(const TimeOutCallBack &cb) { timeOutCb_ = cb; }

    void start(); //启动服务器
    void exit();
    void exitThreadPool(); //关闭线程池
    void joinAll();

    //libevent flags
    void setListenOpt(unsigned int flags, int backlog);                 //for evconnlistener_new_bind
    void setBuffereventOpt(int opts);                                   //for bufferevent_socket_new
    void setBuffereventEvents(short enableEvents, short disableEvents); //for bufferevent_enable,disable

    //设置底层回调 libevent支持的C语言回调，这个设置了就不能用上面的回调了
    void setBackGroundListenCallBacks(ListenCallBack &cb);
    void setBackGroundCallBacks(EventReadCallBack &readcb, EventWriteCallBack &writecb,
                                BufferEventCallBack &eventcb);

    struct sockaddr_in *getAddr() { return &sin_; }
    struct event_base *getBase() { return base_; }
    EventLoop *getLoop() { return loop_; }
    EventLoop *getLoop(struct bufferevent *bev); //

    bool addChannel(EventLoop *loop, ConnPtr &conn);
    bool addEvent(EventLoop *loop, struct event *ev, struct timeval *tv);
    bool removeChannel(EventLoop *loop, ConnPtr &conn);
    bool removeEvent(EventLoop *loop, struct event *ev) { return loop->removeAndFreeEvent(ev); }

private:
    void newLinstener(TcpServer *server); //开始监听

    //用于默认的服务器回调
    //onConnection，onMessage等6个服务器接口
    void defaultCallBack(ConnPtr &cptr, const char *msg) { LOG_INFO(msg); }
    //void defaultConnCallBack(ConnPtr &cptr);
    void defaultCloseCallBack(ConnPtr &cptr, const char *msg);
    void defaultErrorCallBack(ConnPtr &cptr);
    void defaultTimeOutCallBack(ConnPtr &cptr);

private:
    EventLoop *loop_;         //监听的loop
    struct event_base *base_; //

    EventLoop *loopToAddConnection_; //待加入事件的循环

    //线程池
    std::vector<std::shared_ptr<Thread>> threadPool_;
    //std::vector<EventLoop *> evPool_;//线程池中的循环
    int nthread_;
    int loopIndex_;

    short listenPort_;
    struct sockaddr_in sin_; //server addr(listen)

    //for evconnlistener_new_bind
    struct evconnlistener *listener_;
    unsigned int listenOpts_;
    int backlog_;

    int bevOpts_;           //for bufferevent_socket_new
    short bevEnableEvents_; //for bufferevent_enable
    short bevDisableEvents_;

    //用于服务器主要接口
    ConnectionCallBack connCb_;
    MessageCallBack messageCb_;
    WriteCompleteCallBack writeCompleteCb_;
    CloseCallBack closeCb_;
    ErrorCallBack errorCb_;
    TimeOutCallBack timeOutCb_;

    //底层回调，可以设置
    ListenCallBack listenCb_;  //evconnlistener_new_bind
    EventReadCallBack readCb_; //和以下2个由bufferevent_setcb调用
    EventWriteCallBack writeCb_;
    BufferEventCallBack eventCb_;

private:
    //默认的底层回调，
    //用于evconnlistener_new_bind， bufferevent_setcb
    static void listener_cb(struct evconnlistener *, evutil_socket_t,
                            struct sockaddr *, int socklen, void *);
    static void conn_readcb(struct bufferevent *, void *); //
    static void conn_writecb(struct bufferevent *, void *);
    static void conn_eventcb(struct bufferevent *, short, void *);
};

#endif
