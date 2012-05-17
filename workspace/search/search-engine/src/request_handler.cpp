/**
 * @file request_handler.cpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-12-03
 */
#include <pthread.h>
#include <sys/time.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <boost/shared_ptr.hpp>

#include "request_handler.h"
#include "utility/log.h"
#include "utility/doc_sort.hpp"
#include "utility/char_to_py.hpp"
#include "key-process-engine/key_process_engine.h"
#include "search-kernel/search_kernel.h"
#include "word-segment/word_segment_pool.hpp"
#include "utility/utility.hpp"
#include "server-skeleton/tcp_connection.hpp"

const char* CRLF = "\r\n";

request_handler::request_handler(key_process_engine& key_proc,
                                 search_kernel& search_proc,
                                 char_to_py& char_to_py_inst,
                                 redisContext* redis_conn,
                                 MYSQL* mysql_conn,
                                 scws_t p_scws,
                                 pthread_rwlock_t* rw_lock,
                                 int* redis_table_id,
                                 int redis_cache_table,
                                 const std::string& url_str)
    : m_key_proc_(key_proc),
      m_search_proc_(search_proc),
      m_char_to_py_inst_(char_to_py_inst),
      m_redis_conn_(redis_conn),
      m_redis_table_id_(redis_table_id),
      m_redis_cache_table_(redis_cache_table),
      m_mysql_conn_(mysql_conn),
      m_p_scws_(p_scws),
      m_rwlock_(rw_lock),
      m_snippet_url_(url_str)
{

}

int request_handler::handle_request(const tcp_connection_ptr_t& conn_ptr, void* buffer, size_t len)
{
    if (!buffer || len < 0) {
        return -1;
    }

    char* pack_start = (char*)buffer;
    do {
        ::pthread_rwlock_rdlock(m_rwlock_);

        boost::shared_ptr<pthread_rwlock_t> rw_lock_ptr(
                m_rwlock_, pthread_rwlock_wrap_unlock());

        char* pack_end = std::search(pack_start, pack_start + len, CRLF, CRLF + 2);
        if (pack_end == pack_start + len) {
            return len;
        }

        ///处理协议，解析json格式
        std::string pack_str(pack_start, pack_end - pack_start);

        json_object* pack_obj = ::json_tokener_parse(pack_str.c_str());
        if (!pack_obj || is_error(pack_obj)) {
            ERROR_LOG("json_tokener_parse failed, %s", pack_str.c_str());
            return -1;
        }

        json_object* cmd_obj = ::json_object_object_get(pack_obj, "cmd_id");
        if (!cmd_obj || is_error(cmd_obj)) {
            ::json_object_put(pack_obj);
            return -1;
        }
        int cmd_id = ::json_object_get_int(cmd_obj);

        std::string resp_json_str;

        switch(cmd_id) {
            case SEARCH_ENGINE_HINTS_CMD:
                __request_get_hints(pack_obj, resp_json_str);
                break;
            case SEARCH_ENGINE_RECOMMEND_CMD:
                __request_get_recommend(pack_obj, resp_json_str);
                break;
            case SEARCH_ENGINE_SEARCH_CMD:
#ifdef _DEBUG
                struct timeval start_time, end_time;
                gettimeofday(&start_time, NULL);
#endif
                __request_get_search(pack_obj, resp_json_str);
#ifdef _DEBUG
                gettimeofday(&end_time, NULL);
                DEBUG_LOG("request search time [%ld]", end_time.tv_usec - start_time.tv_usec);
#endif
                break;
            default:
                __request_get_error(pack_obj, resp_json_str);
                break;
        }

        ::json_object_put(pack_obj);

        ///发送数据
		resp_json_str += CRLF;
        conn_ptr->send_data((void*)(resp_json_str.c_str()), resp_json_str.length());
        len -= pack_str.length() + 2;
        pack_start += pack_str.length() + 2;

    } while (len > 0);

    return 0;
}

