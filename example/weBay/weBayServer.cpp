#include "weBay.h"

#define SERVER_CONF "../weBayServer.conf"
#define SERVER_PORT 13621

int main()
{
    wx_signal(SIGPIPE, SIG_IGN);

    EventLoop loop;
    WeBayServer server(&loop, SERVER_PORT, SERVER_CONF);
    server.start();

    return 0;
}