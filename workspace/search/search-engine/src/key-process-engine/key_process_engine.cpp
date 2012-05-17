/**
 * @file key_process_engine.cpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-18
 */
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#include <set>

#include <boost/shared_ptr.hpp>

#include "utility/log.h"
#include "key_process_engine.h"
#include "utility/utility.hpp"
#include "utility/char_to_py.hpp"

int key_process_engine::engine_build(pthread_rwlock_t* rw_lock,
                                     redisContext* redis_conn,
                                     MYSQL* mysql_conn,
                                     char_to_py& char_to_py_inst,
                                     int* p_old_table_id)
{
    if (!rw_lock || !redis_conn || !mysql_conn || !p_old_table_id) {
        return -1;
    }

    int table_id = -1;

    ///阻塞其他所有线程进行io操作
    boost::shared_ptr<pthread_rwlock_t> rw_lock_ptr(
            rw_lock, pthread_rwlock_wrap_unlock());

    ::pthread_rwlock_wrlock(rw_lock_ptr.get());

    ///从mysql中取出当前数据库表id
    if (::mysql_ping(mysql_conn)) {
        ERROR_LOG("mysql_ping failed, %s", ::mysql_error(mysql_conn));
        return -1;
    }

    if (::mysql_query(mysql_conn, "SELECT id FROM t_redis_db ORDER BY i_time DESC LIMIT 1")) {
        ERROR_LOG("mysql_query failed, %s", ::mysql_error(mysql_conn));
        return -1;
    }

    MYSQL_RES* p_res = ::mysql_store_result(mysql_conn);
    if (!p_res) {
        ERROR_LOG("mysql_store_result failed, %s", ::mysql_error(mysql_conn));
        return -1;
    }
    boost::shared_ptr<MYSQL_RES> res_ptr(p_res, mysql_res_wrap_free());

    MYSQL_ROW row = ::mysql_fetch_row(res_ptr.get());
    if (row) {
        table_id = atoi(row[0]);
    } else {
        return -1;
    }

    ///判断新老table_id是否一致，若不一致则更新
    if (*p_old_table_id == table_id) {
        DEBUG_LOG("redis_table_id [%d] not change", table_id);
        return 0;
    } else {
        DEBUG_LOG("redis_table_id [%d] change to [%d]", *p_old_table_id, table_id);
    }

    ///从mysql中获得所有的did=> (view, vote)
    if (::mysql_query(mysql_conn, "SELECT did, view_ts, vote_ts FROM t_doc")) {
        ERROR_LOG("mysql_query failed, %s", ::mysql_error(mysql_conn));
        return -1;
    }

    MYSQL_RES* p_rank_res = ::mysql_store_result(mysql_conn);
    if (!p_rank_res) {
        ERROR_LOG("mysql_store_result failed, %s", ::mysql_error(mysql_conn));
        return -1;
    }
    boost::shared_ptr<MYSQL_RES> rank_res_ptr(p_rank_res, mysql_res_wrap_free());
    MYSQL_ROW rank_row = NULL;

    //std::map<uint32_t, view_vote> tmp_did_vv;
    did_vv.clear();
    while ((rank_row = ::mysql_fetch_row(rank_res_ptr.get()))) {
        if (!rank_row[0] || !rank_row[1] || !rank_row[2]) {
            continue;
        }

        uint32_t did = atoi(rank_row[0]);
        view_vote vv; 
        vv.view = atoi(rank_row[1]);
        vv.vote = atoi(rank_row[2]);

        //DEBUG_LOG("fetch %u: %u %u", did, vv.view, vv.vote);
        //DEBUG_LOG("have %d fetch %u: %u %u", did_vv.size(), did, vv.view, vv.vote);

        doc_vv_it dit = did_vv.find(did);

        if (dit == did_vv.end()) {
            did_vv.insert(pair<uint32_t, view_vote>(did, vv));
        } else {
            dit->second.view = vv.view;
            dit->second.vote = vv.vote;
        }
    }

    ///check whether redis is still alive
    redisReply* reply = (redisReply*)::redisCommand(redis_conn, "PING");
    if (redis_conn->err) {
        ERROR_LOG("redis ping failed, %s", redis_conn->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    ///从redis中取出所有的key
    reply = (redisReply*)::redisCommand(redis_conn, "SELECT %d", table_id);
    if (redis_conn->err) {
        ERROR_LOG("redis select failed, %s", redis_conn->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    reply = (redisReply*)::redisCommand(redis_conn, "KEYS *");
    if (redis_conn->err) {
        ERROR_LOG("redis keys failed, %s", redis_conn->errstr);
        return -1;
    } else if(reply->type != REDIS_REPLY_ARRAY) {
        ::freeReplyObject(reply);
        return -1;
    } else {
        ///
    }
    if (!reply->elements) {
        ERROR_LOG("redis table key empty");
        return -1;
    }

    std::set<std::string, std::greater<std::string> > key_set;
    for (size_t i = 0; i < reply->elements; ++i) {
        key_set.insert(std::string(reply->element[i]->str));
    }
    ::freeReplyObject(reply);

    ///构造trie树
    trie_node_t* p_root = (trie_node_t*) ::calloc(1, sizeof(trie_node_t));
    if (!p_root) {
        ERROR_LOG("calloc failed");
        return -1;
    }

    word_node_t* p_order_list = NULL;

    std::set<std::string>::iterator it = key_set.begin();
    for ( ; it != key_set.end(); ++it) {
        std::string output_py;
        if (char_to_py_inst.conv_char_to_py(*it, output_py) < 0) {
            continue;
        }

        trie_node_t* location = p_root;

        const char* p = output_py.c_str();
        while (location && *p) {
            int offset = 0;
            if (*p >= 'A' && *p <= 'Z') {
                offset = *p - 'A';
            } else if (*p >= 'a' && *p <= 'z') {
                offset = *p - 'a';
            } else {
                goto next;
            }

            trie_node_t* p_parent = location;

            if (!location->branch[offset]) {
                location->branch[offset] = (trie_node_t*) ::calloc(1, sizeof(trie_node_t));
                if (!location->branch[offset]) {
                    goto next;
                }
            }

            location->branch[offset]->parent_node = p_parent;
            location = location->branch[offset];
            p++;
        }

        word_node_t* node;
        node = (word_node_t*) ::calloc(1, sizeof(word_node_t));
        if (!node) {
            continue;
        }

        ///该词语以拼音为key插入trie中，该词的word_rate需要从老的trie树中继承
        node->word = ::strdup((*it).c_str());

        if (m_p_root_) {
            word_node_t* old_node = NULL;
            __trie_exact_search(m_p_root_, output_py, *it, &old_node);
            if (old_node) {
                atomic_set(&node->word_rate, atomic_read(&old_node->word_rate));
            }
        } else {
            atomic_set(&node->word_rate, 0);
        }

        node->next = location->word_list;
        location->word_list = node;

        ///该词语插入顺序链表中
        node->order_list_node = p_order_list;
        p_order_list = node;
next:
        continue;
    }
    ///销毁老的数据结构
    __engine_destroy();

    m_p_root_ = p_root;
    m_p_order_list_ = p_order_list;
    m_word_total_num_ = key_set.size();

    ///销毁redis老表中的所有数据
    reply = (redisReply*)::redisCommand(redis_conn, "SELECT %d", *p_old_table_id);
    *p_old_table_id = table_id;

    if (redis_conn->err) {
        DEBUG_LOG("redis select failed, %s", redis_conn->errstr);
        return 0;
    }
    if (reply->type != REDIS_REPLY_STATUS) {
        DEBUG_LOG("redis select failed, %s", reply->str);
        ::freeReplyObject(reply);
        return 0;
    }
    ::freeReplyObject(reply);

    reply = (redisReply*)::redisCommand(redis_conn, "FLUSHDB");
    ::freeReplyObject(reply);

    return 0;
}

int key_process_engine::get_doc_vv(uint32_t did, view_vote& vv)
{
    doc_vv_it it = did_vv.find(did);
    if (it != did_vv.end()) {
        vv.view = it->second.view;
        vv.vote = it->second.vote;
    }

    return 0;
}

int key_process_engine::engine_key_process(const std::string& input_str,
                                           std::string& output_str,
                                           char_to_py& char_to_py_inst)
{
    ///判断搜索关键字是否是英文或是中文
    int is_en_cn = -1;
    int rt = __key_ch_en_judege(input_str);
    if (rt < 0) {
        return -1;
    } else if (rt == 2) {
        is_en_cn = 2; // 中文
    } else if (rt == 1) {
        is_en_cn = 1; // 英文
    } else {
        is_en_cn = 3; // 中英
    }

    std::string output_py;
    if (char_to_py_inst.conv_char_to_py(input_str, output_py) < 0) {
        ERROR_LOG("conv_char_to_py failed");
        return -1;
    }

    trie_node_t* p_trie_node = NULL;
    __trie_fuzz_search(m_p_root_, output_py, &p_trie_node, true);

    ///判断搜索关键字是否在trie中有一样的
    word_node_t* node = p_trie_node->word_list;
    while (node) {
        if (!::strcasecmp(input_str.c_str(), node->word)) {
            output_str.assign(node->word);
            atomic_inc(&node->word_rate);
            break;
        } else {
            node = node->next;
        }
    }

    ///没有相同的关键字，则根据算法给出一个推荐的关键字
    if (!node) {
        if (is_en_cn == 2) {
            if (__key_fuzz_process_cn(p_trie_node, input_str, output_str) == 1) {
                return -1;
            }
        } else if (is_en_cn == 1 || is_en_cn == 3) {
            if (__key_fuzz_process_en(p_trie_node, input_str, output_str) == 1) {
                return -1;
            }
        }

        word_node_t* word_node = NULL;
        __trie_exact_search(m_p_root_, output_py, output_str, &word_node);
        if (word_node) {
            atomic_inc(&word_node->word_rate);
        }
        return 1;
    }

    return 0;
}

int key_process_engine::engine_get_hinted_keys(const std::string& input_str,
                                               std::vector<std::string>& output_str_vec,
                                               char_to_py& char_to_py_inst)
{
    ///判断搜索关键字是否是英文或是中文
    int is_en_cn = -1;
    int rt = __key_ch_en_judege(input_str);
    if (rt < 0) {
        return -1;
    } else if (rt == 2) {
        is_en_cn = 2;
    } else if (rt == 1) {
        is_en_cn = 1;
    } else {
        ///对于中英文混杂和其他字符混杂的情况现不做处理
        return 1;
    }

    std::string output_py;
    if (char_to_py_inst.conv_char_to_py(input_str, output_py) < 0) {
        ERROR_LOG("conv_char_to_py failed");
        return -1;
    }

    trie_node_t* p_trie_node = NULL;
    __trie_fuzz_search(m_p_root_, output_py, &p_trie_node, false);
    if (!p_trie_node) {
        return 1;
    }

    std::multimap<int, char*, std::greater<int> > hinted_str_map;
    __trie_depth_search(p_trie_node, 6, hinted_str_map);

    if (is_en_cn == 2) {
        std::multimap<int, char*, std::greater<int> >::iterator it;
        std::multimap<int, char*, std::greater<int> > tmp_map;

        for (it = hinted_str_map.begin(); it != hinted_str_map.end(); ++it) {
            if (!::strncasecmp(input_str.c_str(), it->second, input_str.length())) {
                tmp_map.insert(std::pair<int, char*>(it->first, it->second));
            }
        }

        hinted_str_map = tmp_map;
        if (hinted_str_map.size() == 0) {
            return 1;
        }
    }

    ///取得前10个提示
    size_t top10_size = hinted_str_map.size() < 10 ? hinted_str_map.size() : 10;
    std::multimap<int, char*, std::greater<int> >::iterator it = hinted_str_map.begin();
    for (size_t i = 0; i < top10_size; i++, ++it) {
        output_str_vec.push_back(std::string(it->second));
    }

    return 0;
}

int key_process_engine::engine_get_recommend_keys(const std::vector<std::string>& input_str_vec,
                                                  std::vector<std::string>& recommend_str_vec,
                                                  char_to_py& char_to_py_inst)
{
    std::multimap<int, char*, std::greater<int> > recommend_str_map;
    for (size_t i = 0; i < input_str_vec.size(); ++i) {
        std::string output_py;
        if (char_to_py_inst.conv_char_to_py(input_str_vec[i], output_py) < 0) {
            continue;
        }

        trie_node_t* p_trie_node = NULL;
        __trie_fuzz_search(m_p_root_, output_py, &p_trie_node, false);
        if (!p_trie_node) {
            continue;
        }

        word_node_t* node = p_trie_node->word_list;
        while (node) {
            if (!::strncasecmp(input_str_vec[i].c_str(), node->word, input_str_vec[i].length())) {
                break;
            } else {
                node = node->next;
            }
        }
        if (!node) {
            continue;
        }

        __order_list_depth_search(node,
                                  input_str_vec[i],
                                  20,
                                  recommend_str_map);
    }

    ///取得最后10个推荐
    size_t top10_size = recommend_str_map.size() < 10 ? recommend_str_map.size() : 10;
    std::multimap<int, char*,
        std::greater<int> >::reverse_iterator rit = recommend_str_map.rbegin();
    for (size_t i = 0; i < top10_size; ++i, ++rit) {
        recommend_str_vec.push_back(std::string(rit->second));
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void key_process_engine::__engine_destroy()
{
    if (m_p_root_) {
        __trie_release(m_p_root_);
        m_p_root_ = NULL;
    }

    m_p_order_list_ = NULL;
    m_word_total_num_ = 0;
}

int key_process_engine::__key_ch_en_judege(const std::string& input_str)
{
    int n_bytes = 0;

    bool pure_en = false;
    bool pure_cn = false;

    for (size_t i = 0; i < input_str.length(); i++) {
        u_char tmp = (u_char)input_str[i];
        if (tmp < 0x80) {
            if ((tmp <= 'Z' && tmp >= 'A') || (tmp <= 'z' && tmp >= 'a')) {
                pure_en = true;
            } else {
                pure_en = false;
                pure_cn = false;
            }
        } else {
            pure_cn = true;
            if (n_bytes == 0) {
                if (tmp >= 0xFC && tmp <= 0xFD) {
                    n_bytes = 6;
                } else if (tmp >= 0xF8) {
                    n_bytes = 5;
                } else if (tmp >= 0xF0) {
                    n_bytes = 4;
                } else if (tmp >= 0xE0) {
                    n_bytes = 3;
                } else if (tmp >= 0xC0) {
                    n_bytes = 2;
                } else {
                    return -1;
                }
                n_bytes--;
            } else {
                if ((tmp & 0xC0) != 0x80) {
                    return -1;
                }
                n_bytes--;
            }
        }
    }

    if (n_bytes > 0) {
        return -1;
    }

    if (pure_cn && pure_en) {
        return 3;
    } else if (pure_cn && !pure_en) {
        return 2;
    } else if (!pure_cn && pure_en) {
        return 1;
    } else {
        return 0;
    }
}

int key_process_engine::__key_coherence_calcu(const std::string& target_str,
                                              const std::string& candidate_str)
{
    u_char** matrix = new u_char*[target_str.length() + 1];
    for (size_t i = 0; i < target_str.length() + 1; i++) {
        matrix[i] = new u_char[candidate_str.length() + 1];
    }

    for (size_t i = 0; i < target_str.length() + 1; i++) {
        matrix[i][0] = (u_char)i;
    }

    for (size_t i = 0; i < candidate_str.length() + 1; i++) {
        matrix[0][i] = 0;
    }

    for (size_t i = 1; i <= target_str.length(); i++) {
        for (size_t j = 1; j <= candidate_str.length(); j++) {
            if (candidate_str[i-1] == target_str[j-1]) {
                matrix[i][j] = matrix[i-1][j-1];
            } else {
                int min
                    = matrix[i-1][j-1] < matrix[i-1][j] ? matrix[i-1][j-1] : matrix[i-1][j];
                min = min < matrix[i][j-1] ? min : matrix[i][j-1];
                matrix[i][j] = (u_char)(min + 1);
            }
        }
    }

    int coherence = matrix[target_str.length()][candidate_str.length()];

    for (size_t i = 0; i < target_str.length() + 1; i++) {
        delete []matrix[i];
    }
    delete []matrix;
    matrix = NULL;

    return coherence;
}

int key_process_engine::__key_fuzz_process_en(trie_node_t* p_root,
                                              const std::string& input_str,
                                              std::string& output_str)
{
    std::multimap<int, char*, std::greater<int> > str_map;

    word_node_t* node = p_root->word_list;
    if (node) {
        while (node) {
            int rate = atomic_read(&node->word_rate);
            str_map.insert(
                    std::pair<int, char*>(rate, node->word));
            node = node->next;
        }
    } else {
        __trie_depth_search(p_root, 6, str_map);
    }

    if (str_map.size() == 0) {
        return 1;
    }

    output_str.assign(str_map.begin()->second);
    return 0;
}

int key_process_engine::__key_fuzz_process_cn(trie_node_t* p_root,
                                              const std::string& input_str,
                                              std::string& output_str)
{
    std::multimap<int,char*, std::greater<int> > str_map;

    word_node_t* node = p_root->word_list;
    while (node) {
        int coherence = __key_coherence_calcu(input_str, node->word);
        str_map.insert(std::pair<int, char*>(coherence, node->word));
        node = node->next;
    }

    if (str_map.size() == 0) {
        //__trie_depth_search(p_root, 5, str_map);
        //output_str.assign(str_map.begin()->second);
        return 1;
    } else {
        output_str.assign(str_map.rbegin()->second);
    }

    return 0;
}

void key_process_engine::__trie_release(trie_node_t* p_root)
{
    for (int i = 0; i < num_chars; i++) {
        if (p_root->branch[i]) {
            __trie_release(p_root->branch[i]);
        }
    }

    word_node_t *node = p_root->word_list;
    while (node) {
        word_node_t* tmp = node;
        node = node->next;

        ::free(tmp->word);
        ::free(tmp);
    }

    ::free(p_root);
}

void key_process_engine::__trie_fuzz_search(trie_node_t* p_root,
                                            const std::string& key_str,
                                            trie_node_t** p_trie_node,
                                            bool is_backtrace)
{
    trie_node_t* location = p_root;
    trie_node_t* parent = NULL;

    const char* p = key_str.c_str();

    while (location && *p) {
        int offset = 0;
        if (*p >= 'A' && *p <= 'Z') {
            offset = *p - 'A';
        } else if (*p >= 'a' && *p <= 'z') {
            offset = *p - 'a';
        } else {
            break;
        }

        parent = location;
        location = location->branch[offset];
        p++;
    }

    if (location) {
        *p_trie_node = location;
        return;
    } else {
        if (is_backtrace) {
            *p_trie_node = parent;
            return;
        } else {
            *p_trie_node = NULL;
            return;
        }
    }
}

void key_process_engine::__trie_exact_search(trie_node_t* p_root,
                                             const std::string& key_str,
                                             const std::string& value_str,
                                             word_node_t** p_word_node)
{
    trie_node_t* location = p_root;
    const char* p = key_str.c_str();

    while (location && *p) {
        int offset = 0;
        if (*p >= 'A' && *p <= 'Z') {
            offset = *p - 'A';
        } else if (*p >= 'a' && *p <= 'z') {
            offset = *p - 'a';
        } else {
            *p_word_node = NULL;
            return;
        }

        location = location->branch[offset];
        p++;
    }

    if (location) {
        word_node_t* node = location->word_list;
        while (node) {
            if (!::strcmp(value_str.c_str(), node->word)) {
                *p_word_node = node;
                return;
            } else {
                node = node->next;
            }
        }
    }

    *p_word_node = NULL;
}

void key_process_engine::__trie_depth_search(trie_node_t* p_root,
                                             int depth,
                                             std::multimap<int, char*, std::greater<int> >& str_map)
{
    for (int i = 0; i < num_chars; i++) {
        if (p_root->branch[i] && (depth > 0)) {
            __trie_depth_search(p_root->branch[i], depth-1, str_map);
        }
    }

    word_node_t* node = p_root->word_list;
    while (node) {
        int rate = atomic_read(&node->word_rate);
        str_map.insert(std::pair<int, char*>(rate, node->word));

        node = node->next;
    }

}

void key_process_engine::__order_list_depth_search(word_node_t* p_order_list,
                                                   const std::string& cmp_str,
                                                   int depth,
                                                   std::multimap<int, char*, std::greater<int> >& str_map)
{
    word_node_t* node = p_order_list;
    while (node && (depth-- > 0)) {
        if (!::strncasecmp(cmp_str.c_str(), node->word, cmp_str.length())) {
            int rate = atomic_read(&node->word_rate);
            str_map.insert(std::pair<int, char*>(rate, node->word));
            node = node->order_list_node;
        } else {
            break;
        }
    }
}

#if 0
int key_process_engine::engine_get_rank(int did) {
    std::map<int, int>::iterator it = m_id_rank_map_.find(did);
    if (it == m_id_rank_map_.end()) {
        return 0;
    } else {
        return it->second;
    }
}
#endif