int request_handler::__request_get_hints(json_object* obj, std::string& resp_json_str)
{
    std::string keyword_str;
    std::vector<std::string> hints_str_vec;
    int status = 0;
    json_object* resp_object = ::json_object_new_object();
    json_object* hints_array = NULL;;

    json_object* keyword_obj = ::json_object_object_get(obj,"keyword");
    if (!keyword_obj || is_error(keyword_obj)) {
        status = SEARCH_ENGINE_ERR;
        goto err;
    }

    keyword_str = ::json_object_get_string(keyword_obj);

    int rt;
    rt = m_key_proc_.engine_get_hinted_keys(keyword_str, hints_str_vec, m_char_to_py_inst_);
    if (rt < 0) {
        ERROR_LOG("engine get hinted keys failed");
        status = SEARCH_ENGINE_ERR;
        goto err;
    } else if (rt == 1) {
        status = SEARCH_ENGINE_NOFIND_HINTS;
        goto err;
    } else {
        ///
    }

    hints_array = ::json_object_new_array();
    for (size_t i = 0; i < hints_str_vec.size(); i++) {
        ::json_object_array_add(hints_array,
                ::json_object_new_string(hints_str_vec[i].c_str()));
    }
    ::json_object_object_add(resp_object, "keyword_hints", hints_array);

err:
    ::json_object_object_add(resp_object,
            "cmd_id", ::json_object_new_int(SEARCH_ENGINE_HINTS_CMD));

    ::json_object_object_add(resp_object, "status", ::json_object_new_int(status));
    resp_json_str = ::json_object_to_json_string(resp_object);

    ::json_object_put(resp_object);

    return 0;
}

int request_handler::__request_get_search(json_object* obj, std::string& resp_json_str)
{
    // tmp store recieved keywords
    std::vector<std::string> keyword_str_vec;
    // store real retrieved keywords
    std::vector<std::string> keyword_proc_str_vec;

    int status = 0;
    int page_num = 0;
    int per_page_cnt = 0;
    int total_count = 0;

    json_object *keywords_obj, *page_num_obj, *result_count_obj;

    sorted_docs docs_map;
    rank_docid rk_docids;
    rank_doc selected_docs;
    vector<string> recommends_str_vec;
    re_rank_docid_it rk_doc_it;

    string log_req_str;

    int rt = 0;

    ///解析json协议
    keywords_obj = ::json_object_object_get(obj, "keywords");
    if (!keywords_obj || is_error(keywords_obj)) {
        status = SEARCH_ENGINE_ERR;
        goto err;
    }

    for (int i = 0; i < ::json_object_array_length(keywords_obj); i++) {
        json_object* obj = ::json_object_array_get_idx(keywords_obj, i);
        string key_item(::json_object_get_string(obj));
        keyword_str_vec.push_back(key_item);
        log_req_str.append(key_item);
        log_req_str.append(" ");
    }
    DEBUG_LOG("retrieve phrase: [%s]", log_req_str.c_str());

    page_num_obj = ::json_object_object_get(obj, "page_num");
    if (!page_num_obj || is_error(page_num_obj)) {
        status = SEARCH_ENGINE_ERR;
        goto err;
    }
    page_num = ::json_object_get_int(page_num_obj);

    result_count_obj = ::json_object_object_get(obj, "result_per_page");
    if (!result_count_obj || is_error(result_count_obj)) {
        status = SEARCH_ENGINE_ERR;
        goto err;
    }
    per_page_cnt = ::json_object_get_int(result_count_obj);

    ///将搜索关键字存入redis中
    __redis_put_search_words(keyword_str_vec);

    ///将关键词进行规范化处理
    if (page_num == 0) {
        status = m_search_proc_.regular_keywords(m_redis_conn_, *m_redis_table_id_,
                m_p_scws_, m_char_to_py_inst_, m_key_proc_, 
                keyword_str_vec, keyword_proc_str_vec, docs_map);
        if (status) {
            goto err;
        }
    } else {
        keyword_proc_str_vec.swap(keyword_str_vec);
    }

    ///计算所有搜索结果并排序
    if (m_search_proc_.kernel_search_process(keyword_proc_str_vec,
                m_redis_conn_, m_mysql_conn_, m_key_proc_, *m_redis_table_id_,
                docs_map, rk_docids) < 0) {
        status = SEARCH_ENGINE_ERR;
        goto err;
    }
    
    total_count = rk_docids.size();

#ifdef _DEBUG
    DEBUG_LOG("page index %u per-page-num %u", page_num, per_page_cnt);
#endif
    ///取出指定范围内的文章编号
    rk_doc_it = rk_docids.rbegin();
    for (int i = 0; rk_doc_it != rk_docids.rend() ; ++rk_doc_it, ++i) {
        if (i >= page_num * per_page_cnt && i < (page_num + 1) * per_page_cnt) {
            uint32_t docid = rk_doc_it->second;
            sorted_docs_it it = docs_map.find(docid);
            if (it == docs_map.end()) {
                ERROR_LOG(" <rank fetch> why unmatch did %u", docid);
                continue;
            }
            selected_docs.insert(pair<uint32_t, doc_info>(rk_doc_it->first, it->second));
       }
       else if (i >= (page_num + 1) * per_page_cnt) {
           break;
       } else {
           continue;
       }
    } /*-- end of for --*/
    
#ifdef _DEBUG
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
#endif

    ///从mysql中取出snippet_title和snippet_content
    if (m_search_proc_.get_snippet_frm_mysql(selected_docs, m_mysql_conn_) < 0) {
        ERROR_LOG("mysql get snippet failed");
        status = SEARCH_ENGINE_ERR;
        goto err;
    }

#ifdef _DEBUG
    gettimeofday(&end_time, NULL);
    DEBUG_LOG("mysql_get_snippet time[%ld]", end_time.tv_usec - start_time.tv_usec);
#endif

    rt = m_key_proc_.engine_get_recommend_keys(keyword_proc_str_vec,
                                               recommends_str_vec,
                                               m_char_to_py_inst_);
    if (rt < 0) {
        ERROR_LOG("engine get recommend keys failed");
        status = SEARCH_ENGINE_ERR;
        goto err;
    }

    if (recommends_str_vec.size() == 0) {
        DEBUG_LOG("engine nofind recommend keys");
        status = SEARCH_ENGINE_NOFIND_RECOMMEND;
        goto err;
    }

err:
    ///将搜索结果组成json协议
    build_search_json_result(status, total_count, keyword_proc_str_vec, 
            recommends_str_vec, selected_docs, resp_json_str);

    return 0;
}

