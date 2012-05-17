/**
 * @file key_process_engine.h
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-17
 */

#ifndef _H_KEY_PROCESS_ENGINE_H_
#define _H_KEY_PROCESS_ENGINE_H_

#include <pthread.h>

#include <hiredis/hiredis.h>
#include <mysql/mysql.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "atomic.h"
#include "../utility/doc_sort.hpp"

class char_to_py;
class key_process_engine {
public:
    key_process_engine()
        : m_p_root_(NULL),
          m_p_order_list_(NULL),
          m_word_total_num_(0)
    { }

    ~key_process_engine() {
        __engine_destroy();
    }

    /**
     * @brief 初始化关键词处理引擎，构造trie树
     * @param rw_lock     读写锁，在初始化引擎期间所有网络操作都被堵塞
     * @param redis_conn  redis连接指针
     * @param mysql_conn  mysql连接指针
     * @param char_to_py_inst  拼音转化模块
     * @param p_old_table_id   当前redis表的指针
     * @return -1failed, 0success
     */
    int engine_build(pthread_rwlock_t* rw_lock,
                     redisContext* redis_conn,
                     MYSQL* mysql_conn,
                     char_to_py& char_to_py_inst,
                     int* p_old_table_id);

    /**
     * @brief 搜索关键词处理函数
     * @param input_str   输入关键词
     * @param output_str  输出关键词
     * @param char_to_py_inst  拼音转换模块
     * @return -1failed, 0success, 1处理所得关键词为近似关键词
     */
    int engine_key_process(const std::string& input_str,
                           std::string& output_str,
                           char_to_py& char_to_py_inst);

    /**
     * @brief 获得关键词提示函数
     * @param input_str  输入关键词
     * @param output_str_vec  输出关键词数组
     * @param char_to_py_inst 拼音转换模块
     * @return -1failed, 0success, 1未获得任何提示
     */
    int engine_get_hinted_keys(const std::string& input_str,
                               std::vector<std::string>& output_str_vec,
                               char_to_py& char_to_py_inst);

    /**
     * @brief 获得关键词推荐函数
     * @param input_str_vec  输入关键词数组
     * @param recommend_str_vec  输出关键词数组
     * @param char_to_py_inst  拼音转换模块
     * @return -1failed, 0success
     */
    int engine_get_recommend_keys(const std::vector<std::string>& input_str_vec,
                                  std::vector<std::string>& recommend_str_vec,
                                  char_to_py& char_to_py_inst);

    /**
     * @brief 从引擎中获得该文章相应的view vote
     * @param 
     * @return 
     */
    int get_doc_vv(uint32_t did, view_vote& vv);

#if 0
    /**
     * @brief 从引擎中获得该文章相应的rank
     * @param did  文章id
     * @return 文章rank
     */
    int engine_get_rank(int did);
#endif

private:
    enum { num_chars = 26 };

    typedef struct word_node {
        char* word;
        atomic_t word_rate;

        struct word_node* next;
        struct word_node* order_list_node;
    } word_node_t;

    typedef struct trie_node {
        word_node_t* word_list;

        struct trie_node* branch[num_chars];
        trie_node* parent_node;
    } trie_node_t;

private:

    /**
     * @brief 销毁关键词处理引擎
     * @return NULL
     */
    void __engine_destroy();

    /**
     * @brief 判断该词是否有纯中文或是英文组成
     * @param input_str  输入单词
     * @return -1failed, 0无法判断，1纯英文, 2纯中文 3中英文混杂
     */
    int __key_ch_en_judege(const std::string& input_str);

    /**
     * @brief 根据编辑距离计算两个单词的相关度
     * @param target_str  目标单词
     * @param candidate_str  候选单词
     * @return -1failed, coherence返回相关度
     */
    int __key_coherence_calcu(const std::string& target_str,
                              const std::string& candidate_str);

    /**
     * @brief 对英文关键词进行模糊搜索，找出相似关键词中热门度最高的
     * @param p_root  trie树节点
     * @param input_str  输入关键词
     * @param output_str 输出关键词
     * @return -1failed, 0success
     */
    int __key_fuzz_process_en(trie_node_t* p_root,
                              const std::string& input_str,
                              std::string& output_str);

    /**
     * @brief 对中文关键词进行模糊搜索，找出相似关键词中热门度最高的
     * @param p_root  trie树节点
     * @param input_str  输入关键词
     * @param output_str 输出关键词
     * @return -1failed, 0success
     */
    int __key_fuzz_process_cn(trie_node_t* p_root,
                              const std::string& input_str,
                              std::string& output_str);

    /**
     * @brief 在trie树中进行模糊搜索，对于不存在的节点则进行回溯
     * @param p_root  trie树根节点
     * @param key_str 搜索key值
     * @param p_trie_node  所找到的节点
     * @param is_backtrace  在未找到节点的情况下是否进行回溯
     * @return NULL
     */
    void __trie_fuzz_search(trie_node_t* p_root,
                            const std::string& key_str,
                            trie_node_t** p_trie_node,
                            bool is_backtrace);

    /**
     * @brief 在trie树中进行精确查找
     * @param p_root  trie树根结点
     * @param key_str  搜索key值
     * @param value_str  比较value值
     * @param p_word_node  返回找到关键词节点
     * @return NULL
     */
    void __trie_exact_search(trie_node_t* p_root,
                             const std::string& key_str,
                             const std::string& value_str,
                             word_node_t** p_word_node);

    /**
     * @brief 在trie树中进行深度递归遍历
     * @param p_root  trie树根结点
     * @param depth   深度
     * @param str_map 搜索得到的所有关键词
     * @return NULL
     */
    void __trie_depth_search(trie_node_t* p_root,
                             int depth,
                             std::multimap<int, char*, std::greater<int> >& str_map);

    /**
     * @brief 在trie树节点的链表中进行广度遍历
     * @param p_order_list  trie树链表
     * @param cmp_str       比较key值
     * @param depth         广度层数
     * @param str_map       遍历得到的所有关键词
     * @return NULL
     */
    void __order_list_depth_search(word_node_t* p_order_list,
                                   const std::string& cmp_str,
                                   int depth,
                                   std::multimap<int, char*, std::greater<int> >& str_map);

    /**
     * @brief 释放trie树
     * @return NULL
     */
    void __trie_release(trie_node_t* p_root);

private:
    trie_node_t* m_p_root_;

    word_node_t* m_p_order_list_;

    size_t m_word_total_num_;

    /*std::map<int, int> m_id_rank_map_;*/

    doc_vv did_vv;
};

#endif
