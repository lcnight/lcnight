/**
 * @file alloc_handler.hpp
 * @brief 分配回调句柄函数
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-15
 */
#ifndef _H_ALLOC_HANDLER_HPP_
#define _H_ALLOC_HANDLER_HPP_

#include <boost/aligned_storage.hpp>
#include <boost/noncopyable.hpp>

class handler_allocator : boost::noncopyable {
public:
    handler_allocator()
        : m_in_use_(false) { }

    void* allocate(size_t size) {
        if (!m_in_use_ && size < m_storage_.size) {
            m_in_use_ = true;
            return m_storage_.address();
        } else {
            return ::operator new(size);
        }
    }

    void deallocate(void* ptr) {
        if (ptr == m_storage_.address()) {
            m_in_use_ = false;
        } else {
            ::operator delete(ptr);
        }
    }

private:
    boost::aligned_storage<1024> m_storage_;
    bool m_in_use_;
};

template <typename handler_t>
class custom_alloc_handler {
public:
    custom_alloc_handler(handler_allocator& a, handler_t h)
        : m_allocator_(a),
          m_handler_(h) { }

    template <typename arg1_t>
    void operator() (arg1_t arg1) {
        m_handler_(arg1);
    }

    template <typename arg1_t, typename arg2_t>
    void operator() (arg1_t arg1, arg2_t arg2) {
        m_handler_(arg1, arg2);
    }

    friend void* asio_handler_allocate(size_t size,
            custom_alloc_handler<handler_t>* this_handler) {
        return this_handler->m_allocator_.allocate(size);
    }

    friend void asio_handler_deallocate(void* ptr, size_t /*size*/,
            custom_alloc_handler<handler_t>* this_handler) {
        this_handler->m_allocator_.deallocate(ptr);
    }

private:
    handler_allocator& m_allocator_;
    handler_t m_handler_;
};

template <typename handler_t>
inline custom_alloc_handler<handler_t>
make_custom_alloc_handler(handler_allocator& a, handler_t t) {
    return custom_alloc_handler<handler_t>(a, t);
}

#endif
