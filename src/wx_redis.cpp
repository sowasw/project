#include "wx_redis.h"

using std::string;

int Redis::rand_pid = 0;

static void redisPrintReply(redisReply *reply);

Redis::Redis() : host(string("127.0.0.1")), port(6379), reply(NULL), nreply(0), replys(NULL)
{
    conn = redisConnect(host.c_str(), port);
    if (conn->err)
    {
        fprintf(stderr, "Redis construct failed: connection error: %s\n", conn->errstr);
        endConnection();
    }
}

Redis::Redis(const char *host, const int port) : reply(NULL), nreply(0), replys(NULL)
{
    this->host = string(host);
    this->port = port;
    conn = redisConnect(this->host.c_str(), this->port);
    if (conn->err)
    {
        fprintf(stderr, "Redis construct failed: connection error: %s\n", conn->errstr);
        endConnection();
    }
}

Redis::Redis(const char *conf) : reply(NULL), nreply(0), replys(NULL)
{
    std::ifstream is;
    std::string str(""), host("");
    int port = 0;

    is.open(conf);

    while (std::getline(is, str) && !is.eof())
    {
        if (str[0] == '#')
            continue;
        if (str.find("REDIS_HOST") != std::string::npos)
        {
            host = str.substr(str.find("REDIS_HOST") + strlen("REDIS_HOST="));
            continue;
        }

        else if (str.find("REDIS_PORT") != std::string::npos)
        {
            port = atoi(str.substr(str.find("REDIS_PORT") + strlen("REDIS_PORT="), str.length()).c_str());
            continue;
        }
    }

    this->host = host;
    this->port = port;
    conn = redisConnect(this->host.c_str(), this->port);
    if (conn->err)
    {
        fprintf(stderr, "Redis construct failed: connection error: %s\n", conn->errstr);
        fprintf(stderr, "Redis %s:%d\n", this->host.c_str(), this->port);
        endConnection();
    }
}

Redis::~Redis()
{
    endConnection();
}

void Redis::endConnection()
{
    if (reply != NULL)
    {
        freeReplyObject(reply);
        reply = NULL;
    }

    freeReplys();

    if (conn != NULL)
    {
        redisFree(conn);
        conn = NULL;
    }
}

void Redis::reconnectWithOptions(const redisOptions *options)
{
    if (conn != NULL)
        endConnection();
    conn = redisConnectWithOptions(options);
    if (conn->err)
    {
        fprintf(stderr, "Redis construct failed: connection error: %s\n", conn->errstr);
        endConnection();
    }
}

bool Redis::cmd(const std::string cmd)
{

    if (reply != NULL)
        freeReplyObject(reply);
    reply = (redisReply *)redisCommand(conn, cmd.c_str());

    return (reply != NULL && reply->type != REDIS_REPLY_ERROR);
}

bool Redis::cmd(const char *cmd)
{
    if (reply != NULL)
        freeReplyObject(reply);
    reply = (redisReply *)redisCommand(conn, cmd);

    return (reply != NULL && reply->type != REDIS_REPLY_ERROR);
}

bool Redis::cmd(const char *format, ...)
{
    if (reply != NULL)
        freeReplyObject(reply);

    va_list vl;
    va_start(vl, format);
    size_t newsize = vsnprintf(NULL, 0, format, vl) + 1; //返回格式化后的长度, +1 for '\0'
    va_end(vl);

    char cmdFormated[newsize];

    //格式化
    va_list vl2;
    va_start(vl2, format);
    //vsnprintf((char *)cmdFormated.c_str(), newsize, format, vl2);
    vsnprintf(cmdFormated, newsize, format, vl2);
    va_end(vl2);

    //printf("cmdFormated: %s\n", cmdFormated);
    reply = (redisReply *)redisCommand(conn, cmdFormated);

    return (reply != NULL && reply->type != REDIS_REPLY_ERROR);
}

bool Redis::pipelineExec()
{
    freeReplys();

    replys = new redisReply *[nreply];

    for (int i = 0; i < nreply; i++)
    {
        redisGetReply(conn, (void **)replys + i);
        if (replys[i] == NULL && replys[i]->type == REDIS_REPLY_ERROR)
            return false;
    }

    return true;
}

void Redis::pipelineClear()
{
    freeReplys();
    nreply = 0;
}

void Redis::freeReplys()
{
    if (replys != NULL)
    {
        for (int i = 0; i < nreply; i++)
        {
            if (replys[i] != NULL)
                freeReplyObject(replys[i]);
        }

        delete[] replys;
    }

    replys = NULL;
}

bool Redis::scriptLua(const char *script)
{
    return cmdArgs("EVAL", script, 0);
}

