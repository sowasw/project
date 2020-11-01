#include "weBay.h"

WeBayServer::WeBayServer(EventLoop *loop, short port, const char *conf) : loop_(loop), server_(loop, port), sql_(conf), redis_(conf)
{
    refreshCommodityInfo_ = true;

    if (sql_.isConnectedMysql())
    {
        LOG_INFO("Mysql connected: %s:%d\n", sql_.getHost().c_str(), sql_.getPort());
    }
    if (redis_.getContext())
    {
        LOG_INFO("Redis connected: %s:%d\n", redis_.getHost().c_str(), redis_.getPort());
    }

    //按顺序初始化回调
    cmdCbVec_.push_back(std::bind(&WeBayServer::signIn, this, std::placeholders::_1));
    cmdCbVec_.push_back(std::bind(&WeBayServer::login, this, std::placeholders::_1));
    cmdCbVec_.push_back(std::bind(&WeBayServer::showCommodities, this, std::placeholders::_1));
    cmdCbVec_.push_back(std::bind(&WeBayServer::addCart, this, std::placeholders::_1));
    cmdCbVec_.push_back(std::bind(&WeBayServer::getCart, this, std::placeholders::_1));
    cmdCbVec_.push_back(std::bind(&WeBayServer::createOrder, this, std::placeholders::_1));
    cmdCbVec_.push_back(std::bind(&WeBayServer::getOrder, this, std::placeholders::_1));

    server_.setMessageCallBack(std::bind(&WeBayServer::onMessage, this, std::placeholders::_1));
    server_.setWriteCompleteCallBack(std::bind(&WeBayServer::onSend, this, std::placeholders::_1));

    //注册清理缓存
    struct timeval tv = {2, 10000};

    cleanSessionFd_ = eventfd(0, EFD_CLOEXEC);
    cleanEv_ = event_new(server_.getBase(), cleanSessionFd_, EV_PERSIST | EV_READ, clean_session_cb, (void *)&redis_);
    event_add(cleanEv_, &tv);
}

void WeBayServer::onMessage(ConnPtr &conn)
{
    struct evbuffer *input = bufferevent_get_input(conn->bev);
    size_t n;

    if ((n = evbuffer_get_length(input)) > 0) //缓冲区接收到的数据大小
    {
        size_t nread;

        if (n < MESSAGE_HEAD_BUF_SIZE)
        {
            LOG_ERROR("Read message head error, close the connection\n");
            server_.removeChannel(loop_, conn); //消息长度不足消息头，关闭连接
            return;
        }
        LOG_DEBUG("There are %d bytes in the evbuffer\n", n);

        //读消息头
        bufferevent_read(conn->bev, conn->read_buf, MESSAGE_HEAD_BUF_SIZE);
        conn->mr.readMessageHead(&conn->head);
        if (conn->head.body_size > MESSAGE_MAX_BODY_SIZE || conn->head.body_size < 0)
        {
            LOG_ERROR("Message body too large\n");
            server_.removeChannel(loop_, conn); //消息体长度过不合法，关闭连接
            return;
        }
        LOG_DEBUG("body_size: %ld, cmd: %d\n", conn->head.body_size, conn->head.cmd);

        //读消息体
        nread = bufferevent_read(conn->bev, conn->read_buf + 8, conn->head.body_size);
        if (nread != conn->head.body_size)
        {
            LOG_ERROR("Message body: nread %ld != conn->head.body_size %ld\n", nread, conn->head.body_size);
            server_.removeChannel(loop_, conn); //消息体长度过不合法，关闭连接
            return;
        }

        //以业务号为下标取回调函数
        cmdCbVec_[conn->head.cmd](conn);
        conn->clear();
    }
    else
    {
        LOG_ERROR("error: no data here\n");
        //break;
    }
}

void WeBayServer::onSend(ConnPtr &conn)
{
    struct evbuffer *output = bufferevent_get_output(conn->bev);
    size_t n;
    if ((n = evbuffer_get_length(output)) > 0)
    {
        LOG_ERROR("Write ERROR: output buffer get length %ld\n", n);
        return;
    }
    //else
    LOG_INFO("onSend!\n");
}

