#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef _WIN32
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#include <sys/socket.h>
#endif
#include <event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include <unistd.h>
#include <string>
#include <iostream>

#include "Protocol.h"
#include "TcpServer.h" //ConnPtr
#include "weBayType.h"

using namespace std;

#define MAXLINE 4096

static const int PORT = 13621;

//连接
struct bufferevent *conn_bev;
ConnPtr conn;
string tocken; //登录令牌

//注册读stdin
struct event stdinev; //#include <event.h>
static void stdin_cb(int fd, short event, void *argc);

//接受连接的fd的读写、错误回调函数
static void conn_readcb(struct bufferevent *, void *); //
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);

//信号回调函数
static void signal_cb(evutil_socket_t, short, void *);

//业务回调
void sign_in();                 //注册
void login();                   //登录
void show_commodities();        //获取商品信息
void add_commodities_to_cart(); //添加商品到购物车
void get_cart();                //获取购物车
void create_order();                  //结算
void get_order();               //浏览订单

//服务器响应后回调
void resp_sign_in();
void resp_login();
void resp_show_commodities();
void resp_add_commodities_to_cart();
void resp_get_cart();
void resp_create_order();
void resp_get_order();

//客户端开启界面
void client_start();
void key_tips();

//
int main(int argc, char **argv)
{
    struct event_base *base;
    struct event *event_sigpipe;
    struct event *event_sigint;
    //struct event stdinev; //#include <event.h>
    struct sockaddr_in sin;

    base = event_base_new();
    if (!base)
    {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr) <= 0)
    {
        fprintf(stderr, "Error inet_pton()!\n");
        event_base_loopbreak(base);
        return 1;
    }

    ///
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    conn_bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!conn_bev)
    {
        fprintf(stderr, "Error constructing bufferevent!\n");
        event_base_loopbreak(base);
        return 1;
    }
    conn = std::make_shared<Channel>(conn_bev);

    //bufferevent_setcb(conn_bev, conn_readcb, conn_writecb, conn_eventcb, NULL);
    bufferevent_setcb(conn_bev, conn_readcb, NULL, conn_eventcb, NULL); //写回调在这里不用
    bufferevent_enable(conn_bev, EV_WRITE | EV_READ);

    if ((bufferevent_socket_connect(conn_bev, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        fprintf(stderr, "Error bufferevent_socket_connect()!\n");
        event_base_loopbreak(base);
        return 1;
    }

    ///事件中加入读标准输入
    event_assign(&stdinev, base, 0, EV_PERSIST | EV_READ, stdin_cb, (void *)conn_bev);
    event_add(&stdinev, NULL);

    ///处理信号事件
    event_sigint = evsignal_new(base, SIGINT, signal_cb, (void *)base);
    event_sigpipe = evsignal_new(base, SIGPIPE, signal_cb, (void *)base);

    if (!event_sigint || event_add(event_sigint, NULL) < 0)
    {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return 1;
    }
    if (!event_sigpipe || event_add(event_sigpipe, NULL) < 0)
    {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return 1;
    }

    event_base_dispatch(base);

    event_free(event_sigint);
    event_free(event_sigpipe);
    event_base_free(base);

    fprintf(stderr, "done\n");
    return 0;
}

///
static void
stdin_cb(int fd, short event, void *args)
{
    struct bufferevent *bev = (struct bufferevent *)args;
    char buf[128];

again:
    memset(buf, 0, sizeof(buf));
    if (read(fd, buf, sizeof(buf)) == 2)
    {
        if (buf[0] == '0')
        {
            sign_in();
        }
        else if (buf[0] == '1')
        {
            login();
        }
        else if (buf[0] == '2')
        {
            show_commodities();
        }
        else if (buf[0] == '3')
        {
            add_commodities_to_cart();
        }
        else if (buf[0] == '4')
        {
            get_cart();
        }
        else if (buf[0] == '5')
        {
            create_order();
        }
        else if (buf[0] == '6')
        {
            get_order();
        }
        else if (buf[0] == 'q' || buf[0] == 'Q')
        {
            exit(1);
        }
        else if (buf[0] == 'c')
            client_start();
    }
    else
    {
        goto again;
    }
}

//bufferevent底层使用LT模式，尽量读到evbuffer;
//bufferevent_read接口类似ET模式行为，一次没读完不会触发读回调，直到新数据到来
static void
conn_readcb(struct bufferevent *bev, void *user_data) //可以读触发此回调
{
    struct evbuffer *input = bufferevent_get_input(conn->bev);
    size_t n;

    if ((n = evbuffer_get_length(input)) > 0) //缓冲区接收到的数据大小
    {
        size_t nread;

        if (n < MESSAGE_HEAD_BUF_SIZE)
        {
            fprintf(stderr, "Read message head error, close the connection\n");
            //消息长度不足消息头，关闭连接
            exit(-1);
        }

        //读取消息头
        bufferevent_read(conn->bev, conn->read_buf, MESSAGE_HEAD_BUF_SIZE);
        conn->mr.readMessageHead(&conn->head);
        if (conn->head.body_size > MESSAGE_MAX_BODY_SIZE || conn->head.body_size < 0)
        {
            fprintf(stderr, "Message body too large\n"); //消息体长度过不合法，关闭连接
            exit(-1);
        }
        //fprintf(stderr, "body_size: %ld, cmd: %d\n", conn->head.body_size, conn->head.cmd);

        //读取消息体
        nread = bufferevent_read(conn->bev, conn->read_buf + 8, conn->head.body_size);
        if (nread != conn->head.body_size)
        {
            fprintf(stderr, "Message body: nread != conn->head.body_size\n");
            exit(-1);
        }

        //读取客户端数据后的业务回调
        if (conn->head.cmd == CMD_SIGN)
            resp_sign_in();
        else if (conn->head.cmd == CMD_LOGIN)
            resp_login();
        else if (conn->head.cmd == CMD_SHOWCOM)
            resp_show_commodities();
        else if (conn->head.cmd == CMD_ADDCART)
            resp_add_commodities_to_cart();
        else if (conn->head.cmd == CMD_GETCART)
            resp_get_cart();
        else if (conn->head.cmd == CMD_CREATEODER)
            resp_create_order();
        else if (conn->head.cmd == CMD_GETORDER)
            resp_get_order();

        conn->clear(); //清空缓冲区，回到初始状态

        key_tips();
    }
}

static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
    /*struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0)
    {
        //printf("flushed answer\n");
    }*/
}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    if (events & BEV_EVENT_EOF)
    {
        fprintf(stderr, "Connection closed...\n");
        event_base_loopexit(bev->ev_base, NULL);
    }
    else if (events & BEV_EVENT_ERROR)
    {
        fprintf(stderr, "Got an error on the connection: %s\n", strerror(errno));
        event_base_loopexit(bev->ev_base, NULL);
    }
    else if (events & BEV_EVENT_CONNECTED) //发起连接端（bufferevent_socket_connect）会走到这
    {
        fprintf(stderr, "Connected\n");
        client_start();
    }
    else if (events & BEV_EVENT_TIMEOUT) //超时，会disable？需重新enable
    {
        fprintf(stderr, "TIMEOUT\n");
        //enable。。。
    }
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    if (sig == SIGINT)
    {
        struct event_base *base = (struct event_base *)user_data;
        struct timeval delay = {0, 300000};
        fprintf(stderr, "\nCaught an interrupt signal; exiting cleanly in 0.3 seconds.\n");
        event_base_loopexit(base, &delay);
    }
}