int request_handler::__redis_put_search_words(const std::vector<std::string>& search_words)
{
    redisReply* reply = NULL;
    ::redisAppendCommand(m_redis_conn_, "PING");
    ::redisAppendCommand(m_redis_conn_, "SELECT %d", m_redis_cache_table_);

    for (size_t i = 0; i < search_words.size(); i++) {
        ::redisAppendCommand(m_redis_conn_,
                             "ZINCRBY search_words 1 %s",
                             search_words[i].c_str());
    }

    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis ping failed, %s", m_redis_conn_->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis select failed, %s", m_redis_conn_->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    size_t s = 0;
    while (s++ < search_words.size()
            && ::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_OK) {
        ///consume all the reply
        ::freeReplyObject(reply);
    }

    return 0;
}

int request_handler::__request_get_error(json_object* obj, std::string& resp_json_str)
{
    json_object* cmd_obj = ::json_object_object_get(obj, "cmd_id");
    int cmd_id = ::json_object_get_int(cmd_obj);

    json_object* resp_obj = ::json_object_new_object();
    ::json_object_object_add(resp_obj, "cmd_id", ::json_object_new_int(cmd_id));
    ::json_object_object_add(resp_obj, "status", ::json_object_new_int(SEARCH_ENGINE_ERR));

    resp_json_str = ::json_object_to_json_string(resp_obj);

    ::json_object_put(resp_obj);

    return 0;
}

