/**
 *      @brief  
 *
 *     Created  02/22/2012 11:05:47 AM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>

int main(int argc, char *argv[])
{
    char name[32] = {0};
    prctl( PR_GET_NAME, ( unsigned long ) name) ;

    prctl( PR_SET_NAME, ( unsigned long ) "hello world hello world") ;

    prctl( PR_GET_NAME, ( unsigned long ) name) ;
    printf("%s\n", name);
    return 0;
}/* -- end of main  -- */
