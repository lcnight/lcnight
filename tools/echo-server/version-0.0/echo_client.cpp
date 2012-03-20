/**
 * =====================================================================================
 *       @file  echo_client.cpp
 *      @brief  test client for echo server
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  01/05/2011 09:16:35 AM 
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>

#define HOST "localhost"
#define PORT 8080
#define BUF_SIZE 1024

int closing = 0;
static void signal_handler(int sig)                      
{                                                        
    switch (sig) {                                       
        case SIGTERM:                                    
            closing = 1;                                 
            perror("SIG");
            break;                                       
        case SIGINT:                                     
            closing  = 1;                                
            perror("SIG");
            break;                                       
        case SIGQUIT:                                    
            closing = 1;                                 
            perror("SIG");
            break;                                       
        case SIGURG:  //out of band data handler         
            printf("IGURG received\n");                  
            perror("SIG");
            break;                                       
        case SIGPIPE:
            printf("get SIG PIPE ...");
            perror("SIG");
            break;
        default:                                         
            printf("ERROR: never come here!");              
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

int main(int argc, char *argv[])
{
    // 处理信号                             
    mysignal(SIGTERM, signal_handler);      
    mysignal(SIGINT, signal_handler);      //Ctrl + C 
//    mysignal(SIGQUIT, signal_handler);   //Ctrl + \.
    mysignal(SIGPIPE, signal_handler);             

    int s_fd ; //= socket(AF_INET, SOCK_STREAM, 0);
    char buf[BUF_SIZE] ;
    struct hostent *he;
    char server_host[50] = HOST;
    int server_port = PORT;
    struct sockaddr_in server_addr;

    if(argc > 1) {
        strncpy(server_host, argv[1], 50);
    }
    if(argc > 2) {
        server_port = atoi(argv[2]);
    }

    if ((s_fd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        perror("socket");
        exit(1);
    }    

    he=gethostbyname(server_host);
    if(he == NULL) {
        perror("can not get host info");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr = *((struct in_addr*)he->h_addr);
    memset(server_addr.sin_zero, 0, 8);

    if(connect(s_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) == -1) {
        perror("can not connect");
        exit(1);
    }

    while(1) {
        printf("input a line ('quit' to teminate):\n");
        fgets(buf, BUF_SIZE, stdin);
        if(strncmp("quit", buf, 4) == 0) {
            printf("close and quit \n");
            send(s_fd, "quit", 5, 0);
            break;
        }
        int flag = 0; //MSG_OOB;
        int size = strlen(buf);
        //send(s_fd, buf, BUF_SIZE, flag);
        //int len = recv(s_fd, buf, BUF_SIZE, 0);
        int sendlen = send(s_fd, buf, size, flag);
        if(sendlen == 0) {
            perror("CLOSE_WAIT");
            break;
        } else if(sendlen > 0 && sendlen < size){
            perror("NOT SEND COMPLETELY");
        }

        int len = recv(s_fd, buf, BUF_SIZE, 0);
        if(len == 0) {
            perror("peer close");
            break;            
        } else if( len < 0) {
            perror("recv error");
        }
        buf[len] = 0;
        printf("recieve: %s \n", buf);
    }

    close(s_fd);
}