void request_handler::build_search_json_result(int status, int total_cnt, 
        vector<string>& keywords, vector<string>& recomends, 
        rank_doc& selected_docs, string& json_str)
{
    json_object* resp_object = ::json_object_new_object();

    ::json_object_object_add(resp_object, "cmd_id", ::json_object_new_int(SEARCH_ENGINE_SEARCH_CMD));
    ::json_object_object_add(resp_object, "status", ::json_object_new_int(status));

    if (status == 0) {
        ::json_object_object_add(resp_object, "search_result_num", ::json_object_new_int(total_cnt));

        json_object* keyword_array = NULL;
        keyword_array = ::json_object_new_array();
        for (size_t i = 0; i < keywords.size(); i++) {
            ::json_object_array_add(keyword_array, ::json_object_new_string(keywords[i].c_str()));
        }
        ::json_object_object_add(resp_object, "search_keywords", keyword_array);


        char did_buf[16] = {0};
        json_object* snippet_array = ::json_object_new_array();
        for (re_rank_doc_it it = selected_docs.rbegin() ; it != selected_docs.rend() ; ++it) {
            json_object* snippet = ::json_object_new_object();

            string snippet_url(m_snippet_url_);
            snprintf(did_buf, 15, "%d", it->second.did);
            snippet_url.append(did_buf);
            ::json_object_object_add(snippet, 
                    "snippet_url", ::json_object_new_string(snippet_url.c_str()));

            string snippet_title(it->second.snippt_title);
            ::json_object_object_add(snippet, 
                    "snippet_title", ::json_object_new_string(snippet_title.c_str()));

            string snippet_content(it->second.snippt_content);
            ::json_object_object_add(snippet, 
                    "snippet_content", ::json_object_new_string(snippet_content.c_str()));

            ::json_object_array_add(snippet_array, snippet);

        } /*-- end of for --*/
        ::json_object_object_add(resp_object, "snippet", snippet_array);

        json_object* recommends_array = NULL;
        recommends_array = ::json_object_new_array();
        for (size_t i = 0; i < recomends.size(); i++) {
            ::json_object_array_add(recommends_array,
                    ::json_object_new_string(recomends[i].c_str()));
        }
        ::json_object_object_add(resp_object, "recommend_keywords", recommends_array);
    }

    json_str = ::json_object_to_json_string(resp_object);
    ::json_object_put(resp_object);
}

int request_handler::__request_get_recommend(json_object* obj, std::string& resp_json_str)
{
    std::vector<std::string> keyword_str_vec;
    std::vector<std::string> recommends_str_vec;
    int status = 0;
    json_object* resp_object = ::json_object_new_object();
    json_object* recommends_array = NULL;;

    json_object* keywords_obj = ::json_object_object_get(obj, "keywords");
    if (!keywords_obj || is_error(keywords_obj)) {
        status = SEARCH_ENGINE_ERR;
        goto err;
    }

    for (int i = 0; i < ::json_object_array_length(keywords_obj); i++) {
        json_object* obj = ::json_object_array_get_idx(keywords_obj, i);
        keyword_str_vec.push_back(std::string(::json_object_get_string(obj)));
    }

    int rt;
    rt = m_key_proc_.engine_get_recommend_keys(keyword_str_vec,
                                               recommends_str_vec,
                                               m_char_to_py_inst_);
    if (rt < 0) {
        ERROR_LOG("engine get recommend keys failed");
        status = SEARCH_ENGINE_ERR;
        goto err;
    }  else {
        ///
    }

    if (recommends_str_vec.size() == 0) {
        DEBUG_LOG("engine nofind recommend keys");
        status = SEARCH_ENGINE_NOFIND_RECOMMEND;
        goto err;
    }

    recommends_array = ::json_object_new_array();
    for (size_t i = 0; i < recommends_str_vec.size(); i++) {
        ::json_object_array_add(recommends_array,
                ::json_object_new_string(recommends_str_vec[i].c_str()));
    }
    ::json_object_object_add(resp_object, "recommend_keywords", recommends_array);

err:
   ::json_object_object_add(resp_object,
            "cmd_id", ::json_object_new_int(SEARCH_ENGINE_RECOMMEND_CMD));
   ::json_object_object_add(resp_object, "status", ::json_object_new_int(status));

    resp_json_str = ::json_object_to_json_string(resp_object);

    ::json_object_put(resp_object);

    return 0;
}

/*********************************** | **************************************/ 
/*********************************** | **************************************/ 
/*********************************** | **************************************/ 

