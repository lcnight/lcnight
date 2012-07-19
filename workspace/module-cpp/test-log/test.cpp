/**
 * =====================================================================================
 *       @file  test.cpp
 *      @brief  
 *
 *     Created  07/05/2011 04:32:20 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#include  <unistd.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  "log.h"
#include  "stat.h"


int main ( int argc, char *argv[] )
{
    //    if(!stat_init("./stat","stat")) {
    //        printf("init stat log failed\n");
    //        return -1;
    //    }
    //
    //    c_time start;
    //    for (int i = 0 ; i < 10000 ; i ++) {
    //        stat_log("wo cha cha %s %d", "hello world", 131524);
    //        stat_log("cha %s %d", "hello world", 11524);
    //        stat_log("cha %s %d", "hello world", 1);
    //    }
    //    c_time end;
    //    printf("time span %ld microseconds\n", end - start);
    //    stat_uninit();

    if(log_init("./log",log_lvl_debug, 4*1024, 0, "log_") != 0) {
        printf("init log failed\n");
        return -1;
    }
    set_log_dest(log_dest_file);

    c_time start;

    DEBUG_LOG("%s %s", "中国", "hello");
    //for (int i = 0 ; i < 10000 ; i ++) {
        //DEBUG_LOG("wo cha cha %s %d", "hello world", 131524);
        //DEBUG_LOG("cha %s %d", "hello world", 11524);
        //DEBUG_LOG("cha %s %d", "hello world", 1);
    //}
    c_time end;
    printf("time span %ld microseconds\n", end - start);
    //log_fini();

    return EXIT_SUCCESS;
} /* ----------  end of function main  ---------- */
