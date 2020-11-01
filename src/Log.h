#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>     /* time_t */
#include <sys/time.h> /* struct timeval */
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/eventfd.h>

#include <string>
#include <iostream>
#include <fstream>
#include <queue>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>

#define LOG_TRACE(...) Log::outputLog(LOG_TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) Log::outputLog(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) Log::outputLog(LOG_INFO, __VA_ARGS__)
#define LOG_WARNING(...) Log::outputLog(LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) Log::outputLog(LOG_ERROR, __VA_ARGS__)
#define LOG_SYSERROR(...) Log::outputLog(LOG_SYSERROR, __VA_ARGS__)
#define LOG_FATAL(...) Log::outputLog(LOG_FATAL, __VA_ARGS__)
#define LOG_CRITICAL(...) Log::outputLog(LOG_CRITICAL, __VA_ARGS__)

enum log_level
{
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_SYSERROR,
    LOG_FATAL, //此级别会crash进程
    LOG_CRITICAL
};

//异步日志类，创建一个线程把日志写入文件
class Log
{
public:
    ~Log();

    static Log *instance();

    static void initLog(const char *conf = NULL); //从conf文件中初始化日志目录，格式为logdir=./，
    //没有指定conf文件或者文件名为空""或者文件中=号后面第一个字符为空字符，则设定日志输出到stderr

    //添加日志到待写队列
    static void outputLog(enum log_level lel, const char *fmt, ...);
    static void outputLog(const char *msg);

    static void join();

protected:
    Log(const char *conf);
    static Log *instance(const char *conf);

private:
    static Log *instance_; //单例模式

    static void *logging(void *args); //线程主函数，写日志到文件

private:
    ///两个队列，一个用于待写日志，一个用于日志线程写入文件
    std::queue<std::shared_ptr<std::string>> bufferQueue[2];
    std::queue<std::shared_ptr<std::string>> *toWrite, *toStore, *tmp;
    //bool isStoredToWrite; //缓冲区是否填好日志

    ///从conf文件中设置日志目录，格式为logdir=./，
    std::string conf;
    std::string logFileName; //日志文件名
    std::string logRootPath;
    FILE *logFile;

    ///线程及同步
    pthread_mutex_t logMutex;
    pthread_cond_t logCond;
    pthread_t logThread;

    ///没有待写日志则阻塞日志线程
    int wakeupfd;
    uint64_t evcount;

    bool wouldCrash; //输出日志后退出 LOG_FATAL级别
};

#endif