void WeBayServer::signIn(ConnPtr &conn)
{
    MessageHead mh = {0, CMD_SIGN};
    conn->mw.writeMessageHead(&mh); //初始化消息头部，后面再计算消息体长度再写入

    //读取消息体
    std::string user, password, email;
    if (!conn->mr.readString(&user, 20))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("signIn: read user error\n");
    }
    if (!conn->mr.readString(&password, 20))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("signIn: read password error\n");
    }
    if (!conn->mr.readString(&email, 36))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("signIn: read password error\n");
    }

    //执行注册并写入执行状态
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "select user_id from webay_user where name = '%s'", user.c_str());
    if (sql_.queryAndStoreRes(buf))
    {
        if (sql_.resToString() != "") //用户已存在
        {
            LOG_INFO("signIn: user %s already exists\n", user.c_str());
            conn->mw.writeInt(STAT_EXIST, STAT_LEN);
        }
        else //开始注册
        {
            char buf2[1024];
            memset(buf2, 0, sizeof(buf2));
            snprintf(buf2, sizeof(buf2), "insert into webay_user values(NULL, '%s', '%s', '%s')", user.c_str(), password.c_str(), email.c_str());
            if (sql_.queryAndStoreRes(buf2))
            {
                conn->mw.writeInt(STAT_OK, STAT_LEN);
                LOG_INFO("signIn: user %s sign in OK\n", user.c_str());
            }
            else
            {
                conn->mw.writeInt(STAT_FAIL, STAT_LEN);
                LOG_ERROR("signIn: user %s sign in FAILED\n", user.c_str());
            }
        }
    }
    else //查询发生错误
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("signIn: query error\n");
    }

    //重写消息体长度
    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4); //消息头只用了四个字符来表示消息体长度

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("signIn: write %ld bytes\n", conn->mw.getLen());
}

void WeBayServer::login(ConnPtr &conn)
{
    //初始化消息头部，后面再计算消息体长度再写入
    initMessageHead(conn, CMD_LOGIN);

    //读取消息体
    std::string host, userId, user, password, session, tocken;
    if (!conn->mr.readString(&user, 20))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("login: read user error\n");
    }
    if (!conn->mr.readString(&password, 20))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("login: read password error\n");
    }

    LOG_INFO("login: host %s, port %d \n", conn->host.c_str(), conn->port);

    //登录
    char buf[256];
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "select user_id from webay_user where name = '%s' and user_password = '%s'", user.c_str(), password.c_str());
    if (sql_.queryAndStoreRes(buf))
    {
        auto ptr = sql_.getResPtr(); //wx_mysql.h

        //用户 或密码错误
        if (ptr->empty())
        {
            LOG_ERROR("login: user %s error or password error\n", user.c_str());
            conn->mw.writeInt(STAT_ERR, STAT_LEN);
            goto end;
        }
        else //添加登录缓存
        {
            userId = *ptr->at(0)->at(0);
            //session = userId + ":" + conn->host;
            time_t timestamp = time(NULL);
            Session ses(conn, userId, timestamp);
            tocken = Tocken()(ses);
            redis_.cmdArgs("hset", "login", tocken, user); //添加登录缓存

            //记录最近登录时间戳
            updateTocken(conn, tocken, user);

            conn->mw.writeInt(STAT_OK, STAT_LEN);
            conn->mw.writeString(&tocken, TOCKEN_LEN);
            LOG_INFO("login: user %s, ID: %s login OK\n", user.c_str(), userId.c_str());
        }
    }
    else //mysql查询发生错误
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_ERROR("login: query error\n");
    }

end:
    //重写消息体长度
    resetMessageBodyLength(conn);

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("login: write %ld bytes\n", conn->mw.getLen());
}

