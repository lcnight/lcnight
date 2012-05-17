/**
 * @file main.cpp
 * @brief
 * @author jerryshao jerryshao@taomee.com
 * @version
 * @date 2011-12-08
 */
#include <signal.h>

#include <iostream>
#include <boost/shared_ptr.hpp>

#include "utility/i_ini_file.h"
#include "word-segment/word_segment_pool.hpp"
#include "daemon.h"
#include "tcp_server.hpp"

char** g_saved_argv = NULL;
int g_saved_argc = 0;
char* g_p_current_dir = NULL;
char* g_prog_name = NULL;

volatile sig_atomic_t g_stop = 0;
volatile sig_atomic_t g_restart = 0;

static int log_init(i_ini_file* p_ini_file);
static int redis_init(i_ini_file* p_ini_file, redis_connect_pool** p_pool);
static int mysql_init(i_ini_file* p_ini_file, mysql_connect_pool** p_pool);
static int segment_module_init(i_ini_file* p_ini_file, word_segment_pool** p_word_segment);

#define VERSION "search engine 0.5"

int main(int argc, char* argv[])
{
    ::std::cout<< VERSION << " build at " << __DATE__ << " " << __TIME__ << ::std::endl;

    if (argc != 2) {
        std::cout << "usage:" << argv[0] << " conf_file" << std::endl;
        return 0;
    }

    i_ini_file* p_ini_file;
    if (create_ini_file_instance(&p_ini_file) < 0) {
        std::cerr << "create_ini_file_instance failed" << std::endl;
        return -1;
    }

    if (p_ini_file->init(argv[1]) < 0) {
        std::cerr << "ini_file init failed" << std::endl;
        return -1;
    }

    ///开启精灵模式
    if (daemon_start(argc, argv) < 0) {
        return -1;
    }

    ///初始化日志系统
    if (log_init(p_ini_file) < 0) {
        return -1;
    }

    ///设置进程标题
    //daemon_set_title("%s-[%s]", g_prog_name, "search-engine");

    ///初始化redis和mysql的连接池
    redis_connect_pool* pool = NULL;
    if (redis_init(p_ini_file, &pool) < 0) {
        ERROR_LOG("redis_connect_pool init failed");
        return -1;
    }
    boost::shared_ptr<redis_connect_pool> redis_conn_pool_ptr(pool);

    mysql_connect_pool* mysql_pool = NULL;
    if (mysql_init(p_ini_file, &mysql_pool) < 0) {
        ERROR_LOG("mysql_connect_pool init failed");
        return -1;
    }
    boost::shared_ptr<mysql_connect_pool> mysql_conn_pool_ptr(mysql_pool);

    ///初始化搜索关键词处理引擎
    key_process_engine key_process_engine_inst;

    ///初始化拼应转换模块
    char py_data_path[PATH_MAX] = {0};
    if (p_ini_file->read_string("engine",
                                "pinyin_data_path",
                                py_data_path,
                                sizeof(py_data_path),
                                NULL)) {
        ERROR_LOG("fail to get pinyin_data_path from conf file");
        return -1;
    }

    char_to_py char_to_py_inst(py_data_path);

    ///初始化读写锁
    pthread_rwlock_t rw_lock;
    if (::pthread_rwlock_init(&rw_lock, NULL)) {
        ERROR_LOG("pthread_rwlock_init failed, %s", ::strerror(errno));
        return -1;
    }
    boost::shared_ptr<pthread_rwlock_t> rwlock_ptr(&rw_lock, ::pthread_rwlock_destroy);

    ///搜索引擎关键字处理引擎构建
    int redis_table_id = -1;
    if (key_process_engine_inst.engine_build(rwlock_ptr.get(),
                                             redis_conn_pool_ptr->get_next_redis_conn(),
                                             mysql_conn_pool_ptr->get_next_mysql_conn(),
                                             char_to_py_inst,
                                             &redis_table_id) < 0) {
        ERROR_LOG("key_process_engine engine_build failed");
        return -1;
    }

    ///初始化分词模块
    word_segment_pool* p_word_segment = NULL;
    if (segment_module_init(p_ini_file, &p_word_segment) < 0) {
        ERROR_LOG("segment_module_init failed");
        return -1;
    }
    boost::shared_ptr<word_segment_pool> word_segment_pool_ptr(p_word_segment);

    ///初始化核心搜索排序模块
    search_kernel search_kernel_inst;

    ///初始化tcp服务端
    int redis_cache_table = p_ini_file->read_int("redis_conf", "redis_cache_table", 2);

    char tcp_host_ip[16] = {0};
    u_short tcp_host_port = 0;
    if (p_ini_file->read_string("engine", "engine_host", tcp_host_ip, sizeof(tcp_host_ip), NULL)
        || !(tcp_host_port = p_ini_file->read_int("engine", "engine_port", 0))) {
        ERROR_LOG("fail to read engine host info from conf file");
        return -1;
    }

    int engine_thread_num = p_ini_file->read_int("engine", "thread_num", 4);

    char url_prefix[PATH_MAX] = {0};
    if (p_ini_file->read_string("engine", "url_prefix", url_prefix, sizeof(url_prefix), NULL)) {
        ERROR_LOG("fail to read url_prefix from conf file");
        return -1;
    }
    std::string url_str(url_prefix);

    if (p_ini_file) {
        p_ini_file->uninit();
        p_ini_file->release();
        p_ini_file = NULL;
    }

    try {
        boost::shared_ptr<tcp_server> tcp_serv_ptr(
                new tcp_server(std::string(tcp_host_ip),
                               tcp_host_port,
                               engine_thread_num,
                               key_process_engine_inst,
                               search_kernel_inst,
                               char_to_py_inst,
                               *redis_conn_pool_ptr,
                               *mysql_conn_pool_ptr,
                               *word_segment_pool_ptr,
                               rwlock_ptr.get(),
                               &redis_table_id,
                               redis_cache_table,
                               url_str));

        tcp_serv_ptr->run();

        while (!g_stop && !g_restart) {
            ::usleep(100000);
        }
        tcp_serv_ptr->stop();

    } catch (std::exception& e) {
        ERROR_LOG("exception: %s", e.what());
    }

    daemon_stop();
    return 0;
}

