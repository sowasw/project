//g++ protocol-test.cpp ../Protocol.cpp -I../

#include <iostream>
#include <string>
#include <unistd.h>
#include <stdio.h>

#include "Protocol.h"

using namespace std;

int main()
{
    ///1
    printf("MessageRead test-----\n");
    char buf[4096];
    //strcpy(buf, "10240010weng            haha1024        ");

    memset(buf, 0, 4096);
    sprintf(buf, "1024");
    sprintf(buf + 4, "1");
    sprintf(buf + 8, "wang");
    sprintf(buf + 8 + 16, "haha1024");

    MessageRead mr(buf);

    message_head mh;
    mr.readMessageHead(&mh);
    string user;
    string password;
    mr.readString(&user, 16);
    mr.readString(&password, 16);

    cout << "head: " << mh.body_size << " " << mh.cmd << "\n";
    cout << "user password: " << user << " " << password << "\n";

    ///2
    printf("MessageWrite test-----\n");

    char wbuf[4096];
    memset(wbuf, 0, 4096);
    MessageWrite mw(wbuf);
    message_head mhw = {40, 2};

    string user2("zhang");
    char password2[] = "helloworld";

    mw.writeMessageHead(&mhw);
    mw.writeString(&user2, 16);
    mw.writeCString(password2, 16);

    mw.writeLong(9012222222258656454, 20);
    mw.writeDouble(586546546479.2546, 20);

    write(1, wbuf, 4096);
    printf("\n");

    memset(buf, 0, 4096);
    memcpy(buf, wbuf, mw.getLen());

    mr.clear();
    mr.readMessageHead(&mh);
    mr.readString(&user, 16);
    mr.readString(&password, 16);

    long l;
    double d;
    mr.readLong(&l, 20);
    mr.readDouble(&d, 20);

    cout << "head: " << mh.body_size << " " << mh.cmd << "\n";
    cout << "user password: " << user << " " << password << "\n";
    cout << "long double: " << l << " " << d << "\n";

    return 0;
}