void sign_in()
{
    event_del(&stdinev); //让出标准输入

    //conn->clear();
    MessageHead mh = {0, CMD_SIGN};
    conn->mw.writeMessageHead(&mh); //初始化消息头部，后面再计算消息体长度再写入

    //从stdin读数据
    string user, password, email;
    do
    {
        cout << "请输入用户名（不超过20字符）: \n";
        cin >> user;
    } while (user.length() > 20);

    do
    {
        cout << "请输入密码（不超过20字符）: \n";
        cin >> password;
    } while (password.length() > 20);

    do
    {
    again:
        cout << "请输入邮箱（不超过36字符）: \n";
        cin >> email;
        if (email.find("@") == -1 || email.length() < 3)
        {
            cerr << "请输入正确的邮箱格式\n";
            //continue;
            goto again;
        }
    } while (email.length() > 36);

    //写到消息缓冲区
    conn->mw.writeString(&user, 20);
    conn->mw.writeString(&password, 20);
    conn->mw.writeString(&email, 36);

    //重置消息体长度
    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);

    //发送到服务器
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());

    event_add(&stdinev, NULL); //重新注册标准输入以选择业务
}
void login()
{
    tocken.assign(""); //清空令牌

    event_del(&stdinev); //让出标准输入

    //conn->clear();
    MessageHead mh = {0, CMD_LOGIN};
    conn->mw.writeMessageHead(&mh); //初始化消息头部，后面再计算消息体长度再写入

    //从stdin读数据
    string user, password;
    do
    {
        cout << "请输入用户名（不超过20字符）: \n";
        cin >> user;
    } while (user.length() > 20 || cin.eof());

    do
    {
        cout << "请输入密码（不超过20字符）: \n";
        cin >> password;
    } while (password.length() > 20);

    //写到消息缓冲区
    conn->mw.writeString(&user, 20);
    conn->mw.writeString(&password, 20);

    //重置消息体长度
    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);

    //发送到服务器
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());

    event_add(&stdinev, NULL); //重新注册标准输入以选择业务
}
void show_commodities()
{
    MessageHead mh = {0, CMD_SHOWCOM};
    conn->mw.writeMessageHead(&mh);
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
}

