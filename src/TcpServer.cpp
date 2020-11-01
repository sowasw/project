#include "TcpServer.h"

TcpServer::TcpServer(EventLoop *loop, short port) : loop_(loop), listenPort_(port)
{
    base_ = loop->getBase();
    loopToAddConnection_ = loop;
    loopIndex_ = 0;

    memset(&sin_, 0, sizeof(sin_));
    sin_.sin_family = AF_INET;
    sin_.sin_port = htons(port);

    listenOpts_ = LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE;
    backlog_ = -1;

    bevOpts_ = BEV_OPT_CLOSE_ON_FREE; //for bufferevent_socket_new,释放bufferevent时关闭套接字
    bevEnableEvents_ = EV_READ;       //默认触发读事件
    bevDisableEvents_ = 0;

    connCb_ = std::bind(&TcpServer::defaultCallBack, this, std::placeholders::_1, "Connection build!\n");
    messageCb_ = std::bind(&TcpServer::defaultCallBack, this, std::placeholders::_1, "Recv message!\n");
    writeCompleteCb_ = std::bind(&TcpServer::defaultCallBack, this, std::placeholders::_1, "Send message!\n");
    closeCb_ = std::bind(&TcpServer::defaultCloseCallBack, this, std::placeholders::_1, "Connection closed!\n");
    errorCb_ = std::bind(&TcpServer::defaultErrorCallBack, this, std::placeholders::_1);
    timeOutCb_ = std::bind(&TcpServer::defaultTimeOutCallBack, this, std::placeholders::_1);

    listenCb_ = listener_cb;
    readCb_ = conn_readcb;
    writeCb_ = conn_writecb;
    eventCb_ = conn_eventcb;

    LOG_TRACE("init the server\n");

    nthread_ = 0;
    threadPoolInit(0);
}

TcpServer::~TcpServer()
{
    //closeCb_();

    if (listener_)
        evconnlistener_free(listener_);
    loop_->clear();
    threadPool_.clear();
}

void TcpServer::threadPoolInit(int nthread)
{
    if (nthread <= 0)
        return;
    LOG_TRACE("init threadpool: %d threads\n", nthread);
    nthread_ = nthread;
    for (int i = 0; i < nthread_; i++)
    {
        threadPool_.push_back(std::make_shared<Thread>());
        threadPool_[i]->start();
    }

    if (nthread_ > 0)
    {
        loopToAddConnection_ = threadPool_[0]->getLoop();
        bevOpts_ |= BEV_OPT_THREADSAFE;
    }
    LOG_TRACE("threads created\n");
}

void TcpServer::start()
{
    newLinstener(this);
    loop_->run();

    exit();
    joinAll();
}

void TcpServer::exit()
{
    event_base_loopbreak(base_);
    exitThreadPool();
}

void TcpServer::exitThreadPool()
{
    for (int i = 0; i < nthread_; i++)
        event_base_loopbreak(threadPool_[i]->getBase());
}

void TcpServer::joinAll()
{
    for (int i = 0; i < nthread_; i++)
        threadPool_[i]->join();
}

void TcpServer::setListenOpt(unsigned int flags, int backlog)
{
    listenOpts_ = flags;
    backlog_ = backlog;
}
void TcpServer::setBuffereventOpt(int opts)
{
    bevOpts_ = opts;
}
void TcpServer::setBuffereventEvents(short enableEvents, short disableEvents)
{
    bevEnableEvents_ = enableEvents;
    bevDisableEvents_ = disableEvents;
}

void TcpServer::setBackGroundListenCallBacks(ListenCallBack &cb)
{
    if (cb != NULL)
        listenCb_ = cb;
}

void TcpServer::setBackGroundCallBacks(EventReadCallBack &readcb,
                                       EventWriteCallBack &writecb,
                                       BufferEventCallBack &eventcb)
{
    if (readcb)
        readCb_ = readcb;
    if (writecb)
        writeCb_ = writecb;
    if (eventcb)
        eventCb_ = eventcb;
}

EventLoop *TcpServer::getLoop(struct bufferevent *bev)
{
    if (bev->ev_base == base_)
        return loop_;

    for (int i = 0; i < nthread_; i++)
        if (bev->ev_base == threadPool_[i]->getBase())
            return threadPool_[i]->getLoop();
}

bool TcpServer::addChannel(EventLoop *loop, ConnPtr &conn)
{
    return loop->addChannel(conn);
}

bool TcpServer::addEvent(EventLoop *loop, struct event *ev, struct timeval *tv)
{
    return loop->addEvent(ev, tv);
}

bool TcpServer::removeChannel(EventLoop *loop, ConnPtr &conn)
{
    return loop->removeAndFreeChannel(conn);
}

void TcpServer::newLinstener(TcpServer *server)
{
    listener_ = evconnlistener_new_bind(base_, listenCb_, this,
                                        listenOpts_, backlog_,
                                        (struct sockaddr *)&sin_,
                                        sizeof(sin_));

    if (!listener_)
    {
        LOG_ERROR("Could not create a listener!\n");
        ::exit(1);
    }

    char str[32];
    inet_ntop(AF_INET, &sin_.sin_addr, str, sizeof(str));
    LOG_INFO("start listen: %s:%d\n", str, listenPort_);
}

