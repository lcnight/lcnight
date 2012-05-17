/**
 * @file io_thread_pool.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-16
 */

#ifndef _H_IO_THREAD_POOL_HPP_
#define _H_IO_THREAD_POOL_HPP_

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

class io_thread_pool : boost::noncopyable {
public:
    io_thread_pool(size_t pool_size)
        : m_p_io_service_(new boost::asio::io_service),
          m_p_work_(new boost::asio::io_service::work(*m_p_io_service_)),
          m_pool_size_(pool_size) { }

    void run() {
        for (size_t i = 0; i < m_pool_size_; i++) {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                        boost::bind(&boost::asio::io_service::run, m_p_io_service_)));
            m_thread_vec_.push_back(thread);
        }
    }

    void join() {
        for (size_t i = 0; i < m_thread_vec_.size(); i++) {
            m_thread_vec_[i]->join();
        }
    }

    void stop() {
        m_p_io_service_->stop();
    }

    boost::asio::io_service& get_io_service() {
        return *m_p_io_service_;
    }

private:
    typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr_t;
    typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr_t;

    io_service_ptr_t m_p_io_service_;
    work_ptr_t m_p_work_;

    size_t m_pool_size_;
    std::vector<boost::shared_ptr<boost::thread> > m_thread_vec_;
};

#endif
