/**
 *      @brief  
 *
 *     Created  02/22/2012 04:06:18 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "inttypes.h"
#include "atomic.h"

int main(int argc, char *argv[])
{
    //atomic_t a = 123; # err initilizer
    atomic_t a;
    atomic_set(&a, 123);
    atomic_inc(&a);
    printf("%d\n", atomic_add_return(1, &a));
    printf("%d\n", atomic_read(&a));
    return 0;
}/* -- end of main  -- */
