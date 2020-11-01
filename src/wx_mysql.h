#ifndef _WX_MYSQL_
#define _WX_MYSQL_

#include <mysql.h>
//#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>

//extern int ntry;

class Mysql
{
public:
    typedef std::shared_ptr<std::vector<std::shared_ptr<std::string>>> RowPtr;//存储一行的每列
    typedef std::shared_ptr<std::vector<RowPtr>> ResPtr; //存储所有行

    Mysql();
    Mysql(const char *conf);
    //Mysql(const char *conf);
    ~Mysql();
    void initMysql();

    MYSQL_RES *getRes() { return res; }
    MYSQL *getConn() { return &conn; }
    bool isConnectedMysql() { return this->isConnected; }
    std::string getHost();
    int getPort() { return conn.port; }

    bool connect(const char *host, const char *user, const char *password, const unsigned int port = 0,
                 const char *socket = NULL, const char *db_name = NULL, const unsigned int flags = 0);
    bool connect(const char *conf = "mysql.conf");

    bool query(const char *query);            //执行SQL语句
    bool queryAndStoreRes(const char *query); //执行SQL语句并填充MYSQL_RES结构
    bool storeRes();                          //填充MYSQL_RES结构

    ResPtr getResPtr(); //返回结果集的字符串点阵

    //适用于打印的结果集字符串
    std::string resToString();
    std::string &resToString(std::string &result);
    void printRes(); //打印执行语句后的结果到stdout，与mysql命令行客户端格式一样

private:
    char opt_host_name[16 + 1];
    char opt_user_name[16];
    char opt_password[16];
    unsigned int opt_port_num;
    char opt_socket_name[16];
    char opt_db_name[16];
    unsigned int opt_flags;

    bool isConnected;

    MYSQL conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    MYSQL_FIELD *field;
};

#endif
