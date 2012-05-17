/**
 *      @brief  keep configuration information
 *
 *     Created  11/17/2011 08:03:25 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"

static i_ini_file *cfg_ini_file = NULL;
static db_info *src_db_info = NULL;
static db_info *dst_db_info = NULL;
static i_mysql_iface *src_db = NULL;
static i_mysql_iface *dst_db = NULL;
static sk_addr *redis_addr = NULL;
static fcls_t dicts = NULL;
static char *dicts_str = NULL;

bool cfg_init(const char* cfg_file_path)
{
    if (!cfg_file_path) { return false; }
    if (create_ini_file_instance(&cfg_ini_file)) { return false; }
    if (cfg_ini_file->init(cfg_file_path)) { return false; }

    src_db_info = new db_info;
    if (cfg_ini_file->read_string("src_db", "host", src_db_info->host, CFG_STRLEN, "localhost")
            || cfg_ini_file->read_string("src_db", "user", src_db_info->user, CFG_STRLEN, "")
            || cfg_ini_file->read_string("src_db", "passwd", src_db_info->pass, CFG_STRLEN, "")
            || cfg_ini_file->read_string("src_db", "db", src_db_info->db, CFG_STRLEN, "")
       ) {
        return false;
    } else {
        char *ptr = strchr(src_db_info->host, ':');
        if (ptr) {
            *ptr++ = 0;
            src_db_info->port = atoi(ptr);
        }
    }

    dst_db_info = new db_info;
    if (cfg_ini_file->read_string("dst_db", "host", dst_db_info->host, CFG_STRLEN, "localhost")
            || cfg_ini_file->read_string("dst_db", "user", dst_db_info->user, CFG_STRLEN, "")
            || cfg_ini_file->read_string("dst_db", "passwd", dst_db_info->pass, CFG_STRLEN, "")
            || cfg_ini_file->read_string("dst_db", "db", dst_db_info->db, CFG_STRLEN, "")
       ) {
        return false;
    } else {
        char *ptr = strchr(dst_db_info->host, ':');
        if (ptr) {
            *ptr++ = 0;
            dst_db_info->port = atoi(ptr);
        }
    }

    return true;
}

bool cfg_init_logging()
{
    enum log_lvl log_level = log_lvl_trace;
    log_dest_t log_dst = log_dest_both;
    char log_dir[CFG_PATHMAX] = {0};
    char log_prefix[CFG_PATHMAX] = {0};
    int log_maxfiles = 0;
    unsigned int log_size = DEF_LOGSIZE;
    char tmp_buf[CFG_PATHMAX] = {0};

    if (cfg_ini_file->read_string("log", "log_dir", log_dir, CFG_PATHMAX, "./log")) {
        return false;
    } else {
       if (!path_exist(log_dir)) {
           if (mkdir(log_dir, 0777)) {
               fprintf(stderr, "%s - %s\n", log_dir, strerror(errno));
               return false;
           }
       }
    }

    if (cfg_ini_file->read_string("log", "log_prefix", log_prefix, CFG_PATHMAX, "idxer_")) {
        return false;
    }

    if (cfg_ini_file->read_string("log", "log_dst", tmp_buf, CFG_PATHMAX, "both")) {
        return false;
    } else {
        if (!strncasecmp(tmp_buf, "both", 4)) {
            log_dst = log_dest_both;
        } else if (!strncasecmp(tmp_buf, "file", 4)) {
            log_dst = log_dest_file;
        } else if (!strncasecmp(tmp_buf, "stdout", 6)) {
            log_dst = log_dest_terminal;
        } else {
            fprintf(stderr, "logging destination error, set to both\n");
            log_dst = log_dest_both;
        }
    }

    log_level = (enum log_lvl)cfg_ini_file->read_int("log", "log_lvl", log_level);
    log_size = cfg_ini_file->read_int("log", "log_size", DEF_LOGSIZE);
    log_maxfiles = cfg_ini_file->read_int("log", "log_maxfiles", 0);

    log_init(log_dir, log_level, log_size, log_maxfiles, log_prefix);
    set_log_dest(log_dst);
    return true;
}

db_info *cfg_get_src_db_info(bool force)
{
    if (!force && src_db_info) {
        return src_db_info;
    }

    src_db_info = new db_info;
    if (cfg_ini_file->read_string("src_db", "host", src_db_info->host, CFG_STRLEN, "localhost")
            || cfg_ini_file->read_string("src_db", "user", src_db_info->user, CFG_STRLEN, "")
            || cfg_ini_file->read_string("src_db", "passwd", src_db_info->pass, CFG_STRLEN, "")
            || cfg_ini_file->read_string("src_db", "db", src_db_info->db, CFG_STRLEN, "")
       ) {
        delete src_db_info;
        src_db_info = NULL;
    } else {
        char *ptr = strchr(src_db_info->host, ':');
        if (ptr) {
            *ptr++ = 0;
            src_db_info->port = atoi(ptr);
        }
    }
    return src_db_info;
}

i_mysql_iface *cfg_get_src_db()
{
    if (src_db) {
        return src_db;
    }
    if (!src_db_info) {
        cfg_get_src_db_info(true);
    }
    create_mysql_iface_instance(&src_db);
    if (src_db->init(src_db_info->host, src_db_info->port, src_db_info->db, 
            src_db_info->user, src_db_info->pass, "utf8")
       ){
        fprintf(stderr, "fail to init src db, %s\n", src_db->get_last_errstr());  
        src_db->release();
        src_db = NULL;
    }
    return src_db;
}

db_info *cfg_get_dst_db_info(bool force)
{
    if (!force && dst_db_info) {
        return dst_db_info;
    }

    dst_db_info = new db_info;
    if (cfg_ini_file->read_string("dst_db", "host", dst_db_info->host, CFG_STRLEN, "localhost")
            || cfg_ini_file->read_string("dst_db", "user", dst_db_info->user, CFG_STRLEN, "")
            || cfg_ini_file->read_string("dst_db", "passwd", dst_db_info->pass, CFG_STRLEN, "")
            || cfg_ini_file->read_string("dst_db", "db", dst_db_info->db, CFG_STRLEN, "")
       ) {
        delete dst_db_info;
        dst_db_info = NULL;
    } else {
        char *ptr = strchr(dst_db_info->host, ':');
        if (ptr) {
            *ptr++ = 0;
            dst_db_info->port = atoi(ptr);
        }
    }
    return dst_db_info;
}

i_mysql_iface *cfg_get_dst_db()
{
    if (dst_db) {
        return dst_db;
    }
    if (!dst_db_info) {
        cfg_get_dst_db_info(true);
    }
    create_mysql_iface_instance(&dst_db);
    if (dst_db->init(dst_db_info->host, dst_db_info->port, dst_db_info->db, 
            dst_db_info->user, dst_db_info->pass, "utf8")
       ){
        fprintf(stderr, "fail to init src db, %s\n", dst_db->get_last_errstr());  
        dst_db->release();
        dst_db = NULL;
    }
    return dst_db;
}

const char *cfg_get_dicts_str()
{
    if (dicts_str) {
        return dicts_str;
    }

    dicts_str = (char*)malloc(COMMON_BUFSZ);

    if (cfg_ini_file->read_string("dict", "path", dicts_str, COMMON_BUFSZ, "")) {
        free(dicts_str);
        dicts_str = NULL;
    }
    return dicts_str;
}

fcls_t cfg_get_dicts()
{
    if (dicts) {
        return dicts;
    }

    dicts = fcls_new();
    char *buf = (char*)malloc(COMMON_BUFSZ);
    if (cfg_ini_file->read_string("dict", "path", buf, COMMON_BUFSZ, "")) {
        fcls_free_all(&dicts);
    } else {
        char *saveptr = NULL;
        char *ptr = strtok_r(buf, ",", &saveptr);
        while (ptr) {
            while(*ptr == ' ' || *ptr == '\t') ptr++;
            fcls_add_cpy(dicts, ptr);
            ptr = strtok_r(NULL, ",", &saveptr);
        } /*-- end of while --*/

        // do check,  test the dicts file existed?
        fcls_t it_fcls = dicts;
        fcls_rewind(it_fcls);
        int dict_num = 0;
        char *file = NULL;
        file = fcls_next(&it_fcls);
        while(file) {
            dict_num ++;
            if (file && !path_exist(file)) {
                fprintf(stderr, "dict [%s] not exist!\n", file);
                it_fcls->count = dict_num;
            }
            file = fcls_next(&it_fcls);
        }
    }
    free(buf);
    return dicts;
}