void WeBayServer::showCommodities(ConnPtr &conn)
{
    initMessageHead(conn, CMD_SHOWCOM);

    std::string info;
    bool getInfo = false;

    //刷新商品信息
    if (refreshCommodityInfo_)
    {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "select * from webay_commodity");
        if (sql_.queryAndStoreRes(buf))
        {
            info = sql_.resToString();
            if (info != "")
            {
                redis_.cmdArgs("set", "commodity_info", info); //使用字符串缓存商品信息
                refreshCommodityInfo_ = false;
                getInfo = true;
            }
            else
            {
                conn->mw.writeInt(STAT_FAIL, STAT_LEN);
                goto end;
            }
        }
    }

    if (!getInfo)
    {
        redis_.cmdArgs("get", "commodity_info");
        info = redis_.resToString();
        if (info == "")
        {
            conn->mw.writeInt(STAT_FAIL, STAT_LEN);
            goto end;
        }
    }

    conn->mw.writeInt(STAT_OK, STAT_LEN);
    conn->mw.writeString(&info, (size_t)info.length());

end:
    //重写消息体长度
    resetMessageBodyLength(conn);

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("showCommodities: write %ld bytes\n", conn->mw.getLen());
}

void WeBayServer::addCart(ConnPtr &conn)
{
    initMessageHead(conn, CMD_ADDCART);

    std::string tocken, info, item, commodity_name, user;
    int commodity_id, count;
    int pos, endpos;

    //检查tocken， 用户是否登录
    if (!checkTocken(conn, tocken, user))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_INFO("addCart: a not log in user or expired session: %s\n", user.c_str());
        goto end;
    }

    updateTocken(conn, tocken, user);

    conn->mr.readInt(&commodity_id, 6);
    conn->mr.readInt(&count, 6);

    //找到单条商品名字
    redis_.cmdArgs("get", "commodity_info");
    info = redis_.resToString();
    pos = info.find(std::to_string(commodity_id));
    for (int i = pos; i < info.length(); i++)
    {
        bool found = false;
        if (std::isalpha(info[i]))
        {
            int j = i;
            while (info[j] != '|')
            {
                item += info[j];
                j++;
            }
            found = true;
        }
        if (found)
            break;
    }

    //去除商品名后面的空格
    pos = item.length() - 1;
    if (item[pos] == ' ')
    {
        while (item[pos] == ' ')
            --pos;
        item = item.substr(0, pos + 1);
    }

    //添加购物车缓存
    //以字符串"cart"接令牌作购物车键名
    if (count <= 0)
        redis_.cmdArgs("hrem", "cart:" + tocken, item);
    else
        redis_.cmdArgs("hset", "cart:" + tocken, item, count);

    conn->mw.writeInt(STAT_OK, STAT_LEN);
    LOG_INFO("addCart: ok, user: %s\n", user.c_str());

end:
    //重写消息体长度
    resetMessageBodyLength(conn);

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("addCart: write %ld bytes\n", conn->mw.getLen());
}

void WeBayServer::getCart(ConnPtr &conn)
{
    initMessageHead(conn, CMD_GETCART);

    std::string tocken, user;
    int commodity_id, count;
    Redis::CmdResPtr cart_ptr;

    //检查tocken， 用户是否登录
    if (!checkTocken(conn, tocken, user))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_INFO("getCart: not log in or expired session: %s\n", user.c_str());
        goto end;
    }

    //更新最近登录时间
    updateTocken(conn, tocken, user);

    //获取用户购物车
    redis_.cmdArgs("hgetall", "cart:" + tocken);
    cart_ptr = redis_.getCmdResPtr();

    if (cart_ptr->empty())
    {
        conn->mw.writeInt(STAT_VOID, STAT_LEN);
        LOG_INFO("getCart: no commodity in cart: %s\n", user.c_str());
        goto end;
    }

    conn->mw.writeInt(STAT_OK, STAT_LEN);
    for (int i = 0; i < cart_ptr->size(); i += 2)
    {
        //auto res_ptr = cart_ptr->at(i);
        //for (int j = 0; j < res_ptr->size(); j += 2)
        {
            conn->mw.writeString(cart_ptr->at(i).get());
            conn->mw.writeCString(" ");
            conn->mw.writeString(cart_ptr->at(i + 1).get());
            conn->mw.writeCString("\n");
        }
    }

