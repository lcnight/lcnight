/**
 * =====================================================================================
 *       @file  echo_server.c
 *      @brief  echo server implimented by epoll Edge Triggered ( ET )
 *
 *     @author  masonliu (mason liu), masonliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <list>
#include <map>
#include <memory>

//waiting to be accepted sockets completed established
#define BACKLOG 10
//default port for echo server
#define PORT 8080

//buffer size used to recieve data
#define MAX_BUFSZ (4*1024)
//max returned events number once
#define MAXEVENTS 10
//max clients at the same time
#define MAXCLIENTS 100
//timeout (in second) for sending data
#define TIMEOUT 3

//struct for handling send block
typedef class connection {
private:
    connection& operator=(connection& rhs);
    inline void init() {
        rbuf = (char*)malloc(MAX_BUFSZ);
        rsize = 0;
        wbuf = (char*)malloc(MAX_BUFSZ);
        wsize = 0;
    }
    inline void release() {
        free(rbuf);
        free(wbuf);
    }

public:
    connection(): fd(-1), atime(0), rbuf(0), rsize(0), wbuf(0), wsize(0) {
        init();
    }
    connection(int fd): fd(fd), atime(0), rbuf(0), rsize(0), wbuf(0), wsize(0) {
        init();
    }
    connection(const connection &rhs) {
        this->fd = rhs.fd;
        this->atime = rhs.atime;
        this->rbuf = rhs.rbuf;
        this->rsize = rhs.rsize;
        this->wbuf = rhs.wbuf;
        this->wsize = rhs.wsize;
    }

    //~connection() {
        //release();
    //}
    void destroy() {
        release();
    }

    int fd;
    uint32_t atime;

    char *rbuf;
    int rsize;
    char *wbuf;
    int wsize;
} conn_t;
typedef std::map<int, connection> conn_list_t;
//pair ::= (fd, conn_t)
typedef std::map<int, connection>::iterator conn_list_iter;
conn_list_t conn_list;

// connection statistic
static struct {
    int conn_amount;
}STAT = {
    0
};

//static char inner_buf[MAX_BUFSZ] = {0};

//control continue recieve client connection
static int closing = 0;

#define D_L(fmt, arg...) printf("F:%s L:%d "fmt"\n", __FUNCTION__, __LINE__, ##arg)
#define LOG(fmt, arg...) printf("[%d] "fmt"\n", getpid(), ##arg)

//set the fd to NONBLOCK
int setnonblocking(int fd)
{
    int flag = fcntl(fd, F_GETFL);
    if(flag < 0) {
        perror("F_GETFL");
        return flag;
    }

    if(fcntl(fd, F_SETFL, flag | O_NONBLOCK)) {
        return -1;
    }
    return 0;
}

int setkeepalive(int rs) {
//    if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, ))
    int keepAlive = 1; // 开启keepalive属性
    int keepIdle = 5; // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepInterval = 1; // 探测时发包的时间间隔为5 秒
    int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

    setsockopt(rs, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(rs, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(rs, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(rs, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));
    return 0;
}

static void signal_handler(int sig)
{
    switch (sig) {
        case SIGTERM:
            closing = 1;
            break;
        case SIGINT:
            closing  = 1;
            break;
        case SIGQUIT:
            closing = 1;
            break;
        case SIGURG:  //out of band data handler
            printf("IGURG received\n");
            break;
        default:
            D_L("ERROR: never come here!");
            break;
    }
}

int register_sig(int sig, void(*signal_handler)(int))
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    return sigaction(sig, &act, 0);
}

void conn_add(int fd) {
    conn_list_iter it = conn_list.find(fd);
    if (it == conn_list.end()) {
        connection tmp(fd);
        conn_list.insert(std::pair<int, connection>(fd, tmp));

        STAT.conn_amount++;
    }
}

void conn_release(int fd) {
    conn_list_iter it = conn_list.find(fd);
    if (it != conn_list.end()) {
        it->second.destroy();
        conn_list.erase(it);

        STAT.conn_amount--;
    }
}

void closeclient(int epfd, int fd)
{
    close(fd);
    conn_release(fd);
    // kernel 2.6.9 +, event can be set to NULL
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

int main(int argc, char *argv[])
{
    bool verbose = false;
    int echo_port = PORT;
    LOG("Usage: executable [options|port:8080]\n"
            "options:\t-v\t verbose");
    verbose = strcasestr(argv[1], "-v") != NULL;
    if(!verbose && argc > 1) {
        LOG("IN %d OUT %d RDHUP %d HUP %d ERR %d", EPOLLIN, EPOLLOUT, EPOLLRDHUP, EPOLLHUP, EPOLLERR);
        char *endptr = NULL;
        echo_port = strtol(argv[1], &endptr, 10);
        if (echo_port <= 0 || echo_port >= LONG_MAX) {
            LOG("LONG_MIN %ld LONG_MAX %ld, echo port %d", LONG_MIN, LONG_MAX, echo_port);
            exit(-1);
        }
    }
    LOG("listen to localhost:%d\n", echo_port);

    // 处理信号
    register_sig(SIGTERM, signal_handler);
    register_sig(SIGINT, signal_handler);
    register_sig(SIGQUIT, signal_handler);
    register_sig(SIGPIPE, SIG_IGN);
    /* 设置SIGURG 的处理函数　sig_urg */
    //if(fcntl(ss_fd, F_SETOWN, getpid()) < 0) {
    //perror("can not set OOB own");
    //}
    //register_sig(SIGURG, signal_handler);

    int ss_fd = -1;
    struct sockaddr_in server_addr = {0};
    struct sockaddr_in client_addr = {0};
    socklen_t addr_in_len = sizeof(struct sockaddr_in);
    int yes = 1;
    ss_fd = socket( AF_INET, SOCK_STREAM, 0);
    if(ss_fd == -1){
        perror("create server socket error");
        return EXIT_FAILURE;
    }

    if (setsockopt(ss_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(echo_port);
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(ss_fd, (struct sockaddr *)&server_addr, addr_in_len) == -1) {
        perror("bind server socket error");
        return EXIT_FAILURE;
    } else {
        printf("bind successfully \n");
    }

    if (listen(ss_fd, BACKLOG) == -1) {
        perror("listen server socket error");
        return EXIT_FAILURE;
    } else {
        printf("listen successfully \n");
    }

    printf("listen port %d\n", echo_port);

    int cpid = fork();
    if (cpid > 0) {
        LOG("parent");
    } else {
        LOG("child");
    }

    struct epoll_event ev = {0};
    struct epoll_event  events[MAXEVENTS] = {{0}};

    int epfd = epoll_create(MAXCLIENTS);
    //add server socket to epoll
    ev.events = EPOLLIN ; //| EPOLLET;
    ev.data.fd = ss_fd;
    if(setnonblocking(ss_fd) != 0) {
        perror("css_fd an not nonblocking");
        return EXIT_FAILURE;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, ss_fd, &ev);
    if(epfd == -1){
        perror("epoll listen socket");
        return EXIT_FAILURE;
    }

#define     E_TIMEOUT       10
    /***********************************  main loop  **************************************/
    while(!closing) {
        int ready_fds = epoll_wait(epfd, events, MAXEVENTS, E_TIMEOUT);
        if(ready_fds == -1){
            perror("epoll_wait");
            break;
        }

        for(int n = 0; n < ready_fds; ++n) {
            if (events[n].events & EPOLLERR || events[n].events & EPOLLRDHUP ||
                    events[n].events & EPOLLHUP) {
                LOG("ERROR or HUP\n");
                if (events[n].data.fd != ss_fd) {
                    closeclient(epfd, events[n].data.fd);
                }
                continue;
            }
            if (events[n].events & EPOLLIN) {
                if(events[n].data.fd == ss_fd) { //listening socket
                    while(true) {
                        int c_fd = accept(ss_fd, (struct sockaddr *) &client_addr, &addr_in_len);
                        if(c_fd < 0){
                            if(errno == EAGAIN){
                                break;
                            } else if(errno == EINTR){
                                continue;
                            } else {
                                LOG("accept error %s", strerror(errno));
                                close(ss_fd);
                                break;
                            }
                        } 
                        setnonblocking(c_fd);
                        setkeepalive(c_fd);

                        ev.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                        ev.data.fd = c_fd;

                        //add to epoll events queue
                        if (STAT.conn_amount < MAXCLIENTS) {
                            LOG("total [%d] client, new fd %d Addr: %s:%d", STAT.conn_amount, c_fd,\
                                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                            if (epoll_ctl(epfd, EPOLL_CTL_ADD, c_fd, &ev) < 0) {
                                fprintf(stderr, "epoll set insertion error: fd=%d", c_fd);
                                break;
                            }

                            conn_add(c_fd);
                        } else {
                            const char *refuse_str = "max connections, refuse and close, bye";
                            LOG("%s\n", refuse_str);
                            send(c_fd, refuse_str, strlen(refuse_str), 0);
                            close(c_fd);
                        }
                    }
                } else { //common client
                    int fd = events[n].data.fd;
                    conn_list_iter it = conn_list.find(fd);
                    if (it != conn_list.end()) {
                        int left_sz = MAX_BUFSZ - it->second.rsize;
                        int rsz = recv(fd, it->second.rbuf + it->second.rsize, left_sz, 0);
                        if (rsz <= 0) {
                            LOG("get from fd %d err: %s, close it", fd, strerror(errno));
                            closeclient(epfd, fd);
                            continue;
                        } else if (rsz < left_sz) {
                            //D_L("get from fd %d len: %d", fd, rsz);
                            it->second.rsize += rsz;
                            if (verbose) {
                                LOG("%s", it->second.rbuf);
                            }
                        } else {
                            LOG("recv buf size is to small for bufsz %d fd %d len %d", MAX_BUFSZ, fd, rsz);
                            closeclient(epfd, fd);
                            continue;
                        }
                    } else {
                        LOG("get unknown fd %d EPOLLIN", fd);
                    }
                }
            }
            if (events[n].events & EPOLLOUT) {
                int fd = events[n].data.fd;
                conn_list_iter it = conn_list.find(fd);
                if (it != conn_list.end()) {
                    if (it->second.wsize == 0 && it->second.rsize == 0) {
                        continue;
                    }
                    int nop_sz = it->second.rsize;
                    int left_sz = MAX_BUFSZ - it->second.wsize;
                    if (nop_sz > left_sz) { //buffer too small, send first
                        int wsz = send(fd, it->second.wbuf, it->second.wsize, 0);
                        if (wsz < 0) {
                            LOG("send to fd %d err: %s", fd, strerror(errno));
                            if (errno == EINTR || errno == EAGAIN) {
                                continue;
                            } 
                            closeclient(epfd, fd);
                            continue;
                        }
                        if (wsz != it->second.wsize) {
                            memmove(it->second.wbuf, it->second.wbuf + wsz, it->second.wsize - wsz);
                        }
                        it->second.wsize -= wsz;
                    }
                    left_sz = MAX_BUFSZ - it->second.wsize;
                    if (nop_sz > left_sz) {
                        LOG("send buf size to small for fd %d nop_size %d left_size %d", fd, nop_sz, left_sz);
                        closeclient(epfd, fd);
                        continue;
                    }
                    memcpy(it->second.wbuf + it->second.wsize, it->second.rbuf, nop_sz);
                    it->second.wsize += nop_sz;
                    it->second.rsize -= nop_sz;
                    char *buf = it->second.wbuf;
                    int wsz = send(fd, buf, it->second.wsize, 0);
                    if (wsz < 0) {
                        if (errno == EINTR || errno == EAGAIN) {
                            continue;
                        } 
                        LOG("send to fd %d err: %s, close it", fd, strerror(errno));
                        closeclient(epfd, fd);
                        continue;
                    }
                    it->second.wsize -= wsz;
                } else {
                    LOG("get unknown fd %d EPOLLOUT", fd);
                    closeclient(epfd, fd);
                }
            } 
            //D_L("unkown events get fd %d:%x", events[n].data.fd, events[n].events);
        }
    }
    close(epfd);
    return 0;
}
