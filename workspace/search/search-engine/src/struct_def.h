/**
 * @file proto.h
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-24
 */

#ifndef _H_PROTO_H_
#define _H_PROTO_H_

#include <stdint.h>
#include <map>

#include "utility/rbtree.h"

#define TITLE_MAX_LEN    40
#define CONTENT_MAX_LEN  80

typedef struct {
    uint32_t did;
#define FLAG_TITLE    0x1
#define FLAG_CONTENT  0x2
    uint32_t flag;
    uint32_t offset[0];
} __attribute__((packed)) redis_index_t;

typedef struct {
    uint32_t keyword_id;
    uint32_t offset;
} offset_data_t;

typedef struct index_data{
    offset_data_t* data;
    int len;
    int flag;
    struct index_data* next;
} index_data_t;

typedef struct {
    struct rb_node tr_node;
    uint32_t rank;
    uint32_t did;
    char* snippet_title;
    char* snippet_content;
    index_data_t* index_list_node;
} did_tr_node_t;

typedef struct {
    struct rb_root did_tr_root;
    struct rb_root rank_tr_root;
} set_tr_node_t;

///比较二进制中1的个数，1的个数多的排在序列的前面
struct set_map_cmp {
    bool operator() (int lhs, int rhs) {
        int lhs_count = get_1_nums(lhs);
        int rhs_count = get_1_nums(rhs);
        if (lhs_count > rhs_count) {
            return true;
        } else if (lhs_count == rhs_count) {
            return lhs < rhs;
        } else {
            return false;
        }
    }
private:
    int get_1_nums(int value) {
        if (value == 0) return 0;

        int a = value;
        int b = value - 1;
        int c = 0;

        int count = 1;

        while ((c = a & b)) {
            count++;
            a = c;
            b = c - 1;
        }

        return count;
    }
};


typedef std::map<int, set_tr_node_t, set_map_cmp> set_data_map_t;
typedef std::map<int, set_tr_node_t, set_map_cmp>::iterator set_data_map_it_t;

struct offset_cmp {
    bool operator() (const offset_data_t& lhs, const offset_data_t& rhs) const {
        return lhs.offset < rhs.offset;
    }
};

#define SEARCH_ENGINE_HINTS_CMD         2001
#define SEARCH_ENGINE_RECOMMEND_CMD     2002
#define SEARCH_ENGINE_SEARCH_CMD        2003

#define SEARCH_ENGINE_ERR               1000
#define SEARCH_ENGINE_NOFIND_HINTS      1001
#define SEARCH_ENGINE_NOFIND_RECOMMEND  1002
#define SEARCH_ENGINE_NOFIND_SEARCH     1003
#define SEARCH_ENGINE_RECOMMED_SEARCH   1004

#endif