end:
    //重写消息体长度
    resetMessageBodyLength(conn);

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("getCart: write %ld bytes\n", conn->mw.getLen());
}

void WeBayServer::createOrder(ConnPtr &conn)
{
    initMessageHead(conn, CMD_CREATEODER);

    std::string tocken, user;
    int commodity_id, quantity, count;
    Redis::CmdResPtr cart_ptr;

    //购物车商品名+数量的组合
    std::vector<std::shared_ptr<std::pair<std::shared_ptr<std::string>, std::shared_ptr<std::string>>>> items;

    //检查tocken， 用户是否登录
    if (!checkTocken(conn, tocken, user))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_INFO("createOrder: not log in or expired session: %s\n", user.c_str());
        goto end;
    }

    //更新最近登录时间
    updateTocken(conn, tocken, user);

    redis_.cmdArgs("hgetall", "cart:" + tocken);
    cart_ptr = redis_.getCmdResPtr();

    //检查购物车
    if (cart_ptr->empty())
    {
        conn->mw.writeInt(STAT_VOID, STAT_LEN);
        LOG_INFO("createOrder: no commodity in cart: %s\n", user.c_str());
        goto end;
    }

    //开启mysql事务
    if (!sql_.queryAndStoreRes("START TRANSACTION"))
    {
        conn->mw.writeInt(STAT_ERR, STAT_LEN);
        goto end;
    }

    //生成简要订单，ID + 时间
    char order[256];
    memset(order, 0, sizeof(order));
    snprintf(order, sizeof(order), "insert into webay_order values(NULL, (select user_id from webay_user where name = '%s'), NOW())", user.c_str());
    if (!sql_.queryAndStoreRes(order))
        goto fail;

    //添加购物车商品到待结算的vector
    for (int i = 0; i < cart_ptr->size(); i += 2)
    {
        auto ptr = std::make_shared<std::pair<std::shared_ptr<std::string>, std::shared_ptr<std::string>>>(cart_ptr->at(i), cart_ptr->at(i + 1));
        items.push_back(ptr);
    }

    for (int i = 0; i < items.size(); i++)
    {
        char query[256];
        memset(query, 0, sizeof(query));
        snprintf(query, sizeof(query), "select commodity_id, quantity from webay_commodity where name = '%s'", items[i]->first->c_str());

        if (!sql_.queryAndStoreRes(query))
            goto fail;

        auto ptr = sql_.getResPtr();

        if (ptr->empty()) //商品不存在
        {
            LOG_ERROR("createOrder: commodity not exists\n");
            conn->mw.writeInt(STAT_ERR, STAT_LEN);
            goto fail;
        }
        else
        {
            count = atoi(items[i]->second->c_str());
            quantity = atoi(ptr->at(0)->at(1)->c_str());
            commodity_id = atoi(ptr->at(0)->at(0)->c_str());

            //需要用到的sql语句
            char form[1024];
            char order_id[256];
            char user_id[256];
            char price[256];
            memset(order_id, 0, sizeof(order_id));
            memset(user_id, 0, sizeof(user_id));
            snprintf(order_id, sizeof(order_id), "select order_id from webay_order order by order_id desc limit 1");
            snprintf(user_id, sizeof(user_id), "select user_id from webay_user where name = '%s'", user.c_str());

            if (count > quantity) //库存不足
            {
                LOG_ERROR("createOrder: %s understock\n", items[i]->first->c_str());
                conn->mw.writeInt(STAT_ERR, STAT_LEN);
                goto fail;
            }
            else //结算订单，正式应用中应该需要增加金额结算逻辑
            {
                //更新库存
                char update[256];
                memset(update, 0, sizeof(update));
                snprintf(update, sizeof(update), "update webay_commodity set quantity = %d where name = '%s'", quantity - count, items[i]->first->c_str());
                if (!sql_.queryAndStoreRes(update))
                    goto fail;

                //生成详细订单
                memset(form, 0, sizeof(form));
                memset(price, 0, sizeof(price));
                snprintf(price, sizeof(price), "select price from webay_commodity where commodity_id = %d", commodity_id);
                //用子查询插入数据
                snprintf(form, sizeof(form), "insert into webay_order_form values((%s), %d, %d, (%s) * %d)", order_id, commodity_id, count, price, count);
                if (!sql_.queryAndStoreRes(form))
                    goto fail;
            }
        }
    }

    //提交事务
    if (sql_.queryAndStoreRes("COMMIT"))
    {
        refreshCommodityInfo_ = true; //刷新商品信息的标志

        //清除购物车缓存
        for (int i = 0; i < items.size(); i++)
            redis_.cmdArgs("hdel", "cart:" + tocken, items[i]->first->c_str());

        //刷新订单缓存
        std::string order_form;
        refeshOrderCache(order_form, tocken, user); //刷新历史订单缓存

        conn->mw.writeInt(STAT_OK, STAT_LEN);

        LOG_INFO("createOrder: mysql transaction OK\n");
        goto end;
    }

