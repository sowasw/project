#include "TcpServer.h"

#define PORT 9999

//使用自由函数作回调
void onMessage(ConnPtr &conn);
void onSend(ConnPtr &conn);

int main()
{
	EventLoop loop;
	TcpServer server(&loop, PORT);

	server.setMessageCallBack(std::bind(&onMessage, std::placeholders::_1)); //传递引用
	//MessageCallBack mf = std::bind(&onMessage, std::placeholders::_1);
	//server.setMessageCallBack(mf);
	WriteCompleteCallBack wf = std::bind(&onSend, std::placeholders::_1);
	server.setWriteCompleteCallBack(wf);

	server.start();

	return 0;
}

void onMessage(ConnPtr &conn)
{
	struct evbuffer *input = bufferevent_get_input(conn->bev);
	int n;
	if ((n = evbuffer_get_length(input)) > 0) //缓冲区接收到的数据大小
	{
		if (bufferevent_write_buffer(conn->bev, input) < 0)
            LOG_ERROR("pthread: %ld bufferevent_read error!\n", pthread_self());
        else
            LOG_INFO("pthread: %ld read %d bytes!\n", pthread_self(), n);

		// char buf[1024];
		// int nread = bufferevent_read(conn->bev, buf, 1024); //读到用户缓冲区
		// //        write(1,buf,nread);
		// if (nread > 0)
		// {
		// 	LOG_INFO("read %d bytes!\n", nread);
		// 	bufferevent_write(conn->bev, buf, nread); //发送数据，要放在读回调函数中
		// }
		// else
		// 	LOG_ERROR("bufferevent_read error!\n");
	}
}

void onSend(ConnPtr &conn)
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
