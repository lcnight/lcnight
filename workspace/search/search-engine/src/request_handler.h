/**
 * @file request_handler.h
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-12-03
 */

#ifndef _H_REQUEST_HANDLER_H_
#define _H_REQUEST_HANDLER_H_

#include <iostream>
#include <pthread.h>
#include <mysql/mysql.h>
#include <hiredis/hiredis.h>
#include <json/json.h>
#include <scws/scws.h>

#include <vector>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "server-skeleton/callback.h"
#include "struct_def.h"
#include "utility/doc_sort.hpp"

class key_process_engine;
class search_kernel;
class char_to_py;
class word_segment;
class request_handler : public boost::enable_shared_from_this<request_handler>,
                        boost::noncopyable {
public:
    request_handler(key_process_engine& key_proc,
                    search_kernel& search_proc,
                    char_to_py& char_to_py_inst,
                    redisContext* redis_conn,
                    MYSQL* mysql_conn,
                    scws_t p_scws,
                    pthread_rwlock_t* rw_lock,
                    int* redis_table_id,
                    int redis_cache_table,
                    const std::string& url_str);

    ~request_handler() {
    }

    /**
     * @brief 处理请求函数
     * @param conn_ptr  该连接的tcp指针
     * @param buffer    该请求的buffer
     * @param len       buffer的长度
     * @return -1failed, 0success
     */
    int handle_request(const tcp_connection_ptr_t& conn_ptr, void* buffer, size_t len);

private:
    /**
     * @brief 请求错误处理函数
     * @param obj 请求包体json指针
     * @param resp_json_str 返回包体
     * @return -1failed, 0success
     */
    int __request_get_error(json_object* obj, std::string& resp_json_str);

    /**
     * @brief 请求获得关键字提示函数
     * @param obj  请求包体json格式
     * @param resp_json_str  返回包体
     * @return -1failed, 0success
     */
    int __request_get_hints(json_object* obj, std::string& resp_json_str);

    /**
     * @brief 将搜索关键字存入redis函数
     * @param search_words  请求关键字数组
     * @return -1failed, 0success
     */
    int __redis_put_search_words(const std::vector<std::string>& search_words);

    /**
     * @brief 请求搜索函数
     * @param obj 请求包体json指针
     * @param resp_json_str 返回包体
     * @return -1failed, 0success
     */
    int __request_get_search(json_object* obj, std::string& resp_json_str);

    /**
     * @brief 构造返回的json结果
     * @param 
     * @return -1failed, 0success
     */
    void build_search_json_result(int status, int total_cnt, 
            vector<string>& keywords, vector<string>& recomends, 
            rank_doc& selected_docs, string& resp_json);

    /**
     * @brief 搜索结果存入redis函数
     * @param search_words   搜索关键字
     * @param set_data_map   搜索结果map
     * @return -1failed, 0success
     */
    int __redis_put_search_result(const std::vector<std::string>& search_words,
                                  set_data_map_t& set_data_map);
    /**
     * @brief 请求获得推荐搜索关键字函数
     * @param obj  请求包体json格式
     * @param resp_json_str 返回包体
     * @return -1failed, 0success
     */
    int __request_get_recommend(json_object* obj, std::string& resp_json_str);

    /**
     * @brief 从redis中获得搜索结果函数
     * @param search_words  请求搜索关键字数组
     * @param node_vec      结果节点数组
     * @param page_num      请求搜索页面
     * @param result_count  请求搜索结果条数
     * @param total_count   请求返回搜索总结果
     * @return -1failed, 0success
     */
    int __redis_get_search_result(const std::vector<std::string>& search_words,
                                  std::vector<did_tr_node_t*>& node_vec,
                                  int page_num,
                                  int result_count,
                                  int& total_count);

private:
    key_process_engine& m_key_proc_;
    search_kernel& m_search_proc_;
    char_to_py& m_char_to_py_inst_;

    redisContext* m_redis_conn_;
    int* m_redis_table_id_;
    int m_redis_cache_table_;

    MYSQL* m_mysql_conn_;

    scws_t m_p_scws_;

    pthread_rwlock_t* m_rwlock_;

    const std::string& m_snippet_url_;
};
#endif
