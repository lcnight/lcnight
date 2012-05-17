/**
 * =====================================================================================
 *       @file  test.cpp
 *      @brief  main file for test
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macro.h"

#include "conhash_dist.h"

uint32_t hash_f(const char *str) {
    return strlen(str);
}

int main(int argc, char *argv[])
{
    conhash_dist tmp(hash_f);
    tmp.add("localhost", 80);
    tmp.add("localhost", 8080);
    uint32_t n_id = 65535;
    int fd = tmp.get_nodefd(n_id);
    PRINT("get fd %d for node id %u", fd, n_id);
    n_id = 4294967296;
    fd = tmp.get_nodefd(n_id);
    PRINT("get fd %d for node id %u", fd, n_id);
    n_id = 4294967295;
    fd = tmp.get_nodefd(n_id);
    PRINT("get fd %d for node id %u", fd, n_id);
    tmp.dump();
    tmp.destroy();
    tmp.destroy();
    tmp.dump();
    return 0;
}
