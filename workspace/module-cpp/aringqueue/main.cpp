/**
 * =====================================================================================
 *       @file  test.cpp
 *      @brief  main file for test
 *
 *     Created  05/29/2011 05:01:47 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, lc
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include  "aringqueue.h"

#define     VERSION     "1.0.0"

int main(int argc, char *argv[])
{
    if (argc == 2 && strcasecmp(argv[1], "-v") == 0) {
       printf("%s %s %s %s\n", basename(argv[0]), VERSION, __DATE__, __TIME__);
       return 0;
    }

    aringqueue a(16);;
    //aringqueue b(a);
    //a=b;
#ifdef DEBUG 
    a.dump();
#endif
    for (int i = 0 ; i < 4 ; i ++) {
        int rn = rand();
        printf("%d   ", rn);
        a.push(&rn, sizeof(int));
    } /*-- end of for --*/
    printf("\ncapcity %d length %d\n", a.get_capcity(), a.get_length());
#ifdef DEBUG 
    a.dump();
#endif
    int buf;
    for (int i = 0 ; i < 4 ; i ++) {
        a.peer(&buf, sizeof(int));
        printf("get %d   ", buf);
    } /*-- end of for --*/
    printf("\ncapcity %d length %d\n", a.get_capcity(), a.get_length());
#ifdef DEBUG 
    a.dump();
#endif
    for (int i = 0 ; i < 4 ; i ++) {
        a.pop(&buf, sizeof(int));
        printf("get %d   ", buf);
    } /*-- end of for --*/
    printf("\ncapcity %d length %d\n", a.get_capcity(), a.get_length());
#ifdef DEBUG 
    a.dump();
#endif
    return 0;
}