void TcpServer::defaultCloseCallBack(ConnPtr &cptr, const char *msg)
{
    bufferevent_free(cptr->bev);
    getLoop(cptr->bev)->removeAndFreeChannel(cptr);
    LOG_WARNING(msg);
}

void TcpServer::defaultErrorCallBack(ConnPtr &cptr)
{
    LOG_ERROR("Got an error on the connection: %s\n", strerror(errno));
    for (int i = 0; i < nthread_; i++)
        event_base_loopbreak(threadPool_[i]->getBase());
    event_base_loopbreak(base_);
}

void TcpServer::defaultTimeOutCallBack(ConnPtr &cptr)
{
    LOG_WARNING("Time out\n");
    bufferevent_enable(cptr->bev, bevEnableEvents_);
}

void TcpServer::listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                            struct sockaddr *sa, int socklen, void *user_data)
{
    ///connCb_
    TcpServer *tcp = static_cast<TcpServer *>(user_data);

    //tcp->clisin_.sin_addr;
    //tcp->clisin_.sin_port;
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;

    char str[32];
    memset(str, 0, 32);
    inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str));
    //LOG_TRACE("Connection addr: %s, port: %d\n", str, ntohs(sin->sin_port));

    struct event_base *base = tcp->loopToAddConnection_->getBase();
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, tcp->bevOpts_); //底层封装量readv writev
    if (!bev)
    {
        LOG_SYSERROR("Error constructing bufferevent!\n");
        event_base_loopbreak(base);
        ::exit(1);
    }
    //LOG_INFO("connection build!\n");

    //下面的操作也可以放在用户回调connCb_中
    bufferevent_setcb(bev, tcp->readCb_, tcp->writeCb_, tcp->eventCb_, tcp);
    bufferevent_enable(bev, tcp->bevEnableEvents_);
    bufferevent_disable(bev, tcp->bevDisableEvents_);
    //LOG_INFO("bufferevent_setcb!\n");

    ConnPtr connptr = std::make_shared<Channel>(bev);
    connptr->host = str;
    connptr->port = ntohs(sin->sin_port);
    LOG_TRACE("Connection addr: %s, port: %d\n", connptr->host.c_str(), connptr->port);
    tcp->connCb_(connptr);

    tcp->loopToAddConnection_->addChannel(connptr);

    if (tcp->nthread_ > 0)
    {
        tcp->loopIndex_ = ++tcp->loopIndex_ % tcp->nthread_;
        tcp->loopToAddConnection_ = tcp->threadPool_[tcp->loopIndex_]->getLoop();
    }
}

void TcpServer::conn_readcb(struct bufferevent *bev, void *user_data)
{
    ///messageCb_(connptr);
    TcpServer *tcp = static_cast<TcpServer *>(user_data);
    ConnPtr connptr = tcp->getLoop(bev)->getConnPtr(bev);
    tcp->messageCb_(connptr);
}

void TcpServer::conn_writecb(struct bufferevent *bev, void *user_data)
{
    ///writeCompleteCb_
    TcpServer *tcp = static_cast<TcpServer *>(user_data);
    ConnPtr connptr = tcp->getLoop(bev)->getConnPtr(bev);
    tcp->writeCompleteCb_(connptr); //写操作完成
}

void TcpServer::conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    ///closeCb_
    TcpServer *tcp = static_cast<TcpServer *>(user_data);
    ConnPtr connptr = tcp->getLoop(bev)->getConnPtr(bev);
    //tcp->closeCb_(connptr);

    //下面可以打包到用户回调closeCb_中,也可以拆分为几个单元
    //if (events & (BEV_EVENT_EOF | BEV_EVENT_WRITING | BEV_EVENT_READING))//会导致Resource temporarily unavailable
    if (events & (BEV_EVENT_EOF))
    {
        //LOG_WARNING("Connection closed.\n");
        //onClose
        LOG_ERROR("Client closed: %s\n", strerror(errno));
        //tcp->closeCb_(connptr);
    }
    else if (events & BEV_EVENT_ERROR)
    {
        //LOG_ERROR("Got an error on the connection: %s\n",strerror(errno));
        //onError
        //event_base_loopbreak(tcp->base_);
        tcp->errorCb_(connptr);
    }
    /* None of the other events can happen here, since we haven't enabled
	* timeouts */

    else if (events & BEV_EVENT_TIMEOUT) //超时，会disable,需重新enable
    {
        //LOG_INFO("TIMEOUT\n");
        //onTimeout
        tcp->timeOutCb_(connptr);
    }

    else if (events & BEV_EVENT_READING)
    {
        LOG_ERROR("Got an error on READING: %s\n", strerror(errno));
    }

    else if (events & BEV_EVENT_WRITING)
    {
        LOG_ERROR("Got an error on WRITING: %s\n", strerror(errno));
    }
}
