/**
 *      @brief  convert between ip address to uint
 *
 *     Created  12/19/2011 02:33:12 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void usage()
{
    printf("exec [options] <IP address>|<IP num>\n"
           "options:\n\t-n\t*net order\n\t-h\thost order\n" );
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || (argc == 2 && argv[1][0] == '-')) {
        usage();
    }

    bool netOrder = true;
    char ch = 0;
    while((ch=getopt(argc, argv, "nh"))!=-1) {
        switch (ch)
        {
        case 'h' :
            netOrder = false;
            break;
        case 'n' :
            netOrder = true;
            break;
        default :
            usage();
            break;
        }  /* end of switch */
    }

    struct in_addr ip_n = {0};
    char *ip_a = 0;

    //if (optind == 1 && argv[optind][0] == '-') {
        //return 0;
    //}

    if (strchr(argv[optind], '.')) {
        inet_aton(argv[optind], &ip_n);
        if (!netOrder) {
            printf("%u\n", ntohl(ip_n.s_addr));
        } else {
            printf("%u\n", ip_n.s_addr);
        }
    } else {
        char *nptr = NULL;
        uint32_t arg = strtoul(argv[optind], &nptr, 10);
        if (!netOrder) {
            ip_n.s_addr = htonl(arg); 
        } else {
            ip_n.s_addr = arg; 
        }
        ip_a = inet_ntoa(ip_n);
        printf("%s\n", ip_a);
    }
    return 0;
}/* -- end of main  -- */