////////////////////////////////////////////////////////////////////////

static int log_init(i_ini_file* p_ini_file)
{
    char log_dir[PATH_MAX] = {0};
    if (p_ini_file->read_string("log_info", "log_dir", log_dir, PATH_MAX, NULL)) {
        return -1;
    }

    int log_level = p_ini_file->read_int("log_info", "log_level", 8);
    int log_size = p_ini_file->read_int("log_info", "log_size", 1024*1024*100);
    int log_count = p_ini_file->read_int("log_info", "log_maxfiles", 0);

    char log_prefix[NAME_MAX] = {0};
    if (p_ini_file->read_string("log_info", "log_prefix", log_prefix, NAME_MAX, NULL)) {
        return -1;
    }

    if (log_init(log_dir, (log_lvl_t)log_level, log_size, log_count, log_prefix)) {
        return -1;
    }

    enable_multi_thread();
    return 0;
}

static int redis_init(i_ini_file* p_ini_file, redis_connect_pool** p_pool)
{
    char redis_host[16] = {0};
    u_short redis_port = 0;

    if (p_ini_file->read_string("redis_conf", "redis_host", redis_host, sizeof(redis_host), NULL)
        || !(redis_port = p_ini_file->read_int("redis_conf", "redis_port", 6379))) {
        ERROR_LOG("read redis conf from conf file failed, %s", p_ini_file->get_last_errstr());
        return -1;
    }

    int redis_pool_size = p_ini_file->read_int("redis_conf", "redis_pool_size", 4);

    *p_pool = new redis_connect_pool(redis_pool_size);
    if ((*p_pool)->redis_conn_pool_init(redis_host, redis_port) < 0) {
        ERROR_LOG("redis_connect_pool init failed");
        delete *p_pool;
        *p_pool = NULL;
        return -1;
    }

    return 0;
}

static int mysql_init(i_ini_file* p_ini_file, mysql_connect_pool** p_pool)
{
    char mysql_host[16] = {0};
    u_short mysql_port = 0;
    char mysql_db[NAME_MAX] = {0};
    char mysql_user[NAME_MAX] = {0};
    char mysql_passwd[NAME_MAX] = {0};
    char mysql_charset[NAME_MAX] = {0};

    if (p_ini_file->read_string("mysql_conf", "mysql_host", mysql_host, sizeof(mysql_host), NULL)
        || !(mysql_port = p_ini_file->read_int("mysql_conf", "mysql_port", 3306))
        || p_ini_file->read_string("mysql_conf", "mysql_db", mysql_db, sizeof(mysql_db), NULL)
        || p_ini_file->read_string("mysql_conf", "mysql_user", mysql_user, sizeof(mysql_user), NULL)
        || p_ini_file->read_string("mysql_conf", "mysql_passwd", mysql_passwd, sizeof(mysql_passwd), NULL)
        || p_ini_file->read_string("mysql_conf", "mysql_charset",mysql_charset, sizeof(mysql_charset), NULL)) {

        ERROR_LOG("read mysql conf from conf file failed, %s", p_ini_file->get_last_errstr());
        return -1;
    }

    int mysql_pool_size = p_ini_file->read_int("mysql_conf", "mysql_pool_size", 4);

    *p_pool = new mysql_connect_pool(mysql_pool_size);
    if ((*p_pool)->mysql_conn_pool_init(mysql_host,
                                        mysql_port,
                                        mysql_db,
                                        mysql_user,
                                        mysql_passwd,
                                        mysql_charset) < 0) {
        ERROR_LOG("mysql_connect_pool init failed");
        delete *p_pool;
        *p_pool = NULL;
        return -1;
    }

    return 0;
}

static int segment_module_init(i_ini_file* p_ini_file, word_segment_pool** p_word_segment)
{
    char charset[32] = {0};
    char dict_path[PATH_MAX] = {0};
    char rule_path[PATH_MAX] = {0};
    size_t pool_size = 0;

    if (p_ini_file->read_string("engine", "word_segment_charset", charset, sizeof(charset), NULL)
        || p_ini_file->read_string("engine", "word_segment_dict_path", dict_path, sizeof(dict_path), NULL)
        || p_ini_file->read_string("engine", "word_segment_rule_path", rule_path, sizeof(rule_path), NULL)) {
        ERROR_LOG("read word_segment conf from conf file failed, %s", p_ini_file->get_last_errstr());
        return -1;
    }

    pool_size = p_ini_file->read_int("engine", "word_segment_pool_size", 4);

    *p_word_segment = new word_segment_pool(charset, dict_path, rule_path, pool_size);

    if ((*p_word_segment)->word_segment_pool_init() < 0) {
        ERROR_LOG("word_segment init failed");
        delete *p_word_segment;
        *p_word_segment = NULL;
        return -1;
    }

    return 0;
}