//事务失败跳转到这里
fail:
    if (!sql_.queryAndStoreRes("ROLLBACK"))
    {
        conn->mw.writeInt(STAT_ERR, STAT_LEN);
        //reconnect mysql;
        goto end;
    }
    conn->mw.writeInt(STAT_ERR, STAT_LEN);
    LOG_ERROR("createOrder: mysql transaction failed\n");

end:
    //重写消息体长度
    resetMessageBodyLength(conn);

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("createOrder: write %ld bytes\n", conn->mw.getLen());
}

void WeBayServer::getOrder(ConnPtr &conn)
{
    initMessageHead(conn, CMD_GETORDER);

    std::string tocken, order_form, user;
    int commodity_id, count;
    Redis::CmdResPtr order_ptr;
    bool found = false;

    //检查tocken， 用户是否登录
    if (!checkTocken(conn, tocken, user))
    {
        conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        LOG_INFO("createOrder: not log in or expired session: %s\n", user.c_str());
        goto end;
    }

    //更新最近登录时间
    updateTocken(conn, tocken, user);

    //从缓存查找订单
    redis_.cmdArgs("get", "order:" + tocken);
    order_ptr = redis_.getCmdResPtr();
    if (order_ptr->empty())
    {
        refeshOrderCache(order_form, tocken, user); //刷新历史订单缓存
        if (!order_form.empty())                    //数据库中找到订单
            goto ok;
        else
        {
            conn->mw.writeInt(STAT_VOID, STAT_LEN);
            goto end;
        }
    }
    else
    {
        found = true;
    }

    if (found) //缓存中找到订单
        order_form.assign(order_ptr->at(0)->c_str());

ok:
    conn->mw.writeInt(STAT_OK, STAT_LEN);
    conn->mw.writeString(&order_form);

end:
    //重写消息体长度
    resetMessageBodyLength(conn);

    //发送响应
    bufferevent_write(conn->bev, conn->write_buf, conn->mw.getLen());
    LOG_INFO("getOrder: write %ld bytes\n", conn->mw.getLen());
}

bool WeBayServer::refeshOrderCache(std::string &order, std::string &tocken, std::string &user)
{
    //从数据库查找订单
    char query[1024];
    memset(query, 0, sizeof(query));
    snprintf(query, sizeof(query), "select webay_order.order_id, webay_commodity.name as goods, webay_commodity.price, \
                                            webay_order_form.quantity as count, webay_order_form.sum_of_money as 'total cost', \
                                            webay_order.date_time as date\
                                        from webay_user inner join webay_commodity inner join webay_order inner join webay_order_form \
                                        on webay_user.user_id = webay_order.user_id and webay_order_form.order_id = webay_order.order_id \
                                            and webay_order_form.commodity_id = webay_commodity.commodity_id \
                                        where webay_user.name = '%s'",
             user.c_str());

    sql_.queryAndStoreRes(query);

    //添加订单缓存，用字符串订单缓存，可以优化成hash，连同前面的商品信息一起
    order = sql_.resToString();
    redis_.cmdArgs("set", "order:" + tocken, order);

    return !order.empty();
}

