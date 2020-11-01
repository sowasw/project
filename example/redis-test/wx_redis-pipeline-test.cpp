#include "wx_redis.h"

using std::string;
int ntry = 0;

void redisPrintReply(redisReply *reply);

int main(int argc, char **argv)
{
    Redis conn;

    redisReply *r[20];

    //不增加底层命令计数
    redisAppendCommand(conn.getContext(), "get foo");
    redisAppendCommand(conn.getContext(), "hget hash1 foo");
    redisAppendCommand(conn.getContext(), "get foo");
    redisAppendCommand(conn.getContext(), "exec");
    for (int i = 0; i < 4; i++)
    {
        redisGetReply(conn.getContext(), (void **)r + i);

        //redisGetReplyFromReader(conn.getContext(), (void **)r + i);//?

        redisPrintReply(r[i]);
        freeReplyObject(r[i]);
    }

    std::cout << "-------------pid " << getpid() << "\n";

    conn.pipelineAddCmd("hget", "hash1", "foo");
    conn.pipelineAddCmd("get", "foo");
    conn.pipelineAddCmd("hget", "hash1", "foo");
    conn.pipelineAddCmd("get", "foo");
    conn.pipelineAddCmd("zrange", "zset1", 0, 3, "withscores");
    conn.pipelineAddCmd("exec");
    conn.pipelineExec(); //没有用exec命令

    conn.printReplys();
    conn.pipelineClear();

    conn.scriptLua("return 1");
    conn.printReply();
    std::cout << "--------------------------------------------1 "
              << "\n";
    conn.cmdArgs("get", "foo");
    std::cout << conn.stringCmdReply();
    std::cout << "--------------------------------------------2 "
              << "\n";
    conn.pipelineAddCmd("hget", "hash1", "foo");
    conn.pipelineAddCmd("get", "foo");
    conn.pipelineAddCmd("zrange", "zset1", 0, -1, "withscores");
    conn.pipelineAddCmd("exec");
    conn.pipelineExec(); //没有用exec命令
    std::cout << conn.stringPipelineReplys(0,1);

    wx_sleep(5);

    return 0;
}

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