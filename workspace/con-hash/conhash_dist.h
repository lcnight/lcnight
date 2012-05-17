/**
 * =====================================================================================
 *       @file  conhash-dist.h
 *      @brief  header file
 *
 *   @internal
 *     Created  10/12/2011 11:01:59 PM
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#ifndef  __CONHASH_DIST_H__
#define  __CONHASH_DIST_H__

#include <inttypes.h>
#include <stdlib.h>
#include <map>

#define     MAX_HOST_LEN        24
#define     MAX_HASHSTR_LEN     32
typedef struct socket_connection {
    char host[MAX_HOST_LEN];
    int port;
    int fd;
    bool inited; //denote connected(true) or broken(false)
} conn_t;
typedef struct slave_node {
    char hash_str[MAX_HASHSTR_LEN];
    conn_t *ptr;
} slave_node_t;

typedef  uint32_t (*hash_func_t)(const char *str);

class conhash_dist {
    public:
        conhash_dist(hash_func_t callback, int virtual_num = 64);
        ~conhash_dist();
        int add(const char* host, int port);
        slave_node* get_nodeinfo(const uint32_t hash_id);
        int get_nodefd(const uint32_t hash_id);
        void destroy();

        void dump();
    private:
        inline uint32_t hash_inner(uint32_t node_id, int times);
        typedef  std::map<uint32_t, slave_node>             hash_slave_t;
        typedef  std::map<uint32_t, slave_node>::iterator   hash_slave_iter;
        hash_slave_t hash_map;
        hash_func_t cb_func;
        int v_node_num;
        bool destroyed;
};

#endif
