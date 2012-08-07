/**
 * =====================================================================================
 *       @file  re-load.cpp
 *      @brief
 *
 *     Created  08/08/2011 01:13:07 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define CALL_API(state, result)  { \
    if ((state) != result) printf("error no(%d), str(%m)\n", errno);\
}

typedef void (*sighandler_t)(int);

int g_cnt = 0;

void sig_handler(int no)
{
    switch (no)
    {
        case 1:     //SIGHUP
        case 10 :   //SIGUSR1
        case 12 :   //SIGUSR2
            g_cnt ++;
            break;
        default :
            //printf("get unknown sig: %d\n", no);
            break;
    }  ///* end of switch */
    char buf[1024];

    snprintf(buf, 1024, "p %d pp %d get signal %d, total signal count %d\n", getpid(), getppid(), no, g_cnt);

    int fd = open("log", O_WRONLY|O_CREAT|O_APPEND, 0666);
    write(fd, buf, strlen(buf));
    close(fd);

}/* -----  end of function core  ----- */

void sa_act(int sig, siginfo_t *info, void *ptr)
{
    switch (sig)
    {
    case 12 :
        printf("get sig 12\n");
        break;
    default :
        printf("run sa_act function\n");
        break;
    }  /* end of switch */
    printf("id %d %d\n %x %x %x\n", info->si_pid, info->si_uid, info->si_signo, info->si_errno, info->si_code);
}

int main ( int argc, char *argv[] )
{
    signal(1, sig_handler);
    signal(10, sig_handler);
    signal(15, sig_handler);

    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = sa_act;
    sigaction(12, &act, NULL);

    for (int i=0 ; true ; i++) {
        //usleep(1000);
        sleep(1);
        //printf("%d\n", g_cnt);
    }

    //printf("end \n");

    return EXIT_SUCCESS;
} /* ----------  end of function main  ---------- */
