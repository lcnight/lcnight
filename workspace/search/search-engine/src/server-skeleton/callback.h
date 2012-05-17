/**
 * @file callback.h
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-16
 */

#ifndef _H_CALLBACK_H_
#define _H_CALLBACK_H_

#include <boost/bind.hpp>
#include <boost/function.hpp>

class tcp_connection;
typedef boost::shared_ptr<tcp_connection> tcp_connection_ptr_t;

typedef boost::function<void()> on_timer_callback_t;
typedef boost::function<int (const tcp_connection_ptr_t&)> on_new_conn_callback_t;
typedef boost::function<int (const tcp_connection_ptr_t&,
                             void*,
                             size_t)> on_message_callback_t;
#endif
