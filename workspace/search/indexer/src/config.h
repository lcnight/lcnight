/**
 *      @brief  header file for config
 *
 *     Created  11/17/2011 08:03:40 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#ifndef  __INDEXER_CONFIG_H__
#define  __INDEXER_CONFIG_H__

#include <string.h>
#include "i_ini_file.h"
#include "i_mysql_iface.h"
#include "util.h"
#include "log.h"

#define     APP_NAME        "indexer"
#define     APP_VER         "0.0.0"
#define     CFG_PATHMAX     256
#define     CFG_STRLEN      32
#define     DEF_LOGSIZE     (32 * 1024 * 1024)
#define     COMMON_BUFSZ    4096
#define     DOC_BUFSZ       (1*1024*1024)

typedef struct logging_config {
    char log_dir[CFG_PATHMAX];
    enum log_lvl log_lvl;
    unsigned int log_size;
    int log_maxfiles;
    char log_prefix[CFG_PATHMAX];
    logging_config(): log_lvl(log_lvl_trace), log_size(DEF_LOGSIZE), log_maxfiles(0) {
        memset(log_dir, 0, CFG_PATHMAX);
        memset(log_prefix, 0, CFG_PATHMAX);
    }
} log_info;

typedef struct socket_address_info {
    char ip[CFG_STRLEN];
    int port;
    socket_address_info(): port(-1) {
        memset(ip, 0, CFG_STRLEN);
    }
} sk_addr;

typedef struct database_config {
    char host[CFG_STRLEN];
    int  port;
    char user[CFG_STRLEN];
    char pass[CFG_STRLEN];
    char db[CFG_STRLEN];
    database_config(): port(3306) {
        memset(host, 0, CFG_STRLEN);
        memset(user, 0, CFG_STRLEN);
        memset(pass, 0, CFG_STRLEN);
        memset(db, 0, CFG_STRLEN);
    }
} db_info;

bool cfg_init(const char* cfg_file_path);

/*log_info *cfg_get_log_info();*/
bool cfg_init_logging();

db_info *cfg_get_src_db_info(bool force);
i_mysql_iface *cfg_get_src_db();

db_info *cfg_get_dst_db_info(bool force);
i_mysql_iface *cfg_get_dst_db();

const char *cfg_get_dicts_str();
fcls_t cfg_get_dicts();

sk_addr *cfg_get_redis_addr();

int cfg_get_index_topn();

#define     PROCESS_FILTER  0x1
#define     PROCESS_INDEX   0x2
#define     PROCESS_BOTH    0x3
int cfg_get_index_mode();

void cfg_end();

#endif  /*__INDEXER_CONFIG_H__*/

