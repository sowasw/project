#ifndef _WX_LIB_H_
#define _WX_LIB_H_

#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/eventfd.h>

void wx_sleep(double seconds);

typedef void SIGFUNC(int);
SIGFUNC* wx_signal(int signo, SIGFUNC* func);

#endif