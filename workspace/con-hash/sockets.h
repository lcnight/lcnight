#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include <inttypes.h>

/* ----------------- TCP ----------------- */

int tcpsocket(void);
int tcpnonblock(int sock);
int tcpnodelay(int sock);
int tcpreuseaddr(int sock);
int tcpresolve(const char *hostname,const char *service,uint32_t *ip,uint16_t *port,int passiveflag);
#define     tcpresolve_c(host, service, ip, port)   tcpresolve(host, service, ip, port, 0)
#define     tcpresolve_s(host, service, ip, port)   tcpresolve(host, service, ip, port, 1)
int tcpsetacceptfilter(int sock);
int tcpaccfhttp(int sock);
int tcpaccfdata(int sock);
int tcpnumconnect(int sock,uint32_t ip,uint16_t port);
int tcpstrconnect(int sock,const char *hostname,const char *service);
int tcpnumtoconnect(int sock,uint32_t ip,uint16_t port,uint32_t msecto);
int tcpstrtoconnect(int sock,const char *hostname,const char *service,uint32_t msecto);
int tcpgetstatus(int sock);
int tcpnumbind(int sock,uint32_t ip,uint16_t port);
int tcpstrbind(int sock,const char *hostname,const char *service);
int tcpnumlisten(int sock,uint32_t ip,uint16_t port,uint16_t queue);
int tcpstrlisten(int sock,const char *hostname,const char *service,uint16_t queue);
int tcpaccept(int lsock);
int tcpgetpeer(int sock,uint32_t *ip,uint16_t *port);
int tcpgetmyaddr(int sock,uint32_t *ip,uint16_t *port);
int tcpclose(int sock);
//int32_t tcpread(int sock,void *buff,uint32_t leng);
//int32_t tcpwrite(int sock,const void *buff,uint32_t leng);
int32_t tcptoread(int sock,void *buff,uint32_t leng,uint32_t msecto);
int32_t tcptowrite(int sock,const void *buff,uint32_t leng,uint32_t msecto);

/* ----------------- UDP ----------------- */

int udpsocket(void);
int udpresolve(const char *hostname,const char *service,uint32_t *ip,uint16_t *port,int passiveflag);
int udpnonblock(int sock);
int udpnumlisten(int sock,uint32_t ip,uint16_t port);
int udpstrlisten(int sock,const char *hostname,const char *service);
int udpwrite(int sock,uint32_t ip,uint16_t port,const void *buff,uint16_t leng);
int udpread(int sock,uint32_t *ip,uint16_t *port,void *buff,uint16_t leng);
int udpclose(int sock);

#endif
