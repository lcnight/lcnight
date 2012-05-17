/**
 * @file search_kernel.h
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-28
 */

#ifndef _H_SEARCH_KERNEL_H_
#define _H_SEARCH_KERNEL_H_

#include <stdint.h>

#include <hiredis/hiredis.h>
#include <mysql/mysql.h>
#include <scws/scws.h>

#include <string>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "../struct_def.h"
#include "../utility/doc_sort.hpp"

typedef std::vector<std::string> keyword_vector;
typedef std::vector<std::string>::iterator keyword_vector_it;

class key_process_engine;
class search_kernel {
public:
    search_kernel() { }
    ~search_kernel() { }

    /**
     * @brief 关键词反向索引查找排序
     * @param keyword_vec  关键词数组
     * @param redis_conn   redis连接指针
     * @param mysql_conn   mysql连接指针
     * @param key_proc     关键词处理模块
     * @param table_id     redis反向索引数据表
     * @param set_data_map 排序后的所有搜索结果
     * @return -1failed, 0success
     */
    int kernel_search_process(const std::vector<std::string>& keyword_vec,
                              redisContext* redis_conn, MYSQL* mysql_conn,
                              key_process_engine& key_proc, int table_id,
                              sorted_docs& set_data_map, rank_docid& rk_docs);

    /**
     * @brief 从mysql中取得文章和标题片段
     * @param selected_docs     在本次搜索结果内的文章
     * @param mysql_conn        mysql连接指针
     * @return -1failed, 0success
     */
    int get_snippet_frm_mysql(rank_doc& selected_docs, MYSQL* mysql_conn);


    /**
     * @brief  合并redis中文章关键词索反向索引到文章排序列表中
     * @param  
     * @return -1failed, 0success
     */
    int merge_doc_keyword(sorted_docs& set_data_map, string& cur_str, 
            redis_index_t *doc_keys, int off_num);


    int regular_keywords(redisContext* redis_conn, int table_id, 
        scws_t m_p_scws_, char_to_py& py_inst, key_process_engine& m_key_proc_,
        const keyword_vector& raw_keywords, keyword_vector& real_keywords, sorted_docs& set_data_map);
    
private:
    /**
     * @brief 从redis中获取对应关键词的索引
     * @param keyword_vec  搜索关键词数组
     * @param set_data_map 搜索结果map
     * @param redis_conn   redis连接指针
     * @param table_id     反向索引表id
     * @return -1failed, 0success
     */
    int __redis_get_index(const std::vector<std::string>& keyword_vec,
                          sorted_docs& set_data_map,
                          redisContext* redis_conn,
                          int table_id);
    /**
     * @brief 对于搜索结果计算rank并,并且按照rank大小排序
     * @param doc_map   需要参与排序的文章标识 和 文章信息的因素map
     * @param rk_docs   文章rank值到文章标识的映射
     * @return NULL
     */
    void __index_doc_sort(key_process_engine& key_proc, sorted_docs& doc_map, rank_docid& rk_docs);

};
#endif
