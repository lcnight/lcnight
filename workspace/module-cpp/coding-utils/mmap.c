/**
 *      @brief  
 *
 *     Created  02/09/2012 07:42:30 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int pz = 4096;
    int fd = open("mfile", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("open error: %d, %s\n", fd, strerror(errno));
        _exit(1);
    }
    ftruncate(fd, pz*1024);
    char* p = (char*)mmap(0, pz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("mmap file to: %p\n", p);
    strcpy(p, "hele");
    printf("get: %s\n", p);
    return 0;
}/* -- end of main  -- */
