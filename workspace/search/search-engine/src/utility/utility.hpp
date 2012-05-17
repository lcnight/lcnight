/**
 * @file utility.hpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-21
 */
#ifndef _H_UTILITY_HPP_
#define _H_UTILITY_HPP_

#include <pthread.h>
#include <mysql/mysql.h>

class pthread_rwlock_wrap_unlock {
public:
    void operator() (pthread_rwlock_t* rw_lock) {
        if (rw_lock) {
            ::pthread_rwlock_unlock(rw_lock);
        }
    }
};

class mysql_res_wrap_free {
public:
    void operator() (MYSQL_RES* p_res) {
        if (p_res) {
            ::mysql_free_result(p_res);
            p_res = NULL;
        }
    }
};

#endif
