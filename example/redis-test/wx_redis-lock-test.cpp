#include "wx_redis.h"

using std::string;

#define N_CHILD 10

//分布式锁测试
//测试10秒内10个进程写入redis服务器能获得多少次锁
int ntry = 0;//尝试抢锁的次数， 需要在wx_redis.cpp中lock函数实现中自增
int nlocked = 0;//获取锁的次数
int pid;
struct timeval start, finish;

string intToStr(int n);

void sig_int(int sig);

int main(int argc, char **argv)
{
    /******
    Redis conn;
    conn.cmd(string("set bar 1234567"));
    conn.printReply();
    conn.cmd(string("get bar"));
    conn.printReply();

    conn.cmd("%s %s %d", "set", "bar", 987654321);
    conn.printReply();
    conn.cmd("%s %s", "get", "bar");
    conn.printReply();

    //conn.cmd("get", "bar","ss");
    conn.cmd(string("get") + " bar"); // " " + "bar"
    conn.printReply();

    conn.cmdArgs("get", "foo");
    conn.printReply();
    conn.cmdArgs("get", "bar");
    conn.printReply();

    conn.cmdArgs("rpush", "list1", 125);
    conn.printReply();
    conn.cmdArgs("rpop", "list1");
    conn.printReply();

    conn.cmdArgs("hset", "hash1", "foo", 2569);
    conn.printReply();
    conn.cmdArgs("hget", "hash1", "foo");
    conn.printReply();

    conn.lock("mylock");
    conn.cmdArgs("hget", "hash1", "foo");
    conn.printReply();
    //wx_sleep(7.5);
    //sleep(7);
    conn.unlock("mylock");
    conn.endConnection()
    /******/
    ;
    
    //分布式锁测试
    //测试10秒内10个进程写入redis服务器能获得多少次锁
    signal(SIGINT, sig_int);
    pid = getpid();
    int childpids[N_CHILD];

    for (int i = 0; i < N_CHILD; i++)
    {
        if ((childpids[i] = fork()) == 0)
        {
            Redis conn1;
            pid = getpid();
            gettimeofday(&start, NULL);

            int ind = 0;
            char buf[1024];

            while (1)
            {
                if (conn1.lock("my_lock") == 0)//timeout不能为0，否则会引起错误ERR invalid expire time in setex
                {
                    //++ntry;
                    //printf("lock failed\n");
                    //wx_sleep((rand() / (double)RAND_MAX) / 0.001);
                    continue;
                }

                //++ntry;
                ++nlocked;

                //snprintf(buf, 1024, "pid: %d, %d", pid, ++ind);
                //conn1.cmdArgs("hset", "hash1", "foo", buf);

                conn1.cmdArgs("hset", "hash1", "foo", string("pid ") + intToStr(pid) + " " + intToStr(++ind));
                
                //conn1.printReply();
                conn1.unlock("my_lock");
            }
            return 0;
        }
    }

    wx_sleep(10);
    for (int i = 0; i < N_CHILD; i++)
        kill(childpids[i], SIGINT);

    wx_sleep(0.1);
    return 0;
}

string intToStr(int n)
{
    std::ostringstream os;
    os << n;
    return os.str();
}

void sig_int(int sig)
{
    gettimeofday(&finish, NULL);
    long ds = finish.tv_sec - start.tv_sec;
    long du = finish.tv_usec - start.tv_usec;
    if (finish.tv_usec < start.tv_usec)
    {
        du = start.tv_usec - finish.tv_usec;
        ds -= 1;
    }
    printf("pid: %d, time used: %ld.%06ld seconds, tried: %d, get locked: %d\n", pid, ds, du, ntry, nlocked);
    exit(0);
}
