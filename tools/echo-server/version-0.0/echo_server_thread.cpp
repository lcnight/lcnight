/**
 * =====================================================================================
 *       @file  echo_server_thread.cpp
 *      @brief  handle data read and write in thread
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  01/04/2011 09:13:33 AM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>
//waiting to be accepted sockets completed established
#define BACKLOG 10 
//default port for echo server
#define PORT 8080  
//#define ADDR_LEN  (sizeof(struct sockaddr_in))
#define BUF_SIZE (1*1024)
int conn_amount = 0; // current connection amount
int closing = 0; //control continue recieve client connection

#define D_L(fmt, arg...) printf("F:%s L:%d "fmt"\n", __FUNCTION__, __LINE__, ##arg)

//set the fd to NONBLOCK
void setnonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void closeclient( int fd)
{
    int res = close(fd);
    if(res != 0){
        perror("close client socket");
    }
    conn_amount--;
    // kernel 2.6.9 +, event can be set to NULL
    //epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

//only do echo operation
void* do_echo(void* fds)
{
    pthread_t tid =pthread_self();
    pthread_detach(tid);
    int *resp=(int*)malloc(sizeof(int));
    printf("do echo in thread %u \n", tid);
    int fd = *(int*)fds;
    int revlen = 0;
    char buf[BUF_SIZE]; //1K buffer
    while(1){
        int tmplen = read(fd, buf + revlen, BUF_SIZE - revlen);
        //D_L("revlen %d \n",revlen );
        if( tmplen < 0 ){ // == -1
            // 由于是非阻塞的模式,所以当errno为EAGAIN时,表示当前缓冲区已无数据可读
            // 在这里就当作是该次事件已处理处.
            if(errno == EAGAIN){
                break;
            } else {
                perror("NOT EAGIN ERRNO");
                *resp=-1;
                return resp;
            }
        } else if( tmplen == 0 ){  //reach EOF, peer close
            closeclient( fd);
        }
        revlen += tmplen;
        if(revlen > BUF_SIZE){
            perror("BUFFER OVERFLOW");
                *resp=-1;
                return resp;
        }
    }
    if(revlen < BUF_SIZE-1){
        buf[revlen] = '\n' ;
    }
    //printf("RECV: %s\n", buf);
    if( strncmp("quit", buf, 4) == 0 ){
        closeclient( fd);
    }
    if( strncmp("exit", buf, 4) ==0 ){
        closing = 1;
    }
    send(fd, buf, revlen, 0);
}

int main(int argc, char *argv[])
{
    int ss_fd; //server socket fd 
    int nc_fd; //new connected client fd
    struct sockaddr_in server_addr;    // server address information
    struct sockaddr_in client_addr; // connector's address information
    socklen_t addr_in_len = sizeof(struct sockaddr_in); //record length of socket address for AF_INET
    int yes = 1;
    int echo_port = PORT;

    if( argc > 1) {
        echo_port = atol(argv[1]);
    }
    ss_fd = socket( AF_INET, SOCK_STREAM, 0);
    if( ss_fd == -1 ){
        perror("create server socket error");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(ss_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(echo_port);
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(ss_fd, (struct sockaddr *)&server_addr, addr_in_len) == -1) {
        perror("bind server socket error");
        exit(1);
    } else {
        printf("bind successfully \n");
    }
    if (listen(ss_fd, BACKLOG) == -1) {
        perror("listen server socket error");
        exit(1);
    } else {
        printf("listen successfully \n");
    }    

    printf("listen port %d\n", echo_port);
    struct epoll_event ev, events[BACKLOG];;
    int epfd = epoll_create( BACKLOG );
    //add server socket to epoll
    ev.events = EPOLLIN ; //| EPOLLET; 
    ev.data.fd = ss_fd; 
    setnonblocking(ss_fd);
    epoll_ctl(epfd, EPOLL_CTL_ADD, ss_fd, &ev);
    if( epfd == -1){
        perror("epoll_create"); 
        exit(1); 
    }
    int maxevents = BACKLOG;
    sockaddr_in tmpin;
    getsockname(ss_fd,(sockaddr*) &tmpin, &addr_in_len);
    printf("running server sock %s %d \n", inet_ntoa(tmpin.sin_addr), tmpin.sin_port);
    while(!closing) {
        //record ready fd num
        int nfds = epoll_wait(epfd, events, maxevents, -1);
        //int nfds = epoll_wait(epfd, events, maxevents, 500);
        //printf("epoll_wait return %d\n", nfds);
        if( nfds == -1 ){
            perror("epoll_wait");
        }
        int n = 0; //record handling event index
        for( ; n < nfds; ++n) {
            if(events[n].data.fd == ss_fd) {
                nc_fd = accept(ss_fd, (struct sockaddr *) &client_addr, &addr_in_len);
                if(nc_fd < 0){
                    if(errno == EINTR){
                        continue;
                    } else{
                        printf("accept error: %s\n", strerror(errno));
                        continue;
                    }
                }
                setnonblocking(nc_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = nc_fd;

                //add to epoll events queue
                if (conn_amount < BACKLOG) { 
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, nc_fd, &ev) < 0) {
                        fprintf(stderr, "epoll set insertion error: fd=%d", nc_fd);
                        return -1;
                    }
                    conn_amount++;
                    //char *ip = inet_ntoa(client_addr.sin_addr);
                    //D_L("%s",ip);
                    printf("new connection client[%d] with fd %d\nIP Addr: %s %d\n", conn_amount, nc_fd,\
                            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                } else {
                    printf("max connections arrive, refuse and exit\n");
                    send(nc_fd, "max connection, bye", 4, 0);
                    close(nc_fd);
                }                               
            }  else { // handle client socket
                //do_echo(epfd, events[n].data.fd);
                pthread_t ptid;
                int *fd = (int*)malloc(sizeof(int));
                *fd = events[n].data.fd;
                pthread_create(&ptid, NULL, do_echo, fd);
            }
        }
    }
    close(epfd);
}