int request_handler::__redis_get_search_result(const std::vector<std::string>& search_words,
                                               std::vector<did_tr_node_t*>& node_vec,
                                               int page_num, int result_count,
                                               int& total_count)
{
    std::map<std::string, int> search_map;
    redisReply* reply = NULL;
    ::redisAppendCommand(m_redis_conn_, "PING");
    ::redisAppendCommand(m_redis_conn_, "SELECT %d", m_redis_cache_table_);
    char keywords_buf[4096] = {0};
    char* p = keywords_buf;
    int len = 4096;

    int next_page_num = page_num + 1;

    for (size_t i = 0; i < search_words.size(); ++i) {
        int n = ::snprintf(p, len, "%s:", search_words[i].c_str());
        len -= n;
        p += n;

        search_map[search_words[i]] = (1 << i);
    }
    *(--p) = '\0';

    ::redisAppendCommand(m_redis_conn_, "LLEN %s", keywords_buf);
    ::redisAppendCommand(m_redis_conn_,
                         "LRANGE %s %d %d",
                         keywords_buf,
                         page_num * result_count,
                         next_page_num * result_count - 1);

    ///收到ping的结果
    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis ping failed, %s", m_redis_conn_->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    ///收到select的结果
    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis select failed, %s", m_redis_conn_->errstr);
        return -1;
    }
    ::freeReplyObject(reply);

    ///收到LLEN的结果
    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        return -1;
    }
    if (reply->type != REDIS_REPLY_INTEGER) {
        ::freeReplyObject(reply);
        return -1;
    }

    total_count = reply->integer;
    ::freeReplyObject(reply);

    ///收到LRANGE的结果
    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        return -1;
    }
    if (reply->type != REDIS_REPLY_ARRAY) {
        ::freeReplyObject(reply);
        return -1;
    }
    if (!reply->elements) {
        ::freeReplyObject(reply);
        return 1;
    }

    std::vector<std::string> result_json_str_vec;
    for (size_t i = 0; i < reply->elements; i++) {
        result_json_str_vec.push_back(std::string(reply->element[i]->str));
    }
    ::freeReplyObject(reply);

    ///解析所有的json数据，并存入结构体
    for (size_t i = 0; i < result_json_str_vec.size(); i++) {
        json_object* object = ::json_tokener_parse(result_json_str_vec[i].c_str());
        did_tr_node_t* node = (did_tr_node_t*)::calloc(1, sizeof(did_tr_node_t));
        if (!node) {
            ::json_object_put(object);
            continue;
        }

        index_data_t* index = NULL;
        json_object* title_object = NULL;
        json_object* content_object = NULL;
        std::vector<offset_data_t> offset_vec;

        ///获取did
        json_object* did_object = ::json_object_object_get(object, "did");
        if (!did_object || is_error(did_object)) {
            goto err;
        }
        node->did = ::json_object_get_int(did_object);

        ///获取title offset
        title_object = ::json_object_object_get(object, "title_offset");
        if (!title_object || is_error(title_object)) {
            goto next;
        }

        index = (index_data_t*)::calloc(1, sizeof(index_data_t));
        if (!index) {
            ::free(node);
            goto err;
        }

        offset_vec.clear();
        json_object_object_foreach(title_object, key, val) {
            offset_data_t tmp;
            tmp.keyword_id = search_map[key];
            for (int i = 0; i < ::json_object_array_length(val); i++) {
                json_object* obj = ::json_object_array_get_idx(val, i);
                tmp.offset = ::json_object_get_int(obj);
                offset_vec.push_back(tmp);
            }
        }
        std::sort(offset_vec.begin(), offset_vec.end(), offset_cmp());

        index->data = (offset_data_t*)::calloc(offset_vec.size(), sizeof(offset_data_t));
        if (!index->data) {
            ::free(index);
            ::free(node);
            goto err;
        }

        index->len = offset_vec.size();
        index->flag = FLAG_TITLE;
        std::copy(offset_vec.begin(), offset_vec.end(), index->data);

        index->next = node->index_list_node;
        node->index_list_node = index;

next:
        ///获取content offset
        content_object = ::json_object_object_get(object, "content_offset");
        if (!content_object || is_error(content_object)) {
            goto err;
        }

        index = (index_data_t*)::calloc(1, sizeof(index_data_t));
        if (!index) {
            goto err;
        }

        offset_vec.clear();
        json_object_object_foreach(content_object, key1, val1) {
            offset_data_t tmp;
            tmp.keyword_id = search_map[key1];
            for (int i = 0; i < ::json_object_array_length(val1); i++) {
                json_object* obj = ::json_object_array_get_idx(val1, i);
                tmp.offset = ::json_object_get_int(obj);
                offset_vec.push_back(tmp);
            }
        }
        std::sort(offset_vec.begin(), offset_vec.end(), offset_cmp());

        index->data = (offset_data_t*)::calloc(offset_vec.size(), sizeof(offset_data_t));
        if (!index->data) {
            ::free(index);
            goto err;
        }

        index->len = offset_vec.size();
        index->flag = FLAG_CONTENT;
        std::copy(offset_vec.begin(), offset_vec.end(), index->data);

        index->next = node->index_list_node;
        node->index_list_node = index;

err:
        ::json_object_put(object);
        node_vec.push_back(node);
    }

    return 0;
}

