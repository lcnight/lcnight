/**
 *      @brief  
 *
 *     Created  02/02/2012 08:21:38 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <setjmp.h>


void sig_state(const char *note)
{
    printf("%s:\n", note);
    sigset_t old;
    sigset_t new;

    sigpending(&old);
    printf("pending %llx\n", old);

    sigemptyset(&new);
    sigemptyset(&old);
    sigprocmask(SIG_UNBLOCK, &new, &old);
    printf("current signal mask: %llx\n", old);
    sigprocmask(SIG_SETMASK, &old, &new);
    printf("block SIGALRM: %s\n", sigismember(&old, SIGALRM) > 0 ? "true" : "false");
}

//#define SIG_VERSION  1
#ifndef SIG_VERSION
static jmp_buf buf;
#else
static sigjmp_buf buf;
#endif
 
void signal_handler (int x)
{   
    sig_state("signal handler");

    printf ("Signal with the following number: %d\n", x);
    /* Go back to where setjmp was called. Return the value "1" from
       setjmp. */
#ifndef SIG_VERSION 
    longjmp (buf,1);
#else
    siglongjmp (buf,1);
#endif
}

/* Run indefinitely */

void run_indefinitely ()
{
    while (1) {

    }
}

int main (int argc, char ** argv)
{
    struct sigaction act;
    act.sa_handler = signal_handler;
    sigaction (SIGALRM, & act, 0);

    while(1) {
        /* Set an alarm to go off after 1,000 microseconds (one thousandth
           of a second). */
        ualarm (1000, 0);
#ifndef SIG_VERSION 
        if (! setjmp (buf)) {
#else
        if (! sigsetjmp (buf, 1)) {
#endif
            sig_state("set jump");
            /* The first time setjmp is called, it returns zero. */
            run_indefinitely ();
        } else {
            sig_state("long jump");
            /* This branch is reached from the function "signal_handler"
               above. */
            printf ("Jumped to here.\n");
        }
        printf("\n\n");
    }
    return 0;
}
