#include "wx_mysql.h"

static void mysql_print_error(MYSQL *conn, const char *message);
static void mysql_print_separate_line(int *fields_len, int nfield);

Mysql::Mysql()
{
    initMysql();
}

Mysql::Mysql(const char *conf)
{
    initMysql();
    connect(conf);
}

Mysql::~Mysql()
{
    if (res != NULL)
        mysql_free_result(res);

    mysql_close(&conn);
    mysql_library_end();
}

void Mysql::initMysql()
{
    memset(opt_host_name, 0, sizeof(opt_host_name));
    memset(opt_user_name, 0, sizeof(opt_user_name));
    memset(opt_password, 0, sizeof(opt_password));
    opt_port_num = 0;
    memset(opt_socket_name, 0, sizeof(opt_socket_name));
    memset(opt_db_name, 0, sizeof(opt_db_name));
    opt_flags = 0;

    isConnected = false;

    res = NULL;
    field = NULL;

    if (mysql_library_init(0, NULL, NULL) != 0)
    {
        mysql_print_error(&conn, "mysql_library_init() failed!");
        exit(1);
    }

    //初始化连接处理器
    if (mysql_init(&conn) == NULL)
    {
        mysql_print_error(&conn, "mysql_init() failed!");
        exit(1);
    }
}

std::string Mysql::getHost()
{
    char host[17];
    memset(host, 0, 17);
    strncpy(host, conn.host, 16);
    std::string str(host);
    return str;
}

bool Mysql::connect(const char *host, const char *user, const char *password, const unsigned int port,
                    const char *socket, const char *db_name, const unsigned int flags)
{
    if (res != NULL)
        mysql_free_result(res);

    mysql_close(&conn);

    if (host)
        strncpy(opt_host_name, host, sizeof(opt_host_name));
    if (user)
        strncpy(opt_user_name, user, sizeof(opt_user_name));
    if (password)
        strncpy(opt_password, password, sizeof(opt_password));
    opt_port_num = port;
    if (socket)
        strncpy(opt_socket_name, socket, sizeof(opt_socket_name));
    if (db_name)
        strncpy(opt_db_name, db_name, sizeof(opt_db_name));
    opt_flags = flags;

    if (mysql_real_connect(&conn, opt_host_name, opt_user_name, opt_password,
                           opt_db_name, opt_port_num, opt_socket_name, opt_flags) == NULL)
    {
        mysql_print_error(&conn, "mysql_real_connet() failed!");
        mysql_close(&conn);
        return false;
    }

    isConnected = true;
    return true;
}

bool Mysql::connect(const char *conf)
{
    std::ifstream is;
    std::string str(""), host(""), user(""), password(""), db_name(""), socket_name("");
    int port = 0, flags = 0;

    is.open(conf);
    if (is.fail())
        return false;

    while (std::getline(is, str) && !is.eof())
    {
        if (str[0] == '#')
            continue;
        if (str.find("host") != std::string::npos)
        {
            host = str.substr(str.find("host") + strlen("host="));
            continue;
        }
        else if (str.find("user") != std::string::npos)
        {
            user = str.substr(str.find("user") + strlen("user="));
            continue;
        }
        else if (str.find("password") != std::string::npos)
        {
            password = str.substr(str.find("password") + strlen("password="));
            continue;
        }
        else if (str.find("db_name") != std::string::npos)
        {
            db_name = str.substr(str.find("db_name") + strlen("db_name="), str.length());
            continue;
        }
        else if (str.find("socket_name") != std::string::npos)
        {
            socket_name = str.substr(str.find("socket_name") + strlen("socket_name="), str.length());
            continue;
        }
        else if (str.find("port") != std::string::npos)
        {
            port = atoi(str.substr(str.find("port") + strlen("port="), str.length()).c_str());
            continue;
        }
        else if (str.find("flags") != std::string::npos)
        {
            flags = atoi(str.substr(str.find("flags") + strlen("flags="), str.length()).c_str());
            continue;
        }
    }

    //std::cout << host << std::endl;
    //std::cout << user << std::endl;

    if (connect(host.c_str(), user.c_str(), password.c_str(), port, socket_name.c_str(), db_name.c_str(), flags))
        return true;

    isConnected = false;
    return false;
}

