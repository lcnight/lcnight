/**
 * =====================================================================================
 *       @file  conhash-dist.cpp
 *      @brief  consistent hash distribution
 *
 *   @internal
 *     Created  10/12/2011 11:01:42 PM
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include "macro.h"
#include "sockets.h"
#include "conhash_dist.h"


conhash_dist::conhash_dist(hash_func_t callback, int virtual_num): cb_func(callback), v_node_num(virtual_num), destroyed(false)
{
    //cb_func = callback;
    //v_node_num = virtual_num;
}

conhash_dist::~conhash_dist()
{
    destroy();
}

inline uint32_t conhash_dist::hash_inner(uint32_t node_id, int times)
{
    if (times < 0 || times >32) {
        return node_id + 1;
    } else {
        return node_id + (1<<times) - 1;
    }
}

int conhash_dist::add(const char* host, int port)
{
    conn_t *sk_conn = (conn_t *)malloc(sizeof(conn_t));
    strcpy(sk_conn->host, host);
    sk_conn->port = port;
    sk_conn->fd = -1;
    sk_conn->inited = false;
    slave_node slave = {{0}, sk_conn};
    if (port <= 0) {
        snprintf(slave.hash_str, MAX_HASHSTR_LEN - 1, "%s", slave.ptr->host);
    } else {
        snprintf(slave.hash_str, MAX_HASHSTR_LEN - 1, "%s:%d", slave.ptr->host, slave.ptr->port);
    }
#define     REHASH_TIMES        32
    uint32_t node_id = cb_func(slave.hash_str);
    if (hash_map.find(node_id) == hash_map.end()) {
        hash_map.insert(std::pair<uint32_t, slave_node>(node_id, slave));
    } else {
        for (int i = 0 ; i < REHASH_TIMES ; i++) {
            node_id = hash_inner(node_id, i);
            if (hash_map.find(node_id) == hash_map.end()) {
                hash_map.insert(std::pair<uint32_t, slave_node>(node_id, slave));
                break;
            }
        }
    }

    for (int i = 0 ; i < v_node_num; i++) {
        slave_node tmp = slave;
        snprintf(tmp.hash_str, MAX_HASHSTR_LEN - 1, "%s:%d#%d", tmp.ptr->host, tmp.ptr->port, i);
        uint32_t tmp_id = cb_func(tmp.hash_str);
        if (hash_map.find(tmp_id) == hash_map.end()) {
            hash_map.insert(std::pair<uint32_t, slave_node>(tmp_id, tmp));
        } else {
            for (int i = 0 ; i < REHASH_TIMES ; i++) {
                tmp_id = hash_inner(tmp_id, i);
                if (hash_map.find(tmp_id) == hash_map.end()) {
                    hash_map.insert(std::pair<uint32_t, slave_node>(tmp_id, tmp));
                    break;
                }
            }
        }
    } /*-- end of for --*/
    return 0;
}

slave_node* conhash_dist::get_nodeinfo(const uint32_t hash_id)
{
    slave_node *result = NULL;
    hash_slave_iter it;
    if ((it = hash_map.lower_bound(hash_id)) != hash_map.end()) {
        result = &it->second;
    } else {
        result = &hash_map.begin()->second;
    }
    return result;
}

int conhash_dist::get_nodefd(const uint32_t hash_id)
{
    slave_node *dst = get_nodeinfo(hash_id);
    if (dst == NULL) {
        return -1;
    }

    if (!dst->ptr) {
        //dst->ptr = (conn_t*)malloc(sizeof(conn_t));
        //dst->ptr->inited = false;
        //dst->ptr->fd = -1;
        return -1;
    }

    int r_fd = -1;
    if (dst->ptr->inited && dst->ptr->fd != -1) {
        r_fd = dst->ptr->fd;
    } else {
#define     E_CONN_TIMEOUT      3*1000
        r_fd = tcpsocket();
        uint32_t ip = 0;
        uint16_t port = dst->ptr->port > 0 ? dst->ptr->port : 0;
        if (tcpresolve_c(dst->ptr->host, NULL, &ip, NULL) != 0
                || tcpnumtoconnect(r_fd, ip, port, E_CONN_TIMEOUT) != 0) {
            tcpclose(r_fd);
            r_fd = -1;
            dst->ptr->inited = false;
        } else {
            dst->ptr->inited = true;
        }
        dst->ptr->fd = r_fd;
    }
    return r_fd;
}

void conhash_dist::destroy()
{
    for (hash_slave_iter it = hash_map.begin(); it != hash_map.end() ; it ++) {
        if (it->second.ptr && it->second.ptr->inited) {
            tcpclose(it->second.ptr->fd);
            it->second.ptr->fd = -1;
            it->second.ptr->inited = false;
            free(it->second.ptr);
        }
    }
    destroyed = true;
}

void conhash_dist::dump()
{
    if (destroyed) {
        PRINT("Consistent hash distribute has been destroyed");
        return;
    }
    PRINT("have node in consistent hash table");
    PRINT("   node id    |       hash string     | status |  fd");
    int i = 0, j = 0;
    for (hash_slave_iter it = hash_map.begin(); it != hash_map.end() ; it ++, i++) {
        PRINT("%11u%25s%11s%5d", it->first, it->second.hash_str, (it->second.ptr->inited ? ++j, "inited" : "-"), it->second.ptr->fd);
    } /*-- end of for --*/
    PRINT("total have %d slave node, %d inited", i, j);
}
