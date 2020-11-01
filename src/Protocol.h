#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdlib.h>
#include <string.h>
#include <string>

#define MESSAGE_BUF_SIZE 4096
#define MESSAGE_HEAD_BUF_SIZE 8
#define MESSAGE_MAX_BODY_SIZE MESSAGE_BUF_SIZE - MESSAGE_HEAD_BUF_SIZE

struct MessageHeadBuf
{
    char *head_buf;
    size_t head_buf_size;
};

struct MessageHead
{
    size_t body_size;
    int cmd;
};

class MessageRead
{
public:
    MessageRead(const char *buf) : data(buf), len(MESSAGE_BUF_SIZE), cur((char *)buf) {}
    ~MessageRead() {}

    size_t getLen() { return len; }
    void clear() { cur = (char *)data; }

    bool readMessageHead(MessageHead *head); //这里只解析了缓冲区8个字节给消息头结构，即4个字节给body_size, 4个字节给cmd
    bool readInt(int *i, size_t len);
    bool readLong(long *l, size_t len);
    bool readDouble(double *d, size_t len);
    bool readString(std::string *str, size_t len);
    bool readCString(char *str, size_t len);
    bool readString(std::string *str);
    bool readCString(char *str);

private:
    const char *data;
    size_t len;
    char *cur;
};

class MessageWrite
{
public:
    MessageWrite(const char *buf) : data((char *)buf), cur((char *)buf) {}
    ~MessageWrite() {}

    size_t getLen() { return cur - data; }
    void clear() { cur = data; }
    bool writeMessageHead(MessageHead *head); //这里只解析了缓冲区8个字节给消息头结构，即4个字节给body_size, 4个字节给cmd
    bool writeInt(int i, size_t len);
    bool writeLong(long l, size_t len);
    bool writeDouble(double d, size_t len);
    bool writeString(std::string *str, size_t len);
    bool writeCString(const char *str, size_t len);
    bool writeString(std::string *str);
    bool writeCString(const char *str);

    bool writeBodyLen(size_t bodylen, size_t len);

private:
    char *data;
    //size_t len;
    char *cur;
};

#endif