bool Mysql::query(const char *query)
{
    if (mysql_real_query(&conn, query, strlen(query)))
    {
        mysql_print_error(&conn, "mysql_real_query() failed!");
        return false;
    }

    return true;
}

bool Mysql::queryAndStoreRes(const char *query)
{
    if (this->query(query))
    {
        if (storeRes())
            return true;
    }
    return false;
}

bool Mysql::storeRes()
{
    if (res)
        mysql_free_result(res);

    res = mysql_store_result(&conn);
    if (res)
        return true;

    if (mysql_field_count(&conn) == 0)
    {
        //不生成结果集， DELETE，INSERT，UPDATE语句
        //printf("%lu rows affected.\n", (unsigned long)mysql_affected_rows(&conn));
        return true;
    }
    else
    {
        //有错误发生
        mysql_print_error(&conn, "mysql_field_count() failed!");
    }
    //mysql_print_error(&conn, "mysql_store_result() failed!");

    return false;
}

Mysql::ResPtr Mysql::getResPtr()
{
    Mysql::ResPtr resptr = std::make_shared<std::vector<RowPtr>>();

    uint64_t nrow = mysql_num_rows(res);
    if (res == NULL || nrow == 0)
    {
        //Mysql::RowPtr rowptr = std::make_shared<std::vector<std::string>>();
        //rowptr->push_back("");
        //resptr->push_back(rowptr);
        return resptr;
    }

    resptr->reserve(nrow);
    int nfield = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        Mysql::RowPtr rowptr = std::make_shared<std::vector<std::shared_ptr<std::string>>>();
        rowptr->reserve(nfield);
        for (int i = 0; i < nfield; i++)
        {
            //field = mysql_fetch_field(res);
            auto strptr = std::make_shared<std::string>();
            if (row[i] == NULL)
                strptr->assign("");
            else
                strptr->assign(row[i]);
            
            rowptr->push_back(strptr);
        }
        resptr->push_back(rowptr);
    }

    return resptr;
}

std::string Mysql::resToString()
{
    std::string result;
    resToString(result);
    return result;
}

std::string &Mysql::resToString(std::string &result)
{
    if (res == NULL)
    {
        result.assign("");
        return result;
    }

    //无返回结果
    if (mysql_num_rows(res) == 0)
    {
        result.assign("");
        return result;
    }

    std::ostringstream os;

    //获取一行有多少列，每列的长度
    int nfield = mysql_num_fields(res);
    int fields_len[nfield]; //每列长度
    for (int i = 0; i < nfield; i++)
    {
        field = mysql_fetch_field(res);
        fields_len[i] = field->max_length;
        if (!IS_NOT_NULL(field->flags) && fields_len[i] < 4)
            fields_len[i] = 4; // NULL 4字节
        fields_len[i] = fields_len[i] > strlen(field->name) ? fields_len[i] : strlen(field->name);
    }

    //打印+---+---+第一行
    for (int i = 0; i < nfield; i++)
    {
        //printf("+");
        os << "+";
        for (int j = 0; j < fields_len[i] + 2; j++)
            os << "-";
    }
    os << "+\n";

    //打印列名那一行
    mysql_field_seek(res, 0); //重新定位列
    for (int i = 0; i < nfield; i++)
    {
        field = mysql_fetch_field(res);
        //printf("| %-*s ", fields_len[i], field->name); //格式化打印*/
        os << "| " << std::left << std::setw(fields_len[i]) << field->name << " ";
    }
    os << "|\n";

    //打印+---+---+分隔列名与数据的行
    for (int i = 0; i < nfield; i++)
    {
        //printf("+");
        os << "+";
        for (int j = 0; j < fields_len[i] + 2; j++)
            os << "-";
    }
    os << "+\n";

    // 重复读取行，并输出字段的值，直到row为NULL
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        mysql_field_seek(res, 0);
        os << std::setfill(' ');
        for (int i = 0; i < nfield; i++)
        {
            field = mysql_fetch_field(res);
            if (row[i] == NULL)
                //printf("| %-*s ", fields_len[i], "NULL");
                os << "| " << std::setw(fields_len[i]) << "NULL"
                   << " ";
            else if (IS_NUM(field->type))
                //printf("| %*s ", fields_len[i], row[i]); //右对齐
                os << "| " << std::right << std::setw(fields_len[i]) << row[i] << " ";
            else
                //printf("| %-*s ", fields_len[i], row[i]); //左对齐
                os << "| " << std::left << std::setw(fields_len[i]) << row[i] << " ";
        }
        os << std::left << "|\n";
    }

    //格式化表格最后一行
    for (int i = 0; i < nfield; i++)
    {
        //printf("+");
        os << "+";
        for (int j = 0; j < fields_len[i] + 2; j++)
            os << "-";
    }
    os << "+\n";

    result.assign(os.str());
    return result;
}

