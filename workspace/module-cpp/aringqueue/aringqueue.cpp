/**
 * =====================================================================================
 *       @file  aringqueue.cpp
 *      @brief  implementation of automatic increase ringqueue
 *
 *   @internal
 *     Created  09/15/2011 03:15:28 PM 
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "aringqueue.h"

aringqueue::aringqueue()
{
    aringqueue::aringqueue(sysconf(_SC_PAGESIZE));
}

aringqueue::aringqueue(int buf_sz): m_capcity(0), m_buf(0), m_read(0), m_write(0)
{
    if (buf_sz < 0) {
        stat_ok = false;
        return;
    }
    resize(buf_sz);
    stat_ok = true;
    is_full = false;
}

aringqueue::~aringqueue()
{
    stat_ok = false;
    free(m_buf);
    m_capcity = 0;
}


int aringqueue::push(void *buf, int size)
{
    if (!stat_ok || buf == 0 || size <= 0) {
        return -1;
    }

    int empty_size = m_capcity - get_length();
    if (size >= empty_size) {
        resize(2 * m_capcity);
        memcpy(m_write, buf, size);
        m_write += size;
        return 0;
    }

    //empty buffer is enough
    if (m_write < m_read) {
        memcpy(m_write, buf, size);
        m_write += size;
    } 
    else if (m_read < m_write) {
       int sec1_len = m_buf + m_capcity - m_write;
       if (sec1_len >= size) {
           memcpy(m_write, buf, size);
           m_write += size;
           is_full = sec1_len == size ? true : false;
       } else {
           memcpy(m_write, buf, sec1_len);
           int beg_off = (m_write - m_buf + size) % m_capcity;
           memcpy(m_buf, (char*)buf + sec1_len, size - sec1_len);
           m_write = beg_off + m_buf;
       }
    } else {
        m_read = m_buf;
        memcpy(m_buf, buf, size);
        m_write = m_buf + size;
    }
    return size;
}

int aringqueue::pop(void *buf, int size)
{
    if (!stat_ok || buf == 0 || size <= 0) {
        return -1;
    }

    int pop_len = size;
    if (m_read == m_write && !is_full) {
        pop_len = 0;
    }
    else if (m_read == m_write && is_full) {
        int sec1_len = m_buf + m_capcity - m_read;
        if (size >= m_capcity) { //pop all
            memcpy(buf, m_read, sec1_len);
            memcpy((char*)buf + sec1_len, m_buf, m_write - m_buf);

            pop_len = m_capcity;
            m_read = m_write = m_buf;
        } else {
            if (sec1_len >= size) {
                memcpy(buf, m_read, size);
                m_read = m_buf + ((m_read - m_buf) + size) % m_capcity;
            } else {
                memcpy(buf, m_read, sec1_len);
                memcpy((char*)buf + sec1_len, m_buf, size - sec1_len);
                m_read = m_buf + size - sec1_len;
            }
            pop_len = size;
        }
        is_full = size > 0 ? false : true;
    } 
    else if (m_read > m_write) {
        int sec1_len = m_buf + m_capcity - m_read;
        if (sec1_len >= size) {
           memcpy(buf, m_read, size);
           m_read = (m_read - m_buf + size) % m_capcity + m_buf;
           pop_len = size;
        } else {
            memcpy(buf, m_read, sec1_len);
            int more_len = size - sec1_len;
            int sec2_len = m_write - m_buf;
            if (more_len < sec2_len) {
                memcpy((char*)buf + sec1_len, m_buf, sec2_len); 
                pop_len = sec1_len + more_len;
                m_read = m_buf + more_len;
            } else {
                pop_len = sec1_len + sec2_len;
                m_read = m_buf + sec2_len;
            }
        }
        is_full = size > 0 ? false : true;
    } 
    else {
        int avil_len = m_write - m_read;
        pop_len = size >= avil_len ? avil_len : size;
        memcpy(buf, m_read, pop_len);
        m_read += pop_len;
        is_full = false;
    }

    return pop_len;
}

int aringqueue::peer(void *buf, int size)
{
    if (!stat_ok) {
        return -1;
    }

    int peer_len = size;
    if (m_read == m_write && !is_full) {
        peer_len = 0;
    }
    else if (m_read == m_write && is_full) {
        int sec1_len = m_buf + m_capcity - m_read;
        if (size >= m_capcity) { //pop all
            memcpy(buf, m_read, sec1_len);
            memcpy((char*)buf + sec1_len, m_buf, m_write - m_buf);

            peer_len = m_capcity;
        } else {
            if (sec1_len >= size) {
                memcpy(buf, m_read, size);
            } else {
                memcpy(buf, m_read, sec1_len);
                memcpy((char*)buf + sec1_len, m_buf, size - sec1_len);
            }
            peer_len = size;
        }
    } 
    else if (m_read > m_write) {
        int sec1_len = m_buf + m_capcity - m_read;
        if (sec1_len >= size) {
           memcpy(buf, m_read, size);
           peer_len = size;
        } else {
            memcpy(buf, m_read, sec1_len);
            int more_len = size - sec1_len;
            int sec2_len = m_write - m_buf;
            if (more_len < sec2_len) {
                memcpy((char*)buf + sec1_len, m_buf, sec2_len); 
                peer_len = sec1_len + more_len;
            } else {
                peer_len = sec1_len + sec2_len;
            }
        }
    } 
    else {
        int avil_len = m_write - m_read;
        peer_len = size >= avil_len ? avil_len : size;
        memcpy(buf, m_read, peer_len);
    }
    return peer_len;
}

int aringqueue::get_capcity()
{
    if (!stat_ok) {
        return -1;
    }

    return m_capcity;
}

int aringqueue::get_length()
{
    if (!stat_ok) {
        return -1;
    }

    int len = -1;
    if (m_read == m_write && !is_full) {
        len = 0;
    }
    else if (m_read == m_write && is_full) {
        len = m_capcity;
    }
    else if (m_read < m_write) {
        len = m_write - m_read;
    } 
    else {
        int sec1_len = m_buf + m_capcity - m_read;
        int sec2_len = m_write - m_buf;
        len = sec1_len + sec2_len;
    }
    return len;
}


void aringqueue::resize(int buf_sz)
{
    if (buf_sz <= m_capcity || buf_sz <= 0) {
        return;
    }
    char *tmp = (char*)malloc(buf_sz);
    if (m_read == m_write && !is_full) {
        //ringqueue is empty
        m_read = tmp;
        m_write = tmp;
    }
    else if (m_read < m_write) {
        int len = m_write - m_read;
        memcpy(tmp, m_read, len);
        m_read = tmp;
        m_write = tmp + len;
    } 
    else {
        int sec1_len = m_buf + m_capcity - m_read;
        int sec2_len = m_write - m_buf;
        memcpy(tmp, m_read, sec1_len);
        memcpy(tmp + sec1_len, m_buf, sec2_len);
        m_read = tmp;
        m_write = tmp + sec1_len + sec2_len;
    }
    free(m_buf);
    m_buf = tmp;
    m_capcity = buf_sz;
}

#ifdef DEBUG 
#include <stdio.h>
void aringqueue::dump()
{
#define     PRINT(fmt, args...) printf(fmt, ##args)
    PRINT("\n\tSTAT OK: %d, IS FULL: %d\n\tbuf addr: %p, capcity: %d\n\tm_read: %p, m_write: %p\n\n", 
            stat_ok, is_full, m_buf, m_capcity, m_read, m_write);
}
#endif
