#ifndef _WEBAY_TYPE_H_
#define _WEBAY_TYPE_H_

//状态
/*
#define STAT_OK     "ok"
#define STAT_FAIL   "fail"
#define STAT_EXIST  "exist"
#define STAT_ERR    "err"
*/

#define SESSION_LIMIT 3
#define SESSION_MAX_ALIVE_SECOND 5 * 60 //会话最长有效时间
#define TOCKEN_LEN 36

//状态码
#define STAT_OK 1
#define STAT_FAIL 2
#define STAT_EXIST 3
#define STAT_ERR 4
#define STAT_VOID 5

//状态码占用字符数
#define STAT_LEN 2

//业务号
#define CMD_SIGN 0
#define CMD_LOGIN 1
#define CMD_SHOWCOM 2
#define CMD_ADDCART 3
#define CMD_GETCART 4
#define CMD_CREATEODER 5
#define CMD_GETORDER 6

#endif