int request_handler::__redis_put_search_result(const std::vector<std::string>& search_words,
                                               set_data_map_t& set_data_map)
{
    std::map<int, std::string> search_words_map;
    for (size_t i = 0; i < search_words.size(); i++) {
        search_words_map[1 << i] = search_words[i];
    }

    std::vector<std::string> snippet_json_str_vec;
    ///convert each node struct to json string
    set_data_map_it_t it = set_data_map.begin();
    for ( ; it != set_data_map.end(); ++it) {
        for (struct rb_node* p = rb_first(&it->second.rank_tr_root); p; p = rb_next(p)) {
            did_tr_node_t* node = rb_entry(p, did_tr_node_t, tr_node);

            json_object* title_object = NULL;
            json_object* content_object = NULL;

            index_data_t* index = node->index_list_node;
            while (index) {
                std::map<int, json_object*> key_array_obj_map;
                for (int i = 0; i < index->len; i++) {
                    int keyword_id = index->data[i].keyword_id;
                    if (key_array_obj_map[keyword_id] == NULL) {
                        key_array_obj_map[keyword_id] = ::json_object_new_array();
                    }
                    ::json_object_array_add(key_array_obj_map[keyword_id],
                            ::json_object_new_int(index->data[i].offset));
                }

                if (index->flag == FLAG_TITLE) {
                    title_object = ::json_object_new_object();
                    std::map<int, json_object*>::iterator it;
                    for (it = key_array_obj_map.begin();
                         it != key_array_obj_map.end();
                         ++it) {
                        ::json_object_object_add(title_object,
                                                 search_words_map[it->first].c_str(),
                                                 it->second);
                    }
                } else if (index->flag == FLAG_CONTENT) {
                    content_object = ::json_object_new_object();
                    std::map<int, json_object*>::iterator it;
                    for (it = key_array_obj_map.begin();
                         it != key_array_obj_map.end();
                         ++it) {
                        ::json_object_object_add(content_object,
                                                 search_words_map[it->first].c_str(),
                                                 it->second);
                    }
                }

                index = index->next;
            }

            json_object* snippet_object = ::json_object_new_object();
            ::json_object_object_add(snippet_object, "did", ::json_object_new_int(node->did));
            if (title_object) {
                ::json_object_object_add(snippet_object, "title_offset", title_object);
            }
            if (content_object) {
                ::json_object_object_add(snippet_object, "content_offset", content_object);
            }

            snippet_json_str_vec.push_back(::json_object_to_json_string(snippet_object));
            ::json_object_put(snippet_object);
        }
    }

    ///save all the json string into redis, put all the exection in a
    ///transaction, if a key already exists, first delete it.

    int cmd_num = snippet_json_str_vec.size() + 4;
    redisReply* reply = NULL;
    ::redisAppendCommand(m_redis_conn_, "PING");
    ::redisAppendCommand(m_redis_conn_, "SELECT %d", m_redis_cache_table_);
    ::redisAppendCommand(m_redis_conn_, "MULTI");

    char keywords_buf[4096] = {0};
    char* p = keywords_buf;
    int len = 4096;

    for (size_t i = 0; i < search_words.size(); ++i) {
        int n = ::snprintf(p, len, "%s:", search_words[i].c_str());
        len -= n;
        p += n;
    }
    *(--p) = '\0';

    ::redisAppendCommand(m_redis_conn_, "DEL %s", keywords_buf);
    for (size_t i = 0; i < snippet_json_str_vec.size(); i++) {
        ::redisAppendCommand(m_redis_conn_,
                "RPUSH %s %s", keywords_buf, snippet_json_str_vec[i].c_str());
    }
    ::redisAppendCommand(m_redis_conn_, "EXPIRE %s 86400", keywords_buf);
    ::redisAppendCommand(m_redis_conn_, "EXEC");

    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis ping failed, %s", m_redis_conn_->errstr);
        return -1;
    }

    if (::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_ERR) {
        ERROR_LOG("redis select failed, %s", m_redis_conn_->errstr);
        return -1;
    }

    while (cmd_num-- > 0
            && ::redisGetReply(m_redis_conn_, (void**)&reply) == REDIS_OK) {
        ///consume all the replies
        ::freeReplyObject(reply);
    }

    return 0;
}
