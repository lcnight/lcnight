/**
 * =====================================================================================
 *       @file  stat.cpp
 *      @brief  
 *
 *     Created  07/06/2011 11:38:12 AM 
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
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <stdint.h>
#include  <stdarg.h>
#include  <string.h>
#include  <time.h>

#include  "stat.h"

#ifdef  likely
#undef  likely
#endif
#define likely(x) __builtin_expect(!!(x), 1)

#ifdef  unlikely
#undef  unlikely
#endif
#define unlikely(x) __builtin_expect(!!(x), 0)
static int has_init = false;
static char log_pre[32];
static char log_dir[256];
static const uint32_t log_size = 64*1024*1024; //64M
static struct fd_t {
	int	opfd;
	int	seq;
	int	yday;
} fd_info = {-1, 0, -1};

static char filename[512] = {0};
static char logbuf[8*1024] = {0};

bool stat_init(const char * logdir, const char * logpre)
{
    if (logdir == NULL || logpre == NULL) {
        return false;
    }

    strncpy(log_dir, logdir, sizeof(log_dir));
    strncpy(log_pre, logpre, sizeof(log_pre));

    if(access(log_dir, R_OK|W_OK|X_OK) != 0) {
        return false;
    }

    has_init = true;
    return true;
}

static inline void
log_file_path(char* file_path, int seq, const struct tm* tm)
{
    sprintf(file_path, "%s/%s_%d_%04d%02d%02d%06d", log_dir, log_pre, getpid(), 
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, seq);
}
static inline void
close_fd(int fd)
{
    fsync(fd);
    close(fd);
}
static int open_fd(struct fd_t *fds, struct tm *ts) 
{
    log_file_path(filename, fds->seq, ts);
    static int flag = O_WRONLY | O_CREAT | O_APPEND/* | O_LARGEFILE*/;
    int fd = open(filename, flag, 0666);

    return fd < 0 ? -1 : fd;
}

static int shift_fd(struct fd_t *fds, struct tm *ts) 
{
    if (unlikely(fds == NULL || ts == NULL)) {
        return -1;
    }

    if (likely(fds->opfd >= 0)) {
        off_t length = lseek(fds->opfd, 0, SEEK_END);

        if ((length < log_size) /*&& (length > 0)*/ && (fds->yday == ts->tm_yday)) {
            return fds->opfd;
        }
        else if ((length >= log_size) && (fds->yday == ts->tm_yday)) {
            close_fd(fds->opfd);
            fds->seq++;
            fds->opfd = open_fd(fds, ts);
            return fds->opfd;
        } 
        else if (fds->yday != ts->tm_yday) {
            close_fd(fds->opfd);
            fds->seq = 0;
            fds->yday = ts->tm_yday;
            fds->opfd = open_fd(fds, ts);
            return fds->opfd;
        } 
    } else {
        fds->seq = 0;
        fds->yday = ts->tm_yday;
        fds->opfd = open_fd(fds, ts);
        return fds->opfd;
    }
}

void write_log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int cnt = vsnprintf(logbuf, sizeof(logbuf), fmt, ap);
    va_end(ap);

    time_t now;
    struct tm ltm;
    time(&now);
    localtime_r(&now, &ltm);

    if (unlikely(shift_fd(&fd_info, &ltm) < 0)) {
        return;
    }

    write(fd_info.opfd, logbuf, cnt);
}

void stat_uninit()
{
    if (!has_init) {
        return;
    }
    close(fd_info.opfd);
}
#undef  likely
#undef  unlikely

/*-------------------------------impl of c_time class-------------------------------------------*/
c_time::c_time() {
    gettimeofday(&tv, 0);
}
c_time::c_time(long int sec, long int usec){
    tv.tv_sec = sec;
    tv.tv_usec = usec;
}
long int c_time::operator-(const c_time &rhs) {
    if (this == &rhs) {
        return 0;
    }
    return (this->tv.tv_sec - rhs.tv.tv_sec) * 1000000 + (this->tv.tv_usec - rhs.tv.tv_usec);
}
