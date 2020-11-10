#include "TcpServer.h"

#define PORT 9999

//使用类成员函数作为回调
class EchoServer
{
public:
    EchoServer(EventLoop *loop, short port) : loop_(loop), server_(loop, PORT)
    {
        server_.setMessageCallBack(std::bind(&EchoServer::onMessage, this, std::placeholders::_1));
        server_.setWriteCompleteCallBack(std::bind(&EchoServer::onSend, this, std::placeholders::_1));
    }
    void onMessage(ConnPtr &conn);
    void onSend(ConnPtr &conn);

    void start() { server_.start(); }

private:
    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    EchoServer server(&loop, PORT);

    server.start();

    return 0;
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
        LOG_ERROR("evbuffer_get_length(output) error!\n");
    }
    else
        LOG_INFO("flushed!\n");
}
