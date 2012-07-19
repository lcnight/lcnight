/**
 * =====================================================================================
 *       @file  stat.h
 *      @brief  
 *
 *     Created  07/06/2011 11:38:14 AM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#ifndef  __STAT_H__
#define  __STAT_H__
#include  <sys/time.h>

//time class in microseconds units
class c_time
{
    public:
        explicit c_time();
        explicit c_time(long int sec, long int usec);
        long int operator-(const c_time &rhs);

    private:
        struct timeval tv;
};

bool stat_init(const char * logdir, const char * logpre);

#define     stat_log(fmt, args...)      write_log(fmt"\n", ##args)
void write_log(const char* fmt, ...);

void stat_uninit();

#endif  /*__STAT_H__*/
