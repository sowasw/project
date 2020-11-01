#include "Protocol.h"

bool MessageRead::readMessageHead(MessageHead *head)
{
    if (readInt((int *)&head->body_size, 4) && readInt(&head->cmd, 4))
        return true;
    return false;
}

bool MessageRead::readInt(int *i, size_t len)
{
    if (len < 0 || cur + len - data > MESSAGE_BUF_SIZE)
        return false;

    char buf[len + 1];
    buf[len] = '\0';
    strncpy(buf, cur, len);
    *i = atoi(buf);
    cur += len;
    return true;
}

bool MessageRead::readLong(long *l, size_t len)
{
    if (len < 0 || cur + len - data > MESSAGE_BUF_SIZE)
        return false;

    char buf[len + 1];
    buf[len] = '\0';
    strncpy(buf, cur, len);
    *l = atol(buf);
    cur += len;
    return true;
}

bool MessageRead::readDouble(double *d, size_t len)
{
    if (len < 0 || cur + len - data > MESSAGE_BUF_SIZE)
        return false;

    char buf[len + 1];
    buf[len] = '\0';
    strncpy(buf, cur, len);
    *d = atof(buf);
    cur += len;
    return true;
}

bool MessageRead::readString(std::string *str, size_t len)
{
    if (len < 0 || cur + len - data > MESSAGE_BUF_SIZE)
        return false;

    if (strlen(cur) >= len)
        str->assign(cur, len);
    else
        str->assign(cur, strlen(cur));

    cur += len;
    return true;
}

bool MessageRead::readCString(char *str, size_t len)
{
    if (len < 0 || cur + len - data > MESSAGE_BUF_SIZE)
        return false;

    strncpy(str, cur, len);
    cur += len;
    return true;
}

bool MessageRead::readString(std::string *str)
{
    return readString(str, data + len - cur);
}

bool MessageRead::readCString(char *str)
{
    return readCString(str, data + len - cur);
}

bool MessageWrite::writeMessageHead(MessageHead *head)
{
    if (writeInt((int)head->body_size, 4) && writeInt(head->cmd, 4))
        return true;
    return false;
}

bool MessageWrite::writeInt(int i, size_t len)
{
    char buf[48];
    memset(buf, 0, 48);
    sprintf(buf, "%d", i);
    if (strlen(buf) > len)
        return false;

    return writeCString(buf, len);
}

bool MessageWrite::writeLong(long l, size_t len)
{
    char buf[48];
    memset(buf, 0, 48);
    sprintf(buf, "%ld", l);
    if (strlen(buf) > len)
        return false;

    return writeCString(buf, len);
}

bool MessageWrite::writeDouble(double d, size_t len)
{
    char buf[48];
    memset(buf, 0, 48);
    sprintf(buf, "%f", d);
    if (strlen(buf) > len)
        return false;

    return writeCString(buf, len);
}

bool MessageWrite::writeString(std::string *str, size_t len)
{
    if (str->length() >= len)
        return writeCString((char *)str->c_str(), len);
    else
    {
        char *old = cur;
        if (writeCString((char *)str->c_str(), (size_t)str->length()))
        {
            cur = old + len;
            return true;
        }

        return false;
    }
}

bool MessageWrite::writeCString(const char *str, size_t len)
{
    if (len < 0 || cur + len - data > MESSAGE_BUF_SIZE)
        return false;

    if (strlen(str) < len)
        memcpy(cur, str, strlen(str));
    else
        memcpy(cur, str, len);
    cur += len;
    return true;
}

bool MessageWrite::writeString(std::string *str)
{
    return writeString(str, (size_t)str->length());
}

bool MessageWrite::writeCString(const char *str)
{
    return writeCString(str, (size_t)strlen(str));
}

bool MessageWrite::writeBodyLen(size_t bodylen, size_t len)
{
    char buf[len + 1];
    snprintf(buf, len, "%ld", bodylen);
    for (int i = 0; i < len; i++)
        data[i] = buf[i];
    return true;
}
