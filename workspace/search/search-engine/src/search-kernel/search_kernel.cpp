/**
 * @file search_kernel.cpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-28
 */
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <algorithm>

#include "utility/log.h"
#include "utility/rbtree.h"
#include "utility/utility.hpp"
#include "utility/char_to_py.hpp"
#include "key-process-engine/key_process_engine.h"
#include "word-segment/word_segment_pool.hpp"

#include "search_kernel.h"

int search_kernel::kernel_search_process(const std::vector<std::string>& keyword_vec,
                                         redisContext* redis_conn,
                                         MYSQL* mysql_conn,
                                         key_process_engine& key_proc,
                                         int table_id,
                                         sorted_docs& set_data_map, 
                                         rank_docid& rk_docs)
{
    if (!redis_conn || ! mysql_conn) {
        return -1;
    }

#ifdef _DEBUG
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
#endif

    ///从redis中将相关key的所有反向索引都取出
    if (__redis_get_index(keyword_vec,
                          set_data_map,
                          redis_conn,
                          table_id) < 0) {
        ERROR_LOG("redis get index failed");
        return -1;
    }

#ifdef _DEBUG
    gettimeofday(&end_time, NULL);
    DEBUG_LOG("redis get index time[%ld]", end_time.tv_usec - start_time.tv_usec);
    gettimeofday(&start_time, NULL);
#endif

    __index_doc_sort(key_proc, set_data_map, rk_docs);

#ifdef _DEBUG
    gettimeofday(&end_time, NULL);
    DEBUG_LOG("doc rank sort time[%ld]", end_time.tv_usec - start_time.tv_usec);
#endif

    return 0;
}

