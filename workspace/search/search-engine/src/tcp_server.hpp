/**
 * @file tcp_server.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-16
 */

#ifndef _H_TCP_SERVER_HPP_
#define _H_TCP_SERVER_HPP_

#include <pthread.h>

#include <iostream>
#include <string>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "server-skeleton/tcp_connection.hpp"
#include "server-skeleton/callback.h"
#include "server-skeleton/io_service_pool.hpp"

#include "utility/redis_connect_pool.hpp"
#include "utility/mysql_connect_pool.hpp"
#include "utility/char_to_py.hpp"
#include "utility/utility.hpp"

#include "key-process-engine/key_process_engine.h"
#include "search-kernel/search_kernel.h"
#include "word-segment/word_segment_pool.hpp"

#include "request_handler.h"

using boost::asio::ip::tcp;

class tcp_server : boost::noncopyable {
public:
    tcp_server(std::string serv_ip,
               u_short serv_port,
               size_t io_pool_size,
               key_process_engine& key_proc,
               search_kernel& search_proc,
               char_to_py& char_to_py_inst,
               redis_connect_pool& redis_conn_pool,
               mysql_connect_pool& mysql_conn_pool,
               word_segment_pool& word_segment_pool,
               pthread_rwlock_t* rw_lock,
               int* redis_table_id,
               int redis_cache_table,
               const std::string& url_str)
        : m_io_service_pool_(io_pool_size),
          m_endpoint_(boost::asio::ip::address_v4::from_string(serv_ip), serv_port),
          m_acceptor_(m_io_service_pool_.get_io_service(), m_endpoint_),
          m_key_proc_(key_proc),
          m_search_proc_(search_proc),
          m_char_to_py_inst_(char_to_py_inst),
          m_redis_conn_pool_(redis_conn_pool),
          m_mysql_conn_pool_(mysql_conn_pool),
          m_word_segment_pool_(word_segment_pool),
          m_rwlock_(rw_lock),
          m_redis_table_id_(redis_table_id),
          m_redis_cache_table_(redis_cache_table),
          m_snippet_url_(url_str),
          m_index_check_timer_(m_io_service_pool_.get_io_service(), boost::posix_time::seconds(3600)),
          m_redis_check_timer_(m_io_service_pool_.get_io_service(), boost::posix_time::seconds(60))
    {
        m_index_check_timer_.async_wait(
                boost::bind(&tcp_server::index_check_timeout_callback, this));

        m_redis_check_timer_.async_wait(
                boost::bind(&tcp_server::redis_check_timeout_callback, this));

        tcp_connection_ptr_t new_conn(
                new tcp_connection(m_io_service_pool_.get_io_service()));

        m_acceptor_.async_accept(new_conn->get_socket(),
                                 boost::bind(&tcp_server::handle_accept,
                                             this,
                                             new_conn,
                                             boost::asio::placeholders::error));
    }

    void run() {
        m_io_service_pool_.run();
    }

    void stop() {
        m_io_service_pool_.stop();
        m_io_service_pool_.join();
    }

    void index_check_timeout_callback() {
        m_key_proc_.engine_build(m_rwlock_,
                                 m_redis_conn_pool_.get_next_redis_conn(),
                                 m_mysql_conn_pool_.get_next_mysql_conn(),
                                 m_char_to_py_inst_,
                                 m_redis_table_id_);

        m_index_check_timer_.expires_at(
                m_index_check_timer_.expires_at() + boost::posix_time::seconds(3600));

        m_index_check_timer_.async_wait(
                boost::bind(&tcp_server::index_check_timeout_callback, this));
    }

    ///这种重连机制只针对短连接有效
    void redis_check_timeout_callback() {
        boost::shared_ptr<pthread_rwlock_t> rw_lock_ptr(
                m_rwlock_, pthread_rwlock_wrap_unlock());

        ::pthread_rwlock_wrlock(m_rwlock_);

        m_redis_conn_pool_.check();

        m_redis_check_timer_.expires_at(
                m_redis_check_timer_.expires_at() + boost::posix_time::seconds(60));

        m_redis_check_timer_.async_wait(
                boost::bind(&tcp_server::redis_check_timeout_callback, this));

    }

private:
    void handle_accept(tcp_connection_ptr_t new_conn, const boost::system::error_code& error) {
        if (!error) {
            boost::shared_ptr<request_handler> new_request_handler(
                    new request_handler(m_key_proc_,
                                        m_search_proc_,
                                        m_char_to_py_inst_,
                                        m_redis_conn_pool_.get_next_redis_conn(),
                                        m_mysql_conn_pool_.get_next_mysql_conn(),
                                        m_word_segment_pool_.get_next_scws(),
                                        m_rwlock_,
                                        m_redis_table_id_,
                                        m_redis_cache_table_,
                                        m_snippet_url_));

            new_conn->set_message_callback(
                boost::bind(&request_handler::handle_request, new_request_handler, _1, _2, _3));

            new_conn->start();

            new_conn.reset(
                    new tcp_connection(m_io_service_pool_.get_io_service()));

            m_acceptor_.async_accept(new_conn->get_socket(),
                                     boost::bind(&tcp_server::handle_accept,
                                                 this,
                                                 new_conn,
                                                 boost::asio::placeholders::error));
        }
    }

private:
    io_service_pool m_io_service_pool_;

    tcp::endpoint m_endpoint_;
    tcp::acceptor m_acceptor_;

    key_process_engine& m_key_proc_;
    search_kernel& m_search_proc_;
    char_to_py& m_char_to_py_inst_;
    redis_connect_pool& m_redis_conn_pool_;
    mysql_connect_pool& m_mysql_conn_pool_;
    word_segment_pool& m_word_segment_pool_;

    pthread_rwlock_t* m_rwlock_;

    int* m_redis_table_id_;
    int m_redis_cache_table_;

    const std::string& m_snippet_url_;

    boost::asio::deadline_timer m_index_check_timer_;
    boost::asio::deadline_timer m_redis_check_timer_;
};

#endif