void WeBayServer::initMessageHead(ConnPtr &conn, int cmd)
{
    MessageHead mh = {0, cmd};
    conn->mw.writeMessageHead(&mh);
}

void WeBayServer::resetMessageBodyLength(ConnPtr &conn)
{
    size_t bodylen = conn->mw.getLen() - MESSAGE_HEAD_BUF_SIZE;
    conn->mw.writeBodyLen(bodylen, 4);
}

bool WeBayServer::checkTocken(ConnPtr &conn, std::string &tocken, std::string &user)
{
    conn->mr.readString(&tocken, TOCKEN_LEN);
    redis_.cmdArgs("hget", "login", tocken);
    Redis::CmdResPtr ptr = redis_.getCmdResPtr();
    if (ptr->empty())
    {
        //conn->mw.writeInt(STAT_FAIL, STAT_LEN);
        return false;
    }

    user.assign(ptr->at(0)->c_str());
    return true;
}
void WeBayServer::updateTocken(ConnPtr &conn, std::string &tocken, std::string &user)
{
    time_t timestamp = time(NULL);
    redis_.cmdArgs("zadd", "recent", timestamp, tocken);
}

void WeBayServer::clean_session_cb(int fd, short event, void *argc)
{
    Redis *conn = static_cast<Redis *>(argc);

    //清理到期会话
    time_t now = time(NULL);
    time_t expir = now - SESSION_MAX_ALIVE_SECOND;
    conn->cmdArgs("zrangebyscore", "recent", 0, expir);
    auto ptr = conn->getCmdResPtr();
    if (!ptr->empty())
    {
        for (int i = 0; i < ptr->size(); i++)
        {
            conn->cmdArgs("hdel", "login", ptr->at(i)->c_str());  //清理到期登录缓存
            conn->cmdArgs("zrem", "recent", ptr->at(i)->c_str()); //清理到期令牌
            conn->cmdArgs("hdel", "cart:" + *ptr->at(i));         //清理会话购物车
            conn->cmdArgs("del", "order:" + *ptr->at(i));         //清理会话订单

            //printf("s1: %s %ld\n", ptr->at(i)->c_str(), ptr->size());
        }

        LOG_INFO("cleaned: %ld expired sessions\n", ptr->size());
    }

    //清理超过数量限制的会话
    conn->cmdArgs("zcard", "recent");
    ptr = conn->getCmdResPtr();
    if (ptr->empty())
        return;
    int size = atoi(ptr->at(0)->c_str());
    if (size <= SESSION_LIMIT)
        return;

    int end_ind = size - SESSION_LIMIT;
    conn->cmdArgs("zrange", "recent", 0, end_ind - 1);
    ptr = conn->getCmdResPtr();

    printf("size: %d ptr->size: %ld\n", end_ind, ptr->size());
    //auto keys_ptr;
    for (int i = 0; i < ptr->size(); i++)
    {
        conn->cmdArgs("hdel", "login", ptr->at(i)->c_str());  //清理超限登录缓存
        conn->cmdArgs("zrem", "recent", ptr->at(i)->c_str()); //清理超限令牌
        conn->cmdArgs("hdel", "cart:" + *ptr->at(i));         //清理会话购物车
        conn->cmdArgs("del", "order:" + *ptr->at(i));         //清理会话订单

        //printf("s2: %s %ld\n", ptr->at(i)->c_str(), ptr->size());
    }

    LOG_INFO("cleaned: %ld over limited sessions\n", ptr->size());
}

std::string Tocken::operator()(Session &ses)
{
    std::ostringstream os;
    os << ses.id_ << ":" << ses.host_ << ":";
    os << std::setfill('*') << std::left << std::setw(TOCKEN_LEN) << ses.loginTime_;
    return os.str().substr(0, TOCKEN_LEN);
}