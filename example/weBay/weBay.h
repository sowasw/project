#ifndef _WEBAY_H_
#define _WEBAY_H_

#include "EventLoop.h"
#include "TcpServer.h"
#include "Protocol.h"
#include "wx_mysql.h"
#include "wx_redis.h"
#include "wx_lib.h"
#include <vector>
#include <sstream>
#include <time.h>

#include "weBayType.h"

class WeBayServer
{
public:
    typedef std::function<void(ConnPtr &)> CmdCallback;

    WeBayServer(EventLoop *loop, short port, const char *conf);

    ~WeBayServer() { event_free(cleanEv_); }

    void onMessage(ConnPtr &conn);
    void onSend(ConnPtr &conn);

    void start() { server_.start(); }

    void signIn(ConnPtr &conn);
    void login(ConnPtr &conn);
    void showCommodities(ConnPtr &conn);
    void addCart(ConnPtr &conn);
    void getCart(ConnPtr &conn);
    void createOrder(ConnPtr &conn);
    void getOrder(ConnPtr &conn);

    //bool readMessage(ConnPtr &conn);

private:
    TcpServer server_;
    EventLoop *loop_;
    Mysql sql_;
    Redis redis_;

    std::vector<CmdCallback> cmdCbVec_; //业务回调函数

    bool refreshCommodityInfo_; //刷新商品信息的标志

    int cleanSessionFd_; //用于定期清理缓存的描述符
    struct event *cleanEv_;

private:
    void initMessageHead(ConnPtr &conn, int cmd);
    void resetMessageBodyLength(ConnPtr &conn);
    bool checkTocken(ConnPtr &conn, std::string &tocken, std::string &user);
    void updateTocken(ConnPtr &conn, std::string &tocken, std::string &user);

    bool refeshOrderCache(std::string &order, std::string &tocken, std::string &user);

    static void clean_session_cb(int fd, short event, void *argc);
};

//登录会话
class Session
{
public:
    Session(ConnPtr &conn, std::string &user_id, time_t &timestamp) : id_(user_id), host_(conn->host), loginTime_(timestamp) {}

private:
    std::string id_;
    std::string host_;
    time_t loginTime_;
    friend class Tocken;
};

//简易的登录令牌，为Seesion 3个字段连接的字符串取36字符，不足用‘*’填充
class Tocken
{
public:
    Tocken() {}
    std::string operator()(Session &ses);
};

#endif