bool Redis::lock(const char *resource, const int timeout)
{
    //std::cout << timeout << "\n";
    redisReply *lockreply;
    std::ostringstream scriptos;

    if (rand_pid == 0)
    {
        srand(getpid());
        rand_pid = rand() % 789456321 + 1000000000;
    }

    //char script[1024];
    //snprintf(script, sizeof(script), "return redis.call('set', KEYS[1], '%d', 'ex', '%d', 'nx')", rand_pid, timeout);

    scriptos << "return redis.call('set', KEYS[1], '" << rand_pid << "', 'ex', '" << timeout << "', 'nx')";
    string scr = scriptos.str();
    //std::cout << scriptos.str().c_str() << "\n";

    const char *cmdargv[] = {
        "EVAL",      //执行脚本的命令
        scr.c_str(), //Lua脚本
        //script,
        "1",     //脚本键名参数个数
        resource //KEYS[1],lua数组下标从1开始
    };

    int narg = sizeof(cmdargv) / sizeof(char *);
    size_t cmdargvlen[narg];
    for (size_t i = 0; i < narg; i++)
        cmdargvlen[i] = strlen(cmdargv[i]);

    while (1)
    {
#ifdef REDIS_LOCK_DEBUG
        ++ntry;
#endif
        lockreply = (redisReply *)redisCommandArgv(conn, narg, cmdargv, cmdargvlen);

        //printf("reply type: %d, integer: %lld, dval: %f, str_len: %ld, str: %s, elements: %ld\n", \
               lockreply->type, lockreply->integer, lockreply->dval, lockreply->len, lockreply->str, lockreply->elements);

        if (lockreply != NULL)
        {
            if (lockreply->type == REDIS_REPLY_NIL)
            {
                wx_sleep((rand() / (double)RAND_MAX) * 0.001);
                continue;

                //freeReplyObject(lockreply);
                //return false;
            }
            else if ((lockreply->type == REDIS_REPLY_STATUS) && (strcmp("OK", lockreply->str) == 0))
            {
                freeReplyObject(lockreply);
                return true;
            }
            else
            {
                freeReplyObject(lockreply);
                return false;
            }
        }
        else
            return false;
    }
}

bool Redis::unlock(const char *resource)
{
    redisReply *lockreply;
    int stat = 0;
    std::ostringstream scriptos;

    if (rand_pid == 0)
    {
        srand(getpid());
        rand_pid = rand() % 789456321 + 1000000000;
    }

    //char script[1024];
    //snprintf(script, sizeof(script), "if redis.call('get', KEYS[1]) == '%d' then return redis.call('del', KEYS[1]) else return true end", rand_pid);

    scriptos << "if redis.call('get', KEYS[1]) == '" << rand_pid << "' then return redis.call('del', KEYS[1]) else return true end";
    string scr = scriptos.str();

    const char *cmdargv[] = {
        "EVAL",
        scr.c_str(),
        //script,//ok
        "1",
        resource //key[1]
    };

    int narg = sizeof(cmdargv) / sizeof(char *);
    size_t cmdargvlen[narg];
    for (size_t i = 0; i < narg; i++)
        cmdargvlen[i] = strlen(cmdargv[i]);

    lockreply = (redisReply *)redisCommandArgv(conn, narg, cmdargv, cmdargvlen);

    //printf("reply type: %d, integer: %lld, dval: %f, str_len: %ld, str: %s, elements: %ld\n", \
          lockreply->type, lockreply->integer, lockreply->dval, lockreply->len, lockreply->str, lockreply->elements);

    stat = lockreply->integer;
    freeReplyObject(lockreply);

    return (bool)stat;
}

std::string Redis::resToString()
{
    std::ostringstream os;
    if (reply == NULL)
    {
        os << "";
        return os.str();
    }

    redisReply *r = reply;
    switch (r->type)
    {
    case REDIS_REPLY_STRING:
        os << r->str;
        break;

    case REDIS_REPLY_INTEGER:
        os << r->integer;
        break;

    case REDIS_REPLY_DOUBLE:
        os << r->dval;
        break;

    default:
        os << "";
        break;
    }
    return os.str();
}

Redis::CmdResPtr Redis::getCmdResPtr()
{
    Redis::CmdResPtr crp = std::make_shared<std::vector<std::shared_ptr<std::string>>>();
    getCmdResPtr(crp, reply);
    return crp;
}
Redis::PipeResPtr Redis::getPipeResPtr()
{
    Redis::PipeResPtr prp = std::make_shared<std::vector<CmdResPtr>>();
    prp->reserve(nreply);
    for (size_t i = 0; i < nreply; i++)
    {
        Redis::CmdResPtr crp = std::make_shared<std::vector<std::shared_ptr<std::string>>>();
        getCmdResPtr(crp, replys[i]);
        prp->push_back(crp);
    }
    return prp;
}

