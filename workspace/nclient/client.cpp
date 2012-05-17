/**
 *       @file  client.cpp
 *      @brief  test used client: send message packages by rules
 *
 *   @internal
 *     Created  10/17/2011 11:04:28 AM 
 *
 *     @author  lc (l.c.), lc@taomee.com
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "macro.h"
#include "sockets.h"
#include "pack.h"

#define     MAX_SZ      1024
#define     VERBOSE(fmt, arg...)    if(G_VAR.rand || G_VAR.verbose) { PRINT(fmt, ##arg); }
#define     LOG_DUMP(fd, buf, size)    if(G_VAR.rand || G_VAR.verbose) { write(fd, buf, size); }
#define     VERSION         "0.0.0"
typedef struct field {
    int len;
    bool numeric; //denote value is numer or not 
    union { 
        int num;
        char *str;
    };
    field *next;
} fld_t;
struct global_setting{
    fld_t *flds;
    bool rand;

    int send_time;  //send package by time seconds
    int send_count; //send package by count times

    char server[20];
    int port;
    int log_fd;
    bool verbose;
    bool align;
}G_VAR = {0, false, 0, 0, {0}, 0, -1, false, false};
static char data_buf[MAX_SZ] = {0};
static char recv_buf[MAX_SZ] = {0};

void help_usage();

int main(int argc, char *argv[])
{
    char arg = 0;
    char *endptr = NULL;
    while((arg = getopt(argc, argv, "f:rt:n:as:p:x:vVh")) != -1) {
        switch (arg)
        {
        case 'f' :
            {
                strcpy(data_buf, optarg);
                char *ptr = data_buf; 
                if(*ptr == '-') {
                    help_usage();
                    exit(1);
                }
                ptr = strtok(data_buf, ",");
                while(ptr) {
                    fld_t *tmp = (fld_t*)malloc(sizeof(fld_t));
                    tmp->next = NULL;
                    char *tmp_c = strchr(ptr, ':');
                    *tmp_c = 0;
                    tmp->len = strtol(ptr, &endptr, 10);
                    if(*(tmp_c + 1) == '\047') {
                        tmp->numeric = false;
                        tmp->str = strdup(tmp_c + 1);
                    } else {
                        tmp->numeric = true;
                        tmp->num = strtol(tmp_c + 1, &endptr, 10);
                    }
                    //tmp->next = G_VAR.flds;
                    //G_VAR.flds = tmp;
                    if (G_VAR.flds == NULL) {
                        G_VAR.flds = tmp;
                    } else {
                        fld_t *ll = G_VAR.flds;
                        while(ll->next) {
                            ll = ll->next;
                        }
                        ll->next = tmp;
                    }
                    ptr = strtok(NULL, ",");
                }
            }
            break;
        case 'r' :
            G_VAR.rand = true;
            break;
        case 't' :
            G_VAR.send_time = strtol(optarg, &endptr, 10);
            break;
        case 'n' :
            G_VAR.send_count = strtol(optarg, &endptr, 10);
            break;
        case 's' :
            strcpy(G_VAR.server, optarg);
            break;
        case 'p' :
            G_VAR.port = strtol(optarg, &endptr, 10);
            break;
        case 'a' :
            G_VAR.align = true;
            break;
        case 'x' :
            G_VAR.log_fd = open(optarg, O_WRONLY|O_CREAT|O_APPEND, 0666);;
            if (G_VAR.log_fd == -1) {
                PRINT("open log file error: %s", strerror(errno));
                exit(2);
            }
            break;
        case 'v' :
            G_VAR.verbose = true;
            break;
        case 'V' :
            PRINT("client tool ver %s, build on %s %s", VERSION, __DATE__, __TIME__);
            exit(-1);
        default : /* 'h' '?' ':' */
            help_usage();
            exit(-1);
        }  /* end of switch */
    }

    if (strcmp(G_VAR.server, "") == 0 || G_VAR.port <= 0) {
        PRINT("server or port error: %s %d", G_VAR.server, G_VAR.port);
        help_usage();
        exit(1);
    }
    int pkg_len = 0;
    static char alphabet[] = "0123456789ABCDEFGFIJKLMNOPQRSTUVWXYZabcdefgfijklmnopqrstuvwxyz+-*&%$#@!~()[]{}";
    static int alph_len = strlen(alphabet);
    if (G_VAR.rand) {
        if (G_VAR.flds) {
            PRINT("-r -f can not be used together");
            help_usage();
            exit(1);
        }
        srand(time(NULL));
        pkg_len = rand() % MAX_SZ;
        uint8_t *pkg = (uint8_t *)data_buf;
        for (int i = 0 ; i < pkg_len - 2 ; i++) {
            put_u8(&pkg, alphabet[rand() % alph_len]);
        }
        put_u8(&pkg, '\015');
        put_u8(&pkg, '\012');
    } else if (G_VAR.flds) {
        uint8_t *pkg = (uint8_t *)data_buf;
        for(fld_t *f = G_VAR.flds; f ; f = f->next) {
            pkg_len += f->len;
            if (f->numeric && f->len == 2) {
                uint16_t tmp = (uint16_t)f->num;
                put_u16(&pkg, tmp, G_VAR.align);
            } else if (f->numeric && f->len == 4) {
                uint32_t tmp = (uint32_t)f->num;
                put_u32(&pkg, tmp, G_VAR.align);
            } else { //string
                int pack_len = 0;
                for (int i = 0; f->str[i]; i++) {
                    if (f->str[i] && f->str[i]=='\047') {
                        continue;
                    }
                    ++pack_len;
                    put_u8(&pkg, (uint8_t)f->str[i]);
                }
                for ( ;pack_len < f->len  ; pack_len++) {
                    put_u8(&pkg, 0);
                }
                put_u8(&pkg, 0);
            }
        }
    } else {
        help_usage();
        exit(-1);
    }

    if (G_VAR.send_time > 0 && G_VAR.send_count > 0) {
        PRINT("-t -n can not be used together");
        help_usage();
        exit(-1);
    }

    int fd = tcpsocket();
    uint32_t host = 0;
    if (tcpresolve_c(G_VAR.server, NULL, &host, NULL) == -1
            || tcpnumtoconnect(fd, host, G_VAR.port, 1000) == -1) {
        PRINT("cann not connect to server %s %d", G_VAR.server, G_VAR.port);
        exit(1);
    }

    int ep_fd = epoll_create(1024);
    epoll_event ev; 
    ev.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    ev.data.fd = fd;
    epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev);

    static struct epoll_event eve[100] = {{0, {0}}};
    uint32_t t_start = time(NULL);
    int loop_cnt = G_VAR.send_count > 0 ? G_VAR.send_count : 1 ;
    int real_times = 0;

    const char* sep = "\r\n";
    LOG_DUMP(G_VAR.log_fd, "send\r\n",  6);
    LOG_DUMP(G_VAR.log_fd, data_buf, pkg_len);
    LOG_DUMP(G_VAR.log_fd, sep,  strlen(sep));
    int total_snt = 0;
    int total_rcv = 0;
    for (int send_cnt = 0 ; send_cnt < loop_cnt ; ++real_times) {
        VERBOSE("loop cnt %d/%d", send_cnt + 1, loop_cnt);

        int cnt = epoll_wait(ep_fd, eve, 100, 100);
        for (int i=0 ; i < cnt ; i++) {
            if (eve[i].events & EPOLLERR || eve[i].events & EPOLLHUP || eve[i].events & EPOLLRDHUP) {
                PRINT("meet err, close and exit %d", eve[i].events);
                close(fd);
            }

            if (eve[i].events & EPOLLOUT) {
                int ret = send(eve[i].data.fd, data_buf, pkg_len, 0);
                if (ret == -1) {
                    if (errno == EAGAIN) {
                        PRINT("meet send EAGAIN");
                        continue;
                    } else if (errno == EINTR) {
                        PRINT("meet send EINTR");
                        continue;
                    } else {
                        close(eve[i].data.fd);
                    }
                } else {
                    total_snt += ret;
                }
            }

            if (eve[i].events & EPOLLIN) {
                int ret = recv(eve[i].data.fd, recv_buf, MAX_SZ - 1, 0);
                if (ret == -1) {
                    if (errno == EAGAIN || errno == EINTR) {
                        continue;
                    } else {
                        close(eve[i].data.fd);
                    }
                } else {
                    recv_buf[ret] = 0;
                    total_rcv += ret;
                    VERBOSE("%d:%s", ret, recv_buf);
                    LOG_DUMP(G_VAR.log_fd, recv_buf, ret);
                }
            }
        } /*-- end of for --*/

        if (G_VAR.send_count > 0) {
            ++send_cnt;
        } else {
            uint32_t now = time(NULL);
            if (now - G_VAR.send_time > t_start) {
                break;
            }
        }
    }

    PRINT("summary: send package length %d bytes, total(snd:%d, rcv:%d), %d times, %d seconds", 
            pkg_len, total_snt, total_rcv, real_times, G_VAR.send_time);
    close(fd);
    close(ep_fd);
    return 0;
} /* -- end of main  -- */

void help_usage() {
    PRINT("Usage: client -[f|r] arg.. -[t|n] arg.. -s srv -p port\n\n"
            "\t-f\tmessage package fields list '{<field_len, field_value>}'\n"
            "\t-r\tmessage package is a random string end with \\r\\n\n"
            "\t-t\tsend package in denoted seconds\n"
            "\t-n\tsend package count times\n"
            "\t-a\tif align message package to netword order, default not\n"
            "\t-s\tconnect to server address\n"
            "\t-p\tconnect to server port\n"
            "\t-v\tverbose to print operation log\n"
            "\t-x\tdump to log file\n"
            "\t-V\tshow version infomation\n"
            "\t-h\tshow help usage\n"
            );
}
