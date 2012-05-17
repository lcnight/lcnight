/**
 * @file word_segment_pool.hpp
 * @brief 中文分词模块
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-12-19
 */

#ifndef _H_WORD_SEGMENT_POOL_HPP
#define _H_WORD_SEGMENT_POOL_HPP
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

#include <scws/scws.h>
#include "../utility/log.h"

class word_segment_pool : boost::noncopyable {
public:
    word_segment_pool(const std::string& charset,
                      const std::string& dict_path,
                      const std::string& rule_path,
                      size_t pool_size)
        : m_charset_(charset),
          m_dict_path_(dict_path),
          m_rule_path_(rule_path),
          m_pool_size_(pool_size),
          m_next_scws_(0),
          m_scws_(NULL) { }

    ~word_segment_pool() {
        ::scws_free(m_scws_);
        m_scws_ = NULL;

        for (size_t i = 0; i < m_pool_size_; i++) {
            m_scws_pool_[i]->d = NULL;
            m_scws_pool_[i]->r = NULL;
            ::scws_free(m_scws_pool_[i]);
            m_scws_pool_[i] = NULL;
        }
    }

    int word_segment_pool_init() {
        if (!(m_scws_ = ::scws_new())) {
            return -1;
        }

        ::scws_set_charset(m_scws_, m_charset_.c_str());
        if (::scws_set_dict(m_scws_, m_dict_path_.c_str(), SCWS_XDICT_MEM) < 0) {
            ERROR_LOG("set dict error: %s", m_dict_path_.c_str());
            return -1;
        }

        ::scws_set_rule(m_scws_, m_rule_path_.c_str());
        ::scws_set_ignore(m_scws_, 1);
        ::scws_set_duality(m_scws_, 1);

        for (size_t i = 0; i < m_pool_size_; i++) {
            scws_t tmp = ::scws_new();
            if (!tmp) {
                return -1;
            }

            ::scws_set_charset(tmp, m_charset_.c_str());
            tmp->d = m_scws_->d;
            tmp->r = m_scws_->r;

            ::scws_set_ignore(tmp, 1);
            ::scws_set_duality(tmp, 1);

            m_scws_pool_.push_back(tmp);
        }

        return 0;
    }

    static int segment_word(scws_t p_scws,
                            const std::string& input_str,
                            std::vector<std::string>& output_str_vec) {
        ::scws_send_text(p_scws, input_str.c_str(), input_str.length());

        scws_res_t res = NULL;
        scws_res_t cur = NULL;

        while ((res = cur = ::scws_get_result(p_scws))) {
            while (cur) {
                output_str_vec.push_back(
                        std::string(input_str.c_str() + cur->off, cur->len));
                cur = cur->next;
            }
            ::scws_free_result(res);
        }

        return 0;
    }

    scws_t get_next_scws() {
        scws_t tmp = m_scws_pool_[m_next_scws_];

        ++m_next_scws_;
        if (m_next_scws_ == m_scws_pool_.size()) {
            m_next_scws_ = 0;
        }

        return tmp;
    }

private:
    std::string m_charset_;
    std::string m_dict_path_;
    std::string m_rule_path_;

    size_t m_pool_size_;
    size_t m_next_scws_;

    scws_t m_scws_;
    std::vector<scws_t> m_scws_pool_;
};

#endif