void add_commodities_to_cart()
{
    if (tocken.empty())
    {
        cout << "未登录，请先登录" << endl;
        return;
    }

    event_del(&stdinev); //让出标准输入

    MessageHead mh = {0, CMD_ADDCART};
    conn->mw.writeMessageHead(&mh); //初始化消息头部，后面再计算消息体长度再写入
    conn->mw.writeString(&tocken, TOCKEN_LEN);

    //从stdin读数据
    string str, commidity_id, count;
    int id, c;
    cout << "请输入需要添加的商品ID， 数量， 以空格隔开" << endl;
    while (getline(cin, str))
    {
        if (str.find(" ") == -1)
            continue;
        commidity_id = str.substr(0, str.find(" "));
        count = str.substr(str.find(" ") + 1);

        id = stoi(commidity_id);
        c = stoi(count);
        break;
    }

    //写到消息缓冲区
    conn->mw.writeInt(id, 6);
    conn->mw.writeInt(c, 6);

    //重置消息体长度
    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);

    //发送到服务器
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());

    event_add(&stdinev, NULL); //重新注册标准输入以选择业务
}

void get_cart()
{
    if (tocken.empty())
    {
        cout << "未登录，请先登录" << endl;
        return;
    }

    MessageHead mh = {0, CMD_GETCART};
    conn->mw.writeMessageHead(&mh);
    conn->mw.writeString(&tocken, TOCKEN_LEN);

    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);

    //发送到服务器
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
}

void create_order()
{
    if (tocken.empty())
    {
        cout << "未登录，请先登录" << endl;
        return;
    }

    MessageHead mh = {0, CMD_CREATEODER};
    conn->mw.writeMessageHead(&mh);
    conn->mw.writeString(&tocken, TOCKEN_LEN);

    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);

    //发送到服务器
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
}

void get_order()
{
    if (tocken.empty())
    {
        cout << "未登录，请先登录" << endl;
        return;
    }

    MessageHead mh = {0, CMD_GETORDER};
    conn->mw.writeMessageHead(&mh);
    conn->mw.writeString(&tocken, TOCKEN_LEN);

    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);

    //发送到服务器
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
}

