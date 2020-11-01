#include "TcpServer.h"

#define PORT 9999

//使用多线程
class EchoServer
{
public:
    EchoServer(EventLoop *loop, short port) : loop_(loop), server_(loop, PORT)
    {
        server_.threadPoolInit(2);

        server_.setConnectionCallBack(std::bind(&EchoServer::onConn, this, std::placeholders::_1));
        server_.setMessageCallBack(std::bind(&EchoServer::onMessage, this, std::placeholders::_1));
        server_.setWriteCompleteCallBack(std::bind(&EchoServer::onSend, this, std::placeholders::_1));
    }
    void onConn(ConnPtr &conn);
    void onMessage(ConnPtr &conn);
    void onSend(ConnPtr &conn);

    void start() { server_.start(); }

private:
    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    evthread_use_pthreads();
    EventLoop loop;
    //TcpServer server(&loop, PORT);
    //server.threadPoolInit(2);

    EchoServer server(&loop, PORT);

    server.start();

    LOG_FATAL("end server\n");
    Log::join();

    return 0;
}

void EchoServer::onConn(ConnPtr &conn)
{
    struct timeval tv = {2, 0};
    bufferevent_set_timeouts(conn->bev, &tv, &tv);
    //bufferevent_settimeout(conn->bev, 3, 3); //ok
    LOG_INFO("Connection build, and set time out: %ld sec %ld usec\n", tv.tv_sec, tv.tv_usec);
}


void EchoServer::onMessage(ConnPtr &conn)
{
    struct evbuffer *input = bufferevent_get_input(conn->bev);
    int n;
    if ((n = evbuffer_get_length(input)) > 0) //缓冲区接收到的数据大小
    {
        if (bufferevent_write_buffer(conn->bev, input) < 0)
            LOG_ERROR("pthread: %ld bufferevent_read error!\n", pthread_self());
        else
            LOG_INFO("pthread: %ld read %d bytes!\n", pthread_self(), n);
    }
}

void EchoServer::onSend(ConnPtr &conn)
{
    struct evbuffer *output = bufferevent_get_output(conn->bev);
    int n;
    if ((n = evbuffer_get_length(output)) > 0)
    {
        LOG_ERROR("pthread: %ld evbuffer_get_length(output) error!\n", pthread_self());
    }
    else
        LOG_INFO("pthread: %ld flushed!\n", pthread_self());
}

