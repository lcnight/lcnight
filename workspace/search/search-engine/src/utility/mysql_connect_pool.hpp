/**
 * @file mysql_connect_pool.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-21
 */

#ifndef _H_MYSQL_CONNECT_POOL_HPP_
#define _H_MYSQL_CONNECT_POOL_HPP_

#include <mysql/mysql.h>

#include <vector>
#include <string>

#include <boost/noncopyable.hpp>

class mysql_connect_pool : boost::noncopyable {
public:
    explicit mysql_connect_pool(size_t pool_size)
        : m_pool_size_(pool_size),
          m_next_mysql_conn_(0) { }

    ~mysql_connect_pool() {
        mysql_conn_pool_fini();
    }

    int mysql_conn_pool_init(const std::string& mysql_host,
                             const u_short mysql_port,
                             const std::string& db_name,
                             const std::string& user,
                             const std::string& passwd,
                             const std::string& charset) {
        for (size_t i = 0; i < m_pool_size_; i++) {
            MYSQL* p_mysql = ::mysql_init(NULL);
            if (!p_mysql) {
                return -1;
            }

            my_bool auto_reconn = 1;
            if (::mysql_options(p_mysql, MYSQL_OPT_RECONNECT, &auto_reconn)) {
                return -1;
            }

            if (!::mysql_real_connect(p_mysql,
                                      mysql_host.c_str(),
                                      user.c_str(),
                                      passwd.c_str(),
                                      db_name.c_str(),
                                      mysql_port,
                                      NULL,
                                      CLIENT_INTERACTIVE | CLIENT_MULTI_STATEMENTS)) {
                return -1;
            }

            if (::mysql_set_character_set(p_mysql, charset.c_str())) {
                return -1;
            }

            m_mysql_conn_vec_.push_back(p_mysql);
        }

        return 0;
    }

    void mysql_conn_pool_fini() {
        for (size_t i = 0; i < m_mysql_conn_vec_.size(); i++) {
            ::mysql_close(m_mysql_conn_vec_[i]);
            m_mysql_conn_vec_[i] = NULL;
        }
    }

    MYSQL* get_next_mysql_conn() {
        MYSQL* p = m_mysql_conn_vec_[m_next_mysql_conn_];

        ++m_next_mysql_conn_;
        if (m_next_mysql_conn_ == m_mysql_conn_vec_.size()) {
            m_next_mysql_conn_ = 0;
        }

        return p;
    }

private:
    size_t m_pool_size_;
    std::vector<MYSQL*> m_mysql_conn_vec_;

    size_t m_next_mysql_conn_;
};

#endif
