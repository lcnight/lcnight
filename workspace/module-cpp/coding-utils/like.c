/**
 *      @brief  
 *
 *     Created  07/25/2012 05:22:53 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

int main(int argc, char *argv[])
{
    //int a;

    /* Get the value from somewhere GCC can't optimize */
    int a = atoi (argv[1]);

    int x = 0;
    //if ((a == 2))
    if (unlikely (a == 2))
        //if (likely(a == 2))
    {
        //a++; 
        x = 55;}
    else
    {
        //a--; 
        x = 66;}

    printf("%d  %d\n", a, x);

    return 0;
}
