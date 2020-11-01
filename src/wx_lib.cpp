#include "wx_lib.h"

void wx_sleep(double seconds)
{
    static int efd = 0;
    if (efd == 0)
    {
        efd = eventfd(0, EFD_CLOEXEC);
        //printf("wx_sleep fd: %d\n", efd);
    }
    int sec = (int)seconds;
    int usec = (int)((seconds - sec) * 1000000);
    struct timeval tv = {sec, usec};
    select(efd, NULL, NULL, NULL, &tv);
    //close(efd);
}

SIGFUNC *wx_signal(int signo, SIGFUNC *func)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT; //
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    if (sigaction(signo, &act, &oact) < 0)
        return (SIG_ERR);
    return (oact.sa_handler);
}