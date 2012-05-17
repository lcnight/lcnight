/**
 * @file char_to_py.hpp
 * @brief 将char编码汉字转化为拼音程序
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-11-10
 */

#ifndef _H_CHAR_TO_PY_HPP_
#define _H_CHAR_TO_PY_HPP_

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iconv.h>

#include <string>

#include "log.h"

class char_to_py {
public:
    char_to_py(const std::string& py_data_path)
        : m_py_data_path_(py_data_path),
          m_py_data_fd_(-1),
          m_py_data_(NULL),
          m_py_data_len_(0) { }

    char_to_py()
        : m_py_data_fd_(-1),
          m_py_data_(NULL),
          m_py_data_len_(0) { }

    ~char_to_py() {
        if (m_py_data_fd_ != -1) {
            ::munmap(m_py_data_, m_py_data_len_);
            ::close(m_py_data_fd_);
            m_py_data_fd_ = -1;
        }

        m_py_data_ = NULL;
        m_py_data_len_ = 0;
    }

    void set_py_data_path(const std::string& py_data_path) {
        m_py_data_path_ = py_data_path;
    }

    int conv_char_to_py(const std::string& input_char, std::string& output_py) {
        if (m_py_data_fd_ == -1) {
            if (open_py_data_file() < 0) {
                return -1;
            }
        }

        std::string output_char;
        int rt = check_conv_charset(input_char, output_char);
        if (rt < 0) {
            return -1;
        } else {
            ///
        }

        for (size_t i = 0; i < output_char.length(); ) {
            u_char tmp = (u_char)output_char[i];
            u_char tmp1 = (u_char)output_char[i + 1];
            if (tmp >= 0x80) {
            int off = ((tmp - 0x81) << 8)
                + (tmp1 - 0x40) - (tmp - 0x81) * 0x40;
            if (off < 0) {
                ERROR_LOG("input_str[%s] offset[%d] invalid", input_char.c_str(), off);
                return -1;
            }
            char* p = &m_py_data_[off * 8];
            output_py += p;
            i += 2;
            } else {
                output_py.push_back(tmp);
                i++;
            }
        }

        return 0;
    }

private:
    int open_py_data_file() {
        if ((m_py_data_fd_ = ::open(m_py_data_path_.c_str(), O_RDONLY)) < 0) {
            return -1;
        }

        if ((m_py_data_len_ = ::lseek(m_py_data_fd_, 0, SEEK_END)) < 0) {
            ::close(m_py_data_fd_);
            m_py_data_fd_ = -1;
            return -1;
        }

        m_py_data_ =
            (char*)::mmap(NULL, m_py_data_len_, PROT_READ, MAP_SHARED, m_py_data_fd_, 0);
        if (m_py_data_ == MAP_FAILED) {
            ::close(m_py_data_fd_);
            m_py_data_fd_ = -1;
            return -1;
        }

        return 0;
    }

    int check_conv_charset(const std::string& input_char, std::string& output_char) {
        iconv_t p_iconv = NULL;
       if ((p_iconv = ::iconv_open("gbk", "utf8")) < 0) {
           return -1;
       }

        ///convert utf8 to gbk
        std::string tmp_str(input_char);
        char *p_in = (char*)(tmp_str.c_str());
        char** pp_in = &p_in;
        char out[4096] = {0};
        char* p_out = out;
        char** pp_out = &p_out;
        size_t inlen = input_char.length();
        size_t outlen = sizeof(out);
        int rt = 0;
        if ((rt = ::iconv(p_iconv, pp_in, &inlen, pp_out, &outlen)) < 0) {
            ::iconv_close(p_iconv);
            p_iconv = NULL;
            return -1;
        }

        output_char.assign(out);

        ::iconv_close(p_iconv);
        p_iconv = NULL;

        return 0;
    }

private:
    std::string m_py_data_path_;
    int m_py_data_fd_;
    char* m_py_data_;
    int m_py_data_len_;
};

#endif
