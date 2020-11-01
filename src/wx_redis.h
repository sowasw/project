#ifndef _WX_REDIS_
#define _WX_REDIS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/eventfd.h>
#include <hiredis/hiredis.h>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>

#include "wx_lib.h"

//#define REDIS_LOCK_DEBUG 1

#ifdef REDIS_LOCK_DEBUG
extern int ntry;
#endif

class Redis
{
public:
    typedef std::shared_ptr<std::vector<std::shared_ptr<std::string>>> CmdResPtr; //单个命令的返回结果
    typedef std::shared_ptr<std::vector<CmdResPtr>> PipeResPtr;                //pipeline返回结果

    Redis();
    Redis(const char *host, const int port);
    Redis(const char *conf);
    ~Redis();
    void endConnection();

    //重新连接
    void reconnectWithOptions(const redisOptions *options);

    redisContext *getContext() const { return conn; }
    std::string getHost() { return host; }
    int getPort() { return port; }

    //返回执行命令后的结果集， 再次执行命令会清空之前的结果集
    redisReply *getReply() const { return reply; }
    redisReply **getReplys() const { return replys; }

    //执行命令， 用空格分隔的形式， 如cmd("set foo 1024")
    bool cmd(const std::string cmd);
    bool cmd(const char *cmd);
    bool cmd(const char *format, ...);

    //执行命令，参数可变， 如cmdArgs("hset", "hash", "foo", 2569);
    template <typename T>
    bool cmdArgs(const T t); //给最后一个参数使用,递归终点

    template <typename T, typename... Arg>
    bool cmdArgs(const T t, const Arg... a);

    //pipeline, 追加命令， 如
    //pipelineAddCmd("hget", "hash", "foo");
    //pipelineAddCmd("get", "foo");
    template <typename T>
    void pipelineAddCmd(const T t); //给最后一个参数使用,递归终点

    template <typename T, typename... Arg>
    void pipelineAddCmd(const T t, const Arg... a);

    //pipeline返回到结果集replys， 再次追加命令到pipeline会清空结果集
    bool pipelineExec();
    void pipelineClear();
    void freeReplys();

    //执行简单lua脚本
    bool scriptLua(const char *script); //"EVAL", no KEYS or ARGS

    //分布式锁
    bool lock(const char *resource, const int timeout = 10); //timeout 不能为0
    bool unlock(const char *resource);

    //对于只返回一个结果的命令，返回结果的字符串形式
    std::string resToString();
    CmdResPtr getCmdResPtr();
    PipeResPtr getPipeResPtr();
    void getCmdResPtr(CmdResPtr &crp, redisReply *r);

    //返回结果集的字符串（适用于打印）
    std::string stringCmdReply();
    std::string stringPipelineReplys(bool multi = 0, bool exec = 0);               //根据标志，是否要去掉multi，exec两个命令（如果pipeline里面用了这两个命令）返回的字符串
    std::ostringstream &stringCmdReply(redisReply *reply, std::ostringstream &os); //为之前两个函数所调用

    //把结果集打印到stdout
    void printReply() const;
    void printReplys() const;

private:
    std::string host;
    int port;
    redisContext *conn;
    redisReply *reply;

    //for pipeline
    int nreply;
    redisReply **replys;

    std::string command;
    std::ostringstream cmdStream;    //for cmdArgs
    std::vector<std::string> argVec; //for cmdArgs

    static int rand_pid;
};

template <typename T>
bool Redis::cmdArgs(const T t)
{
    cmdStream << t;
    std::string str = cmdStream.str();
    argVec.push_back(str);

    //用参数列表构造redisCommandArgv参数
    int narg = argVec.size();
    const char *cmdargv[narg];
    size_t cmdargvlen[narg];

    for (size_t i = 0; i < narg; i++)
    {
        cmdargv[i] = argVec[i].c_str();
        cmdargvlen[i] = argVec[i].length();
    }

    if (reply != NULL)
        freeReplyObject(reply);
    reply = (redisReply *)redisCommandArgv(conn, narg, cmdargv, cmdargvlen);

    cmdStream.clear(); //只清除标志
    cmdStream.str(""); //设置新缓冲区*/
    argVec.clear();

    return (reply != NULL && reply->type != REDIS_REPLY_ERROR);
}

template <typename T, typename... Arg>
bool Redis::cmdArgs(const T t, const Arg... a)
{
    //用vector保存参数列表
    cmdStream << t;
    std::string str = cmdStream.str();
    argVec.push_back(str);

    cmdStream.clear(); //只清除标志
    cmdStream.str(""); //设置新缓冲区*/

    cmdArgs(a...);
}

template <typename T>
void Redis::pipelineAddCmd(const T t)
{
    cmdStream << t;
    std::string str = cmdStream.str();
    argVec.push_back(str);

    //用参数列表构造redisCommandArgv参数
    int narg = argVec.size();
    const char *cmdargv[narg];
    size_t cmdargvlen[narg];

    for (size_t i = 0; i < narg; i++)
    {
        cmdargv[i] = argVec[i].c_str();
        cmdargvlen[i] = argVec[i].length();
    }

    //if (reply != NULL)
    //   freeReplyObject(reply);
    redisAppendCommandArgv(conn, narg, cmdargv, cmdargvlen);
    ++nreply;

    cmdStream.clear(); //只清除标志
    cmdStream.str(""); //设置新缓冲区*/
    argVec.clear();
}

template <typename T, typename... Arg>
void Redis::pipelineAddCmd(const T t, const Arg... a)
{
    //用vector保存参数列表
    cmdStream << t;
    std::string str = cmdStream.str();
    argVec.push_back(str);

    cmdStream.clear(); //只清除标志
    cmdStream.str(""); //设置新缓冲区*/

    pipelineAddCmd(a...);
}

#endif
