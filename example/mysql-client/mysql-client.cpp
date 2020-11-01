/*
*模拟mysql客户端，从stdin读取并执行语句，输出格式与原版类似
*/

#include "wx_mysql.h"

int main()
{
    //Mysql sql;
    //sql.connect("127.0.0.1", "testdb", "123", 0, NULL, "testdb", 0);

    Mysql sql("mysql.conf");//从配置文件获取数据库用户信息，登录

/*
    //测试
    sql.query("select * from student where student_id < 15");
    sql.storeRes();
    sql.printRes();

    sql.queryAndStoreRes("select * from student where student_id < 5");
    sql.printRes();
*/

    std::string str;
    std::cerr << "Query> ";
    while (std::getline(std::cin, str))
    {
        //std::cout << str << "+\n";
        if (sql.queryAndStoreRes(str.c_str()))
            sql.printRes();
        std::cerr << "Query> ";
    }

    return 0;
}
