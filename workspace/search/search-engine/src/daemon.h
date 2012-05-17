/**
 * @file daemon.h
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-09-01
 */
#ifndef _H_DAEMON_H_
#define _H_DAEMON_H_

int daemon_start(int argc, char** argv);
void daemon_stop();
void daemon_set_title(const char* fmt, ...);

#endif

