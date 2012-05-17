/**
 * =====================================================================================
 *       @file  echo_server.c
 *      @brief  echo server implimented by epoll Edge Triggered ( ET ) 
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  12/30/2010 02:56:10 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
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
#define BUF_SIZE (2*1024)
//max returned events number once
#define MAXEVENTS 10 
//max clients at the same time
#define MAXCLIENTS 100
//timeout (in second) for sending data
#define TIMEOUT 3

//struct for handling send block
typedef struct block_try {
    block_try(): fd(-1), blk_time(0), buf(0), len(0), offset(0) {
    }
    block_try(const struct block_try & blk) {
        fd = blk.fd;
        blk_time = blk.blk_time;
        buf = blk.buf;
        len = blk.len;
        offset = blk.offset;
    }

    char* get_buf() {
        char *tmp = (char*) calloc(1, BUF_SIZE);
        if(tmp == NULL) {
            perror("can not calloc mem");
            return NULL;
        }
        buf = tmp;
        return buf;

    }
    void release_buf() {
        free(buf);
    }

    int fd;
    time_t blk_time;
    //auto_ptr<char> buf;
    char* buf;
    int len;
    int offset;
} blk_try_t;
//typedef std::list<blk_try_t>::iterator blk_list_iter;
//std::list<blk_try_t> blk_list;

std::map<int, blk_try_t> blk_list;
typedef std::map<int, blk_try_t>::iterator blk_list_iter;

char buf[BUF_SIZE] = {0}; //2K buffer

int conn_amount = 0; // current connection amount

int closing = 0; //control continue recieve client connection

#define D_L(fmt, arg...) printf("F:%s L:%d "fmt"\n", __FUNCTION__, __LINE__, ##arg)

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

int mysignal(int sig, void(*signal_handler)(int))
{    
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    return sigaction(sig, &act, 0);
}

void closeclient(int epfd, int fd)
{
    int res = close(fd);
    if(res != 0){
        perror("close client socket");
    }

    blk_list_iter findfd = blk_list.find(fd);
    if(findfd != blk_list.end()) {
        findfd->second.release_buf();
        blk_list.erase(findfd);
    }
        
    conn_amount--;
    // kernel 2.6.9 +, event can be set to NULL
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

//only do echo operation
void do_echo(int epfd, int fd)
{
    int revlen = 0;
    while(1){
        int tmplen = recv(fd, buf + revlen, BUF_SIZE - revlen, 0);
    #ifdef DEBUG
        D_L("this time rev:%s. len:%d \n",buf + revlen, tmplen );
    #endif
        if( tmplen < 0 ){ // == -1
            if(errno == EAGAIN){
                break;
            } else if(errno == EINTR){
                continue;
            } else {
                perror("NOT EAGIN ERRNO");
                closeclient(epfd, fd);
                return;
            }
        } 

        if( tmplen == 0 ){  //reach EOF, peer close
            closeclient(epfd, fd);
            return;
        }

        revlen += tmplen;
    #ifdef DEBUG
        buf[revlen] = 0;
    #endif
        //if(revlen > BUF_SIZE){
        //    perror("BUFFER OVERFLOW");
        //    closeclient(epfd, fd);
        //    return;
        //}
    }
    buf[revlen] = 0;
    //control print revcieve msg or not
    #ifdef DEBUG
        printf("RECV: %s", buf);
    #endif

    if( strncmp("quit", buf, 4) == 0 ){
        closeclient(epfd, fd);
        return;
    }
    if( strncmp("exit", buf, 4) ==0 ){
        blk_list_iter iter = blk_list.begin();
        for(; iter != blk_list.end(); iter++) {
            closeclient(epfd, iter->second.fd);        
        }     
        closing = 1;
        return;
    }

    blk_list_iter findfd = blk_list.find(fd);
    if(findfd != blk_list.end()) {
        if(findfd->second.len + revlen > BUF_SIZE) {
            perror("buffer overflow");
            closeclient(epfd, fd);
            return ;
        }
        strncpy(findfd->second.buf + findfd->second.offset, buf, revlen);
        findfd->second.len += revlen;
        int need_send_len = findfd->second.len - findfd->second.offset;
        int sendlen = send(fd, findfd->second.buf + findfd->second.offset, need_send_len, 0);

        if(sendlen == -1) {
            switch(errno) {
                case EAGAIN: 
                    break;
                case ECONNRESET:
                    closeclient(epfd, fd);
                    break;
                default:
                    perror("ERROR: never come here!");
            } 
        } else if(sendlen == 0) {
            closeclient(epfd, fd);
        } else if(sendlen < need_send_len) {
            findfd->second.offset += sendlen;
        } else {//==
            findfd->second.release_buf();
            blk_list.erase(findfd);
        }
    } else { //a new client socket
        int sendlen = send(fd, buf, revlen, 0);
        if( sendlen != revlen) {
            blk_try_t tmp;
            tmp.fd = fd;
            tmp.blk_time = time(NULL);

            //get_buffer before adding to blk_list, release_buf before erase one blk_try_t
            char * tmpbuf_ptr = tmp.get_buf();
            if(tmpbuf_ptr == NULL) {
                perror("can not get buffer");
            }
            strncpy(tmp.buf, buf, BUF_SIZE);
            tmp.len = revlen;
            tmp.offset = 0;

            if(sendlen == -1) {
                switch(errno) {
                    case EAGAIN: 
                        blk_list.insert(std::pair<int, blk_try_t>(fd, tmp));
                        break;
                    case ECONNRESET:
                        tmp.release_buf();
                        closeclient(epfd, fd);
                        break;
                    default:
                        tmp.release_buf();
                        perror("ERROR: never come here!");
                } 
            }  else if(sendlen < revlen) {
                tmp.offset = sendlen;
                blk_list.insert(std::pair<int, blk_try_t>(fd, tmp));
            }
        }
    }
}

void retry_send(int epfd)
{
    if(!blk_list.empty()) {
        blk_list_iter l_iter = blk_list.begin();
        for(; l_iter != blk_list.end(); l_iter++) {
            time_t cur_t = time(NULL);
            if( cur_t > l_iter->second.blk_time + TIMEOUT) { //timeout
                closeclient(epfd, l_iter->second.fd);
                continue;
            }
            int trylen = l_iter->second.len - l_iter->second.offset;
            int sendlen = send(l_iter->second.fd, l_iter->second.buf + l_iter->second.offset, trylen , 0);
            if(sendlen != trylen) {
                if(sendlen == -1) {
                    switch(errno) {
                        case EAGAIN: 
                            //l_iter->offset = 0;
                            break;
                        case ECONNRESET:
                            closeclient(epfd, l_iter->second.fd);
                            break;
                        default:
                            closeclient(epfd, l_iter->second.fd);
                            perror("ERROR: never come here!");
                    } 
                } else if(sendlen < trylen ) {
                    l_iter->second.offset += sendlen;
                }
            } else { //erase the success trying
                blk_list_iter tmp_iter = l_iter;
                ++l_iter;
                tmp_iter->second.release_buf();
                blk_list.erase(tmp_iter);
                --l_iter;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int ss_fd = -1;   
    int nc_fd = -1; 
    struct sockaddr_in server_addr = {0};   
    struct sockaddr_in client_addr = {0}; 
    socklen_t addr_in_len = sizeof(struct sockaddr_in); 
    int yes = 1;
    int echo_port = PORT;

    if(argc > 1) {
        echo_port = atol(argv[1]);
    }
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

    struct epoll_event ev = {0};  
    struct epoll_event  events[MAXEVENTS] = {0};

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

    // 处理信号
    mysignal(SIGTERM, signal_handler);
    mysignal(SIGINT, signal_handler);
    mysignal(SIGQUIT, signal_handler);
    mysignal(SIGPIPE, SIG_IGN);
    /* 设置SIGURG 的处理函数　sig_urg */
    if(fcntl(ss_fd, F_SETOWN, getpid()) < 0) {
        perror("can not set OOB own");
    }
    mysignal(SIGURG, signal_handler);
    
    while(!closing) {
        int nfds = epoll_wait(epfd, events, MAXEVENTS, 100); //timeout in millsec
        if(nfds == -1){
            perror("epoll_wait");
        }
        
        //try unsuccessful echo operation: may be due to buffer is full when sending
        retry_send(epfd);

        int n = 0; 
        for( ; n < nfds; ++n) {
            if(events[n].data.fd == ss_fd) {
                nc_fd = accept(ss_fd, (struct sockaddr *) &client_addr, &addr_in_len);
                if(nc_fd < 0){
                    if(errno == EINTR){
                        continue;
                    } else {
                        perror("accept error");
                        continue;
                    }
                }

                setnonblocking(nc_fd);
                setkeepalive(nc_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = nc_fd;

                //add to epoll events queue
                if (conn_amount < MAXCLIENTS) { 
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, nc_fd, &ev) < 0) {
                        fprintf(stderr, "epoll set insertion error: fd=%d", nc_fd);
                        return -1;
                    }
                    conn_amount++;
                    //D_L("%s",ip);
                    printf("new connection client[%d] with fd %d\nIP Addr: %s %d\n", conn_amount, nc_fd,\
                         inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                } else {
                    printf("max connections arrive, refuse and exit\n");
                    send(nc_fd, "max connection, bye", 4, 0);
                    close(nc_fd);
                }                               
                #ifdef DEBUG
                sleep(9);
                #endif
            }  else { // handle client socket
                do_echo(epfd, events[n].data.fd);
            }
        }
    }
    close(epfd);
    return 0;
}
