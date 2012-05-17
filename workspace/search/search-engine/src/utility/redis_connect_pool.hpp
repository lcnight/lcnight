/**
 * @file redis_connect_pool.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-17
 */

#ifndef _H_REDIS_CONNECT_POOL_HPP_
#define _H_REDIS_CONNECT_POOL_HPP_
///usr/local/include
#include <stdlib.h>
#include <hiredis/hiredis.h>

#include <iostream>
#include <vector>
#include <string>

#include <boost/noncopyable.hpp>

#include "log.h"

class redis_connect_pool : boost::noncopyable {
public:
    explicit redis_connect_pool(size_t pool_size)
        : m_pool_size_(pool_size),
          m_next_redis_conn_(0) { }

    ~redis_connect_pool() {
        redis_conn_pool_fini();
    }

    int redis_conn_pool_init(const std::string& redis_serv_ip,
                             const u_short redis_conn_port) {
        m_redis_serv_ip_ = redis_serv_ip;
        m_redis_serv_port_ = redis_conn_port;

        struct timeval timeout = {3, 0};
        for (size_t i = 0; i < m_pool_size_; i++) {
            redisContext* c =
                ::redisConnectWithTimeout(redis_serv_ip.c_str(), redis_conn_port, timeout);
            if (c->err) {
                ::redisFree(c);
                return -1;
            }
            m_redis_con_vec_.push_back(c);
        }

        return 0;
    }

    void redis_conn_pool_fini() {
        for (size_t i = 0; i < m_pool_size_; i++) {
            ::redisFree(m_redis_con_vec_[i]);
            m_redis_con_vec_[i] = NULL;
        }
    }

    redisContext* get_next_redis_conn() {
        redisContext* p = m_redis_con_vec_[m_next_redis_conn_];

        ++m_next_redis_conn_;
        if (m_next_redis_conn_ == m_redis_con_vec_.size())
            m_next_redis_conn_ = 0;

        return p;
    }

    void check() {
        for (size_t i = 0; i < m_pool_size_; i++) {
            redisReply* reply = (redisReply*)::redisCommand(m_redis_con_vec_[i], "PING");
            if (m_redis_con_vec_[i]->err) {
                ERROR_LOG("redis ping failed, %s", m_redis_con_vec_[i]->errstr);

                ::redisFree(m_redis_con_vec_[i]);
                m_redis_con_vec_[i] = NULL;

                struct timeval timeout = {0, 500000};
                m_redis_con_vec_[i] = ::redisConnectWithTimeout(
                        m_redis_serv_ip_.c_str(), m_redis_serv_port_, timeout);
            } else {
                ::freeReplyObject(reply);
            }
        }
    }

private:
    size_t m_pool_size_;
    std::vector<redisContext*> m_redis_con_vec_;

    size_t m_next_redis_conn_;

    std::string m_redis_serv_ip_;
    u_short m_redis_serv_port_;
};

#endif