void Redis::getCmdResPtr(CmdResPtr &crp, redisReply *r)
{
    if (r == NULL)
        return;

    auto str_shp = std::make_shared<std::string>();

    switch (r->type)
    {
    case REDIS_REPLY_STRING:
        str_shp->assign(r->str, r->len);
        crp->push_back(str_shp);
        break;

    //set, hashset, zset
    case REDIS_REPLY_ARRAY:
        for (size_t i = 0; i < r->elements; i++)
        {
            getCmdResPtr(crp, r->element[i]);
        }

        break;

        
    case REDIS_REPLY_INTEGER:
        str_shp->assign(std::to_string(r->integer));
        crp->push_back(str_shp);
        break;

    case REDIS_REPLY_DOUBLE:
        str_shp->assign(std::to_string(r->dval));
        crp->push_back(str_shp);
        break;
    

    default:
        break;
    }
}

std::string Redis::stringCmdReply()
{
    std::ostringstream os;
    stringCmdReply(reply, os);
    return os.str();
}

std::string Redis::stringPipelineReplys(bool multi, bool exec)
{
    std::ostringstream os;
    if (replys == NULL)
    {
        os << "nil"
           << "\n";
        return os.str();
    }

    int m = 0;      //start, replys[]
    int n = nreply; //finish, replys[]

    if (multi)
    {
        m += 1;
    }

    if (exec)
    {
        n -= 1;
    }

    for (int i = m; i < n; i++)
        stringCmdReply(replys[i], os);
    return os.str();
}

std::ostringstream &Redis::stringCmdReply(redisReply *r, std::ostringstream &os)
{
    if (r == NULL)
    {
        os << "nil\n";
        return os;
    }

    switch (r->type)
    {
    case REDIS_REPLY_STRING:
        os << "\"" << r->str << "\"\n";
        break;

    //set, hashset, zset
    case REDIS_REPLY_ARRAY:
        for (size_t i = 0; i < r->elements; i++)
        {
            stringCmdReply(r->element[i], os);
        }

        break;

    case REDIS_REPLY_INTEGER:
        os << "integer: " << r->integer << "\n";
        break;

    case REDIS_REPLY_NIL:
        os << "(nil)\n";
        break;

    case REDIS_REPLY_STATUS:
        os << r->str << "\n";
        break;

    case REDIS_REPLY_ERROR:
        os << r->str << "\n";
        break;

    case REDIS_REPLY_DOUBLE:
        os << "dval: " << r->dval << "\n";
        break;

    default:
        printf("reply type: %d, integer: %lld, dval: %f, str_len: %ld, str: %s, elements: %ld\n",
               r->type, r->integer, r->dval, r->len, r->str, r->elements);
        os << "others\n";
        break;
    }
    return os;
}

void Redis::printReply() const
{
    redisPrintReply(reply);
}

void Redis::printReplys() const
{
    if (nreply <= 0 || replys == NULL)
    {
        fprintf(stderr, "replys null.\n");
        return;
    }

    for (int i = 0; i < nreply; i++)
    {
        //redisGetReply(conn, (void **)replys + i);

        redisPrintReply(replys[i]);
        //freeReplyObject(replys[i]);
    }
}

//本文件开始的函数
void redisPrintReply(redisReply *reply)
{
    if (reply == NULL)
    {
        fprintf(stderr, "reply null.\n");
    }
    else
    {
        switch (reply->type)
        {
        case REDIS_REPLY_STRING:
            printf("\"%s\"\n", reply->str);
            //printf("total: %ld bytes\n", reply->len);
            break;

        //set, hashset, zset
        case REDIS_REPLY_ARRAY:
            printf("array: %ld elements\n", reply->elements);
            for (size_t i = 0; i < reply->elements; i++)
            {
                //fprintf(stderr, "reply->element.\n");
                printf("%ld) ", i + 1);
                redisPrintReply(reply->element[i]);
            }

            break;

        case REDIS_REPLY_INTEGER:
            printf("integer: %lld\n", reply->integer);
            break;

        case REDIS_REPLY_NIL:
            printf("(nil)\n");
            break;

        case REDIS_REPLY_STATUS:
            printf("%s\n", reply->str);
            break;

        case REDIS_REPLY_ERROR:
            printf("error: %s\n", reply->str);
            break;

        case REDIS_REPLY_DOUBLE:
            printf("dval: %f \nstr: \"%s\"\n", reply->dval, reply->str);
            break;

        default:
            printf("reply type: %d, integer: %lld, dval: %f, str_len: %ld, str: %s, elements: %ld\n",
                   reply->type, reply->integer, reply->dval, reply->len, reply->str, reply->elements);
            break;
        }
    }
}