sk_addr *cfg_get_redis_addr()
{
    if (redis_addr) {
        return redis_addr;
    } 

    redis_addr = new sk_addr;

    redis_addr->port = 6379;
    if (cfg_ini_file->read_string("redis", "server", redis_addr->ip, CFG_STRLEN, "localhost")) {
        delete redis_addr;
        redis_addr = NULL;
    } else {
        char *ptr = strchr(redis_addr->ip, ':');
        if (ptr) {
            *ptr++ = 0;
            redis_addr->port = atoi(ptr);
        }
    }
    return redis_addr;
}

int cfg_get_index_topn()
{
    if (cfg_ini_file) {
        return cfg_ini_file->read_int("index", "topn", -1);
    }
    return -1;
}

int cfg_get_index_mode()
{
    if (!cfg_ini_file) {
        return PROCESS_BOTH;
    }

    char buf[CFG_STRLEN] = {0};
    if (!cfg_ini_file->read_string("index", "process", buf, CFG_STRLEN, "both")) {
        if (!strcasecmp(buf, "filter")) {
            return PROCESS_FILTER;
        } 
        else if (!strcasecmp(buf, "index")) {
            return PROCESS_INDEX;
        } else {
            return PROCESS_BOTH;
        }
    }
    return PROCESS_BOTH;
}

void cfg_end()
{
    if (cfg_ini_file) {
        cfg_ini_file->release();
    }
    if (src_db) {
        src_db->release();
    }
    if (dst_db) {
        dst_db->release();
    }
    if (dicts_str) {
        free(dicts_str);
        dicts_str = NULL;
    }
    if (src_db_info) {
        delete src_db_info;
        src_db_info = NULL;
    }
    if (dst_db_info) {
        delete dst_db_info;
        dst_db_info = NULL;
    }
    if (redis_addr) {
        delete redis_addr;
        redis_addr = NULL;
    }

    fcls_free_all(&dicts);

    log_fini();
}