void resp_sign_in()
{
    int stat;
    conn->mr.readInt(&stat, STAT_LEN);
    //cout << "sign in: " << stat <<" len: "<< stat.length()<<"\n";
    if (stat == STAT_OK)
        cout << "注册成功\n ";
    else if (stat == STAT_FAIL)
        cout << "注册失败，请重试\n";
    else if (stat == STAT_EXIST)
        cout << "用户名已存在，请重试\n";
    else
        cout << "未知错误，请重试\n";
}

void resp_login()
{
    int stat;
    conn->mr.readInt(&stat, STAT_LEN);
    if (stat == STAT_OK)
    {
        cout << "登录成功\n ";
        conn->mr.readString(&tocken);
        cout << tocken << "\n";
    }
    else if (stat == STAT_FAIL)
        cout << "登录失败，请重试\n";
    else if (stat == STAT_ERR)
        cout << "用户名或密码错误，请重试\n";
}

void resp_show_commodities()
{
    int stat;
    string com_info;
    conn->mr.readInt(&stat, STAT_LEN);
    if (stat != STAT_OK)
        cout << "服务器忙，稍后再试\n ";
    else
    {
        conn->mr.readString(&com_info);
        cout << com_info << endl;
    }
}

void resp_add_commodities_to_cart()
{
    int stat;
    conn->mr.readInt(&stat, STAT_LEN);
    if (stat == STAT_OK)
    {
        cout << "添加到购物车成功:\n ";
        //get_cart();
    }
    else
    {
        cout << "会话已过期，请重新登录\n ";
    }
}

void resp_get_cart()
{
    int stat;
    conn->mr.readInt(&stat, STAT_LEN);
    if (stat == STAT_OK)
    {
        cout << "您的购物车：\n";
        string cart;
        conn->mr.readString(&cart);
        cout << cart << endl;
    }
    else if (stat == STAT_FAIL)
    {
        cout << "会话已过期，请重新登录\n";
    }
    else
    {
        cout << "购物车空空如也，去浏览商品吧\n";
    }
}

void resp_create_order()
{
    int stat;
    conn->mr.readInt(&stat, STAT_LEN);
    if (stat == STAT_OK)
    {
        cout << "结算成功\n";
    }
    else if (stat == STAT_FAIL)
    {
        cout << "会话已过期，请重新登录\n";
    }
    else if (stat == STAT_VOID)
    {
        cout << "购物车空空如也，去浏览商品吧\n";
    }
    else if (stat == STAT_ERR)
    {
        cout << "服务器发生错误，或库存不足\n";
    }
}
void resp_get_order()
{
    int stat;
    conn->mr.readInt(&stat, STAT_LEN);
    if (stat == STAT_OK)
    {
        cout << "您的订单：\n";
        string order;
        conn->mr.readString(&order);
        cout << order << endl;
    }
    else if (stat == STAT_FAIL)
    {
        cout << "会话已过期，请重新登录\n";
    }
    else if (stat == STAT_VOID)
    {
        cout << "您还没有订单\n";
    }
}

void client_start()
{
    cout << endl;
    cout << "*****************************************" << endl;
    cout << "欢迎使用weBay" << endl;
    cout << "*****************************************" << endl;
    //cout << endl;
    key_tips();
}

void key_tips()
{
    cout << endl;
    cout << "按下：" << endl;
    cout << "[0]：注册\t\t";
    cout << "[1]：登录\t\t";
    cout << "[2]：浏览商品" << endl;
    cout << "[3]：添加商品到购物车\t";
    cout << "[4]：浏览购物车\t\t";
    cout << "[5]：生成订单" << endl;
    cout << "[6]：历史订单\t\t";
    cout << "[c]：首页\t\t";
    cout << "[q]：退出" << endl;
    cout << endl;
}