int search_kernel::get_snippet_frm_mysql(rank_doc& selected_docs, MYSQL* mysql_conn)
{
    if (selected_docs.size() == 0) {
        return 0;
    }

    string sql_str;
    char sql_tmp[4096] = {0};
    re_rank_doc_it sit;
    for (sit = selected_docs.rbegin(); sit != selected_docs.rend() ; ++sit) {
        uint32_t docid = sit->second.did;
        DEBUG_LOG("get_snippet: rank %u docid %u", sit->first, docid);
        keyword_offset_it off_it;
        uint32_t title_off = MAX_OFFSET;
        for (off_it = sit->second.off_title.begin() ; off_it != sit->second.off_title.end() ; ++off_it) {
            if ((*(off_it->second.begin()) + 1) < title_off) {
                title_off = *(off_it->second.begin()) + 1;
            }
        } /*-- end of for --*/
        title_off = (title_off < 10 || title_off == MAX_OFFSET) ? 1 : (title_off - 9);
        
        uint32_t content_off = MAX_OFFSET;
        for (off_it = sit->second.off_content.begin() ; off_it != sit->second.off_content.end() ; ++off_it) {
            if ((*(off_it->second.begin()) + 1) < content_off) {
                content_off = *(off_it->second.begin()) + 1;
            }
        } /*-- end of for --*/
        content_off = (content_off < 10 || content_off == MAX_OFFSET) ? 1 : (content_off - 9);

        snprintf(sql_tmp, sizeof(sql_tmp) - 1, 
                "select SUBSTRING(title, %u, %d), SUBSTRING(content, %u, %d) from t_doc where did = %u;", 
                title_off, TITLE_MAX_LEN , content_off, CONTENT_MAX_LEN, docid);
        sql_str.append(sql_tmp);
    } /*-- end of for --*/

    ///在mysql中多查询执行，减少rtt等待时间
    if (::mysql_ping(mysql_conn)) {
        ERROR_LOG("mysql ping failed, %s", ::mysql_error(mysql_conn));
        return -1;
    }

    //DEBUG_LOG("sql: %s", sql_str.c_str());
    if (::mysql_query(mysql_conn, sql_str.c_str())) {
        ERROR_LOG("mysql_query failed(%s) sql: %s", ::mysql_error(mysql_conn), sql_str.c_str());
        return -1;
    }

    ///取出每一条sql语句的查询结果
    for (sit = selected_docs.rbegin(); sit != selected_docs.rend(); ++sit) {
        MYSQL_RES* p_res = ::mysql_store_result(mysql_conn);
        if (!p_res) {
            ERROR_LOG("mysql_store_result failed, %s", ::mysql_error(mysql_conn));
            continue;
        }

        boost::shared_ptr<MYSQL_RES> res_ptr(p_res, mysql_res_wrap_free());

        MYSQL_ROW row = ::mysql_fetch_row(res_ptr.get());
        if (row) {
            if (row[0]) (sit)->second.snippt_title = ::strdup(row[0]);
            if (row[1]) (sit)->second.snippt_content = ::strdup(row[1]);
        }
        if (::mysql_next_result(mysql_conn)) {
            break;
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
int search_kernel::merge_doc_keyword(sorted_docs& set_data_map, 
        string& keyword, redis_index_t *doc_keys, int off_num)
{
    uint32_t did = doc_keys->did;

    //DEBUG_LOG("fetch did %u : times %d : num %d : %s", did, i, off_num, keyword.c_str());

    sorted_docs_it it = set_data_map.find(did);
    if (it != set_data_map.end()) {
        keyword_offset_it key_it;
        if (doc_keys->flag == FLAG_TITLE) {
            key_it = it->second.off_title.find(keyword);
            if (key_it != it->second.off_title.end()) {
                for (int j = 0; j < off_num ; ++j) {
                    key_it->second.insert(doc_keys->offset[j]);
                } /*-- end of for --*/
            } else {
                it->second.off_title.insert(pair<string, offsets>(
                            keyword, offsets(doc_keys->offset, doc_keys->offset + off_num)));
            }
        } 
        else if (doc_keys->flag == FLAG_CONTENT) {
            key_it = it->second.off_content.find(keyword);
            if (key_it != it->second.off_content.end()) {
                for (int j = 0; j < off_num ; ++j) {
                    key_it->second.insert(doc_keys->offset[j]);
                } /*-- end of for --*/
            } else {
                it->second.off_content.insert(pair<string, offsets>(
                            keyword, offsets(doc_keys->offset, doc_keys->offset + off_num)));
            }
        } else {
            ERROR_LOG("get zset item with unknown flag: %u", doc_keys->flag);
            return -1;
        }
    } else {
        doc_info info;
        info.did = did;

        if (doc_keys->flag == FLAG_TITLE) {
            info.off_title.insert(pair<string, offsets>(
                        keyword, offsets(doc_keys->offset, doc_keys->offset + off_num)));
        } 
        else if (doc_keys->flag == FLAG_CONTENT) {
            info.off_content.insert(pair<string, offsets>(
                        keyword, offsets(doc_keys->offset, doc_keys->offset + off_num)));
        } else {
            ERROR_LOG("get zset item with unknown flag: %u", doc_keys->flag);
            return -1;
        }

        set_data_map.insert(pair<uint32_t, doc_info>(did, info));
    }

    return 0;
}


int search_kernel::__redis_get_index(const std::vector<std::string>& keyword_vec,
        sorted_docs& set_data_map, redisContext* redis_conn, int table_id)
{
    ///redis使用pipeline将多个请求封装成一个包体
    redisReply* reply = NULL;
    ::redisAppendCommand(redis_conn, "PING");
    ::redisAppendCommand(redis_conn, "SELECT %d", table_id);
    for (size_t i = 0; i < keyword_vec.size(); i++) {
        ::redisAppendCommand(redis_conn, "ZRANGE %s 0 -1", keyword_vec[i].c_str());
    }

    if (::redisGetReply(redis_conn, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis ping failed, %s", redis_conn->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    if (::redisGetReply(redis_conn, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis select failed, %s", redis_conn->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    ///从redis中取出结果
    size_t s = 0;
    while (s++ < keyword_vec.size() && ::redisGetReply(redis_conn, (void**)&reply) == REDIS_OK) {
        if (reply->type != REDIS_REPLY_ARRAY) {
            ERROR_LOG("redis zrange type not array, %s", redis_conn->errstr);
            goto err;
        }

        // fetch each <docid:type:offset-list> for one keyword
        for (int i = 0 ; (uint32_t) i < reply->elements ; ++i) {
            redis_index_t *tmp = (redis_index_t *)reply->element[i]->str;

            uint32_t reply_len = reply->element[i]->len;
            int off_num = (int)((reply_len - sizeof(redis_index_t)) / sizeof(uint32_t));

            string keyword(keyword_vec[s - 1].c_str());
            
            merge_doc_keyword(set_data_map, keyword, tmp, off_num);
        } /*-- end of for --*/
err:
        ::freeReplyObject(reply);
    }

    return 0;
}

int search_kernel::regular_keywords(redisContext* redis_conn, int table_id, 
        scws_t m_p_scws_, char_to_py& py_inst, key_process_engine& m_key_proc_,
        const keyword_vector& raw_keywords, keyword_vector& real_keywords, sorted_docs& set_data_map)
{
    keyword_vector exists_keys;
    keyword_vector need_split_keys;

    /// test 输入检索词是否已经是关键词
    redisReply* reply = NULL;
    ::redisAppendCommand(redis_conn, "SELECT %d", table_id);
    for (size_t i = 0; i < raw_keywords.size(); i++) {
        ::redisAppendCommand(redis_conn, "ZRANGE %s 0 -1", raw_keywords[i].c_str());
    }

    if (::redisGetReply(redis_conn, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis select failed, %s", redis_conn->errstr);
        return SEARCH_ENGINE_ERR;
    }
    ::freeReplyObject(reply);

    ///从redis中取出结果
    size_t s = 0;
    while (s++ < raw_keywords.size() && ::redisGetReply(redis_conn, (void**)&reply) == REDIS_OK) {
        if (reply->type != REDIS_REPLY_ARRAY) {
            ERROR_LOG("redis zrange type not array, %s", redis_conn->errstr);
            ::freeReplyObject(reply);
            return SEARCH_ENGINE_ERR;
        }

        string keyword(raw_keywords[s - 1].c_str());
        if (reply->elements == 0) {
            DEBUG_LOG("not keys: %s", keyword.c_str());
            need_split_keys.push_back(keyword);
            continue;
        } else {
            DEBUG_LOG("exists keys: %s", keyword.c_str());
            exists_keys.push_back(keyword);
        }

        // fetch each <docid:type:offset-list> for one keyword
        for (int i = 0 ; (uint32_t) i < reply->elements ; ++i) {
            redis_index_t *tmp = (redis_index_t *)reply->element[i]->str;

            uint32_t reply_len = reply->element[i]->len;
            int off_num = (int)((reply_len - sizeof(redis_index_t)) / sizeof(uint32_t));

            merge_doc_keyword(set_data_map, keyword, tmp, off_num);
        } /*-- end of for --*/
        ::freeReplyObject(reply);
    }

    ///// 过长的关键字 再次split
    //for (keyword_vector_it key_it = exists_keys.begin(); key_it != exists_keys.end(); ++key_it) {
        //if (key_it->size() > 9) {
            //need_split_keys.push_back(*key_it);
        //}
    //}

    // tmp store swcs splited keywords
    keyword_vector splitted_keyword_str_vec;
    for (size_t i = 0; i < need_split_keys.size(); ++i) {
        std::vector<std::string> tmp_vec;
        if (word_segment_pool::segment_word(m_p_scws_, need_split_keys[i], tmp_vec) < 0) {
            ERROR_LOG("[%s] word segment failed", need_split_keys[i].c_str());
            continue;
        }
        splitted_keyword_str_vec.insert(splitted_keyword_str_vec.end(),
                tmp_vec.begin(), tmp_vec.end());
    }
    for (size_t i = 0; i < splitted_keyword_str_vec.size(); i++) {
        DEBUG_LOG("splitted keyword[%s]", splitted_keyword_str_vec[i].c_str());
    }

    real_keywords.swap(exists_keys);
    ///对每一个所要搜索的关键字进行处理，如果是第一次搜索则需要对关键字进行处理
    for (size_t i = 0; i < splitted_keyword_str_vec.size(); ++i) {
        /// 过滤掉分隔出来的stopword
        if (stopwords.find(splitted_keyword_str_vec[i]) != stopwords.end()) {
            DEBUG_LOG("filter stopword: %s", splitted_keyword_str_vec[i].c_str());
            continue;
        }

        std::string tmp_str;
        int rt = m_key_proc_.engine_key_process(splitted_keyword_str_vec[i], tmp_str, py_inst);
        if (rt < 0) {
            DEBUG_LOG("keyword[%s] cannot find in dict", splitted_keyword_str_vec[i].c_str());
            continue;
        } else {
            real_keywords.push_back(tmp_str);
        }
    }

    /// 限制关键词不超过7个
    if (real_keywords.size() > 7) {
        real_keywords.resize(7);
    }
    for (size_t i = 0; i < real_keywords.size(); i++) {
        DEBUG_LOG("processed keyword[%s]", real_keywords[i].c_str());
    }

    if (real_keywords.size() == 0) {
        return SEARCH_ENGINE_ERR;
    } else {
        return 0;
    }
}

void search_kernel::__index_doc_sort(key_process_engine& key_proc, sorted_docs& doc_map, rank_docid& rk_docs)
{
    sorted_docs_it it = doc_map.begin();
    for ( ; it != doc_map.end(); ++it) {
        uint32_t did = it->first;

        view_vote vv;
        key_proc.get_doc_vv(did, vv);

        uint32_t rank = get_rank(did, it->second, vv); 

        rk_docs.insert(pair<uint32_t, uint32_t>(rank, did));
    } /*-- end of for --*/
}