void Mysql::printRes()
{
    if (res == NULL)
    {
        if (mysql_field_count(&conn) == 0)
        {
            //不生成结果集， DELETE，INSERT，UPDATE语句
            printf("%lu rows affected.\n", (unsigned long)mysql_affected_rows(&conn));
        }
        else
        {
            //有错误发生
            mysql_print_error(&conn, "mysql_field_count() failed!");
        }
        return;
    }

    //无返回结果
    if (mysql_num_rows(res) == 0)
    {
        printf("%lu rows in set.\n", (unsigned long)mysql_affected_rows(&conn));
        return;
    }

    //获取一行有多少列，每列的长度
    int nfield = mysql_num_fields(res);
    int fields_len[nfield]; //每列长度
    for (int i = 0; i < nfield; i++)
    {
        field = mysql_fetch_field(res);
        fields_len[i] = field->max_length;
        if (!IS_NOT_NULL(field->flags) && fields_len[i] < 4)
            fields_len[i] = 4; // NULL 4字节
        fields_len[i] = fields_len[i] > strlen(field->name) ? fields_len[i] : strlen(field->name);
    }

    //打印+---+---+第一行
    mysql_print_separate_line(fields_len, nfield);

    //打印列名那一行
    mysql_field_seek(res, 0); //重新定位列
    for (int i = 0; i < nfield; i++)
    {
        field = mysql_fetch_field(res);
        printf("| %-*s ", fields_len[i], field->name); //格式化打印*/
    }
    printf("|\n");

    mysql_print_separate_line(fields_len, nfield); //打印+---+---+分隔列名与数据的行

    // 重复读取行，并输出字段的值，直到row为NULL
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        mysql_field_seek(res, 0);
        for (int i = 0; i < nfield; i++)
        {
            field = mysql_fetch_field(res);
            if (row[i] == NULL)
                printf("| %-*s ", fields_len[i], "NULL");
            else if (IS_NUM(field->type))
                printf("| %*s ", fields_len[i], row[i]); //右对齐
            else
                printf("| %-*s ", fields_len[i], row[i]); //左对齐
        }
        printf("|\n");
    }

    //格式化表格最后一行
    mysql_print_separate_line(fields_len, nfield);

    printf("%lu rows in set.\n", (unsigned long)mysql_affected_rows(&conn));
}

void mysql_print_error(MYSQL *conn, const char *message)
{
    fprintf(stderr, "%s\n", message);
    if (conn != NULL)
        fprintf(stderr, "Errno: %u, sqlstate: %s, %s.\n", mysql_errno(conn), mysql_sqlstate(conn), mysql_error(conn));
    else
        fprintf(stderr, "No connection here!\n");
}

void mysql_print_separate_line(int *fields_len, int nfield)
{
    for (int i = 0; i < nfield; i++)
    {
        printf("+");
        for (int j = 0; j < fields_len[i] + 2; j++)
            printf("-");
    }
    printf("+\n");
}
