/**
 * @file io_service_pool.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-17
 */
#ifndef _H_IO_SERVICE_POOL_HPP_
#define _H_IO_SERVICE_POOL_HPP_

#include <boost/asio.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>


/// A pool of io_service objects.
class io_service_pool : boost::noncopyable {
public:
    explicit io_service_pool(size_t pool_size)
        : m_next_io_service_(0) {
        if (pool_size == 0)
            throw std::runtime_error("io_service_pool size is 0");

        for (size_t i = 0; i < pool_size; ++i) {
            io_service_ptr io_service(new boost::asio::io_service);
            work_ptr work(new boost::asio::io_service::work(*io_service));
            m_io_service_vec_.push_back(io_service);
            m_work_vec_.push_back(work);
        }
    }

    void run() {
        for (size_t i = 0; i < m_io_service_vec_.size(); i++) {
            boost::shared_ptr<boost::thread> thread(
                    new boost::thread(
                        boost::bind(&boost::asio::io_service::run, m_io_service_vec_[i])));
            m_thread_vec_.push_back(thread);
        }
    }

    void join() {
        for (size_t i = 0; i < m_thread_vec_.size(); ++i) {
            m_thread_vec_[i]->join();
        }
    }

    void stop() {
        for (size_t i = 0; i < m_io_service_vec_.size(); i++) {
            m_io_service_vec_[i]->stop();
        }
    }

    boost::asio::io_service& get_io_service() {
        boost::asio::io_service& io_service = *m_io_service_vec_[m_next_io_service_];
        ++m_next_io_service_;

        if (m_next_io_service_ == m_io_service_vec_.size())
            m_next_io_service_ = 0;

        return io_service;
    }

private:
    typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
    typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

    /// The pool of io_services.
    std::vector<io_service_ptr> m_io_service_vec_;

    /// The work that keeps the io_services running.
    std::vector<work_ptr> m_work_vec_;

    std::vector<boost::shared_ptr<boost::thread> > m_thread_vec_;

    /// The next io_service to use for a connection.
    size_t m_next_io_service_;
};
#endif
