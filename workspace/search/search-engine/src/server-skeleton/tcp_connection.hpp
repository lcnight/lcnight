/**
 * @file tcp_connection.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-16
 */

#ifndef _H_TCP_CONNECTION_HPP_
#define _H_TCP_CONNECTION_HPP_

#include <iostream>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

#include "alloc_handler.hpp"
#include "continuous_buffer.h"
#include "callback.h"

using boost::asio::ip::tcp;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>,
                       boost::noncopyable {
public:
    tcp_connection(boost::asio::io_service& io_service)
        : m_tcp_sock_(io_service),
          m_io_service_(io_service) { }

    tcp::socket& get_socket() {
        return m_tcp_sock_;
    }

    void set_new_conn_callback(const on_new_conn_callback_t& cb) {
        m_new_conn_cb_ = cb;
    }

    void set_message_callback(const on_message_callback_t& cb) {
        m_message_cb_ = cb;
    }

    void start() {
        if (m_snd_buffer_.continuous_buffer_init() < 0 ||
            m_rcv_buffer_.continuous_buffer_init() < 0) {
            throw std::runtime_error("continuous_buffer init failed");
        }

        if (m_new_conn_cb_) {
            if (m_new_conn_cb_(shared_from_this()) < 0) {
                m_tcp_sock_.shutdown(tcp::socket::shutdown_send);
                return ;
            }
        }

        m_tcp_sock_.async_read_some(
                boost::asio::buffer(m_rcv_buffer_.writeable_addr(), m_rcv_buffer_.writeable_bytes()),
                make_custom_alloc_handler(m_allocator_,
                        boost::bind(&tcp_connection::handle_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred)));
    }



    int send_data(void* buf, size_t buf_len) {
        if (m_snd_buffer_.writeable_bytes() < buf_len) {
            if (m_snd_buffer_.make_writeable_bytes(buf_len) < 0) {
                return -1;
            }
        }

        ::memcpy(m_snd_buffer_.writeable_addr(), (char*)buf, buf_len);
        m_snd_buffer_.move_writeable(buf_len);

        return 0;
    }


private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
        if (!error && m_message_cb_ ) {
            ///接收到数据，调用回调接口
            m_rcv_buffer_.move_writeable(bytes_transferred);

            int rt = m_message_cb_(shared_from_this(),
                                   m_rcv_buffer_.readable_addr(),
                                   m_rcv_buffer_.readable_bytes());
            if (rt < 0) {
                m_tcp_sock_.shutdown(tcp::socket::shutdown_send);
            } else {
                m_rcv_buffer_.move_readable(m_rcv_buffer_.readable_bytes() - rt);

                m_tcp_sock_.async_write_some(
                    boost::asio::buffer(m_snd_buffer_.readable_addr(), m_snd_buffer_.readable_bytes()),
                    make_custom_alloc_handler(m_allocator_,
                        boost::bind(&tcp_connection::handle_write,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred)));
            }

        }
    }

    void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
        if (!error) {
            m_snd_buffer_.move_readable(bytes_transferred);

            m_tcp_sock_.async_read_some(
                    boost::asio::buffer(m_rcv_buffer_.writeable_addr(), m_rcv_buffer_.writeable_bytes()),
                    make_custom_alloc_handler(m_allocator_,
                            boost::bind(&tcp_connection::handle_read,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)));
        }
    }


private:
    tcp::socket m_tcp_sock_;

    boost::asio::io_service& m_io_service_;

    continuous_buffer m_snd_buffer_;
    continuous_buffer m_rcv_buffer_;

    handler_allocator m_allocator_;

    on_new_conn_callback_t m_new_conn_cb_;
    on_message_callback_t m_message_cb_;
};

#endif
