/**
 *      @brief  header file 
 *
 *     Created  11/22/2011 08:28:22 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#ifndef  __INDEXER_H__
#define  __INDEXER_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <hiredis/hiredis.h>
#include "log.h"
#include "i_ini_file.h"
#include "i_mysql_iface.h"
#include "util.h"
#include "filter.h"
#include "acindex.h"

#define     A_TITLE     0x1
#define     A_CONTENT   0x2
typedef const char *keyword_t;
typedef struct keyword_index_value {
    uint32_t did;
    uint32_t flag;          // denote reverse index relate document title/content
    uint32_t offset[0];
}__attribute__((packed)) keyword_idx_st, *keyword_idx_t;

void usage();

void handle_args(int argc, char *argv[]);

/**
 * @brief  sequentially index documents one by one
 * @param  db       documents stored database
 * @param  idxer    real indexer class
 * @param  topn     denote orderly operate top n documents
 * @return  0 => success, others => error code
 */
int idx_documents(i_mysql_iface *db, acidx *idxer, uint32_t topn);

int set_redis_idxs(redisContext *rds_c, uint32_t did, char *txt, uint32_t idx_flag, 
        phrase_list &phrases, range_list &ranges);

int set_redis_db(i_mysql_iface *db, redisContext *rds_c);
int update_redis_db(i_mysql_iface *db, int dbid);
#endif  /*__INDEXER_H__*/
