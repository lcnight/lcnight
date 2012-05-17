/**
 * =====================================================================================
 *      @brief 过滤和索引文章的主程序
 *              
 *     Created  11/17/2011 09:58:53 AM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include "indexer.h"

static char *cfg_file_path;

int main(int argc, char *argv[])
{
    handle_args(argc, argv);
    if (!path_exist(cfg_file_path)) {
        PRINT("config file %s not existed!", cfg_file_path);
        usage();
    }

    if (!cfg_init(cfg_file_path) || !cfg_init_logging()) {
        PRINT("initialize configuration or logging system fail and exit!");
        exit(1);
    }

    fprintf(stderr, "start to run: indexer %s\n", cfg_file_path);

    int process_mode = cfg_get_index_mode();

    // --------------------------------------------------------------------
    //       start to filter all documents to desctination database             
    // --------------------------------------------------------------------
    if (process_mode & PROCESS_FILTER) {
        DEBUG_LOG("start to run filter ... ");
        filter *p_flter = new filter(cfg_get_src_db(), cfg_get_dst_db());
        p_flter->run("t_doc", "t_doc");
        delete p_flter;
        DEBUG_LOG("end filter");
    }

    // --------------------------------------------------------------------
    //        start to build index info for all filtered documents             
    // --------------------------------------------------------------------
    if (process_mode & PROCESS_INDEX) {
        DEBUG_LOG("start to run index ... ");
        acidx *idxer = new acidx(cfg_get_dicts_str());

        if (idxer->init()) {
            ERROR_LOG("fail to init indexer");
            delete idxer;
            return -1;
        }

        //bool exist = idxer->query_phrase("mole");
        //DEBUG_LOG("mole %d", exist);
        int cfg_topn = cfg_get_index_topn();
        if (idx_documents(cfg_get_dst_db(), idxer, cfg_topn)) {
            ERROR_LOG("fail to build documents indexer");
        }
        delete idxer;
        DEBUG_LOG("end index");

    }

    cfg_end();
    fprintf(stderr, "\nrun to end ...\n");
    return 0;
}/* -- end of main  -- */

void usage() {
    printf("Usage: %s <cfg_path>\n", APP_NAME);
    printf("   or: %s -v         show app version infomation\n\n", APP_NAME);
    exit(0);
}

void handle_args(int argc, char *argv[])
{
    for (int i=1 ; i < argc ; i++) {
        if (strcasestr(argv[i], "-v")) {
            printf("%s %s build at: %s %s\n", APP_NAME, APP_VER, __DATE__, __TIME__);
            exit(0);
        }
    }
    if (argc == 1) {
        usage();
    }
    cfg_file_path = argv[1];
}

int idx_documents(i_mysql_iface *db, acidx *idxer, uint32_t topn)
{
    if (!db || !idxer) {
        return -1;
    }
    sk_addr *r_addr = cfg_get_redis_addr();
    redisContext *rds_c = redisConnect(r_addr->ip, r_addr->port);
    if (rds_c->err) {
        ERROR_LOG("fail to connect redis server [%s:%d], %s", r_addr->ip, r_addr->port, rds_c->errstr);
        return -1;
    }
    int redis_dbid = set_redis_db(db, rds_c);
    if (redis_dbid == -1) {
        return -1;
    }

    uint32_t last_did = 0;
    uint32_t docs_num = 0;
    static char content_buf[DOC_BUFSZ] = {0};
    static char title_buf[COMMON_BUFSZ] = {0};
    static char sql_buf[COMMON_BUFSZ] = {0};
    int get_docs_num = 0;
    MYSQL_ROW row = NULL;
    phrase_list phrases;
    range_list ranges;
    while (docs_num < topn) {
        docs_num ++;
        snprintf(sql_buf, COMMON_BUFSZ - 1, 
                "select did, title, content from t_doc where did>%u and d_flag = 0 order by did limit 1;", last_did);
        get_docs_num = db->select_first_row(&row, "%s", sql_buf);
        if (get_docs_num <= 0) {
            DEBUG_LOG("index %u documents, last did %u, break", get_docs_num, last_did);
            break;
        }
        char *nptr = NULL;
        uint32_t did = strtoul(row[0], &nptr, 10);
        strcpy(title_buf, row[1]);
        strcpy(content_buf, row[2]);
        //DEBUG_SHOW("%u %s %s", did, title_buf, content_buf);

        if (last_did < did) {
            last_did = did;
        }

        phrases.clear();
        ranges.clear();
        if (idxer->query_phrases(title_buf, phrases, ranges)) {
            ERROR_LOG("fail to build indexer for did %u, title: %s", did, title_buf);
            return -1;
        }
        uint32_t idx_flag = A_TITLE;
        if (set_redis_idxs(rds_c, did, title_buf, idx_flag, phrases, ranges)) {
            return -1;
        }

        phrases.clear();
        ranges.clear();
        if (idxer->query_phrases(content_buf, phrases, ranges)) {
            ERROR_LOG("fail to build indexer for did %u, content: %s", did, content_buf);
            return -1;
        }
        idx_flag = A_CONTENT;
        if (set_redis_idxs(rds_c, did, content_buf, idx_flag, phrases, ranges)) {
            return -1;
        }
        KNOTI_LOG(did, "end index one\n");
    } /*-- end of while --*/
   
    if (update_redis_db(db, redis_dbid)) {
        return -1;
    }

    DEBUG_LOG("success to index %u documents", docs_num);
    return 0;
}

int set_redis_idxs(redisContext *rds_c, uint32_t did,  char *txt, uint32_t idx_flag, 
        phrase_list &phrases, range_list &ranges)
{
    if (!rds_c) {
        return -1;
    }
    if (!phrases.size() && !ranges.size()) {
        KNOTI_LOG(did, "flag %u: %s no phrases and ranges found", idx_flag, txt);
        return 0;
    }
    KNOTI_LOG(did, "flag %u: found %lu phrases, %lu ranges", idx_flag, phrases.size(), ranges.size());
    KNOTI_SHOW(did, "txt: %s", txt);
    
    redisReply *rds_r = NULL;
    static char ridx_buf[COMMON_BUFSZ] = {0};
    uint32_t idx_len = 0;
    uint32_t idx_occurs = 0;
    uint8_t *k_idx = NULL;

    phrase_list_it p_it = phrases.begin();
    for ( ; p_it != phrases.end() ; ++p_it) {
        k_idx = (uint8_t *)ridx_buf;
        pack(&k_idx, &did, 4);
        pack(&k_idx, &idx_flag, 4);
        idx_len = 8;
        idx_occurs = 0;
        offset_list_it  l_it = p_it->second.begin();
        for (; l_it != p_it->second.end() ; ++l_it) {
            uint32_t offset = *l_it;
            uint32_t char_num = bytes2char_num(txt, offset);
            pack(&k_idx, &char_num, 4);
            idx_len += 4;
            ++ idx_occurs;
        }
        rds_r = (redisReply *)redisCommand(rds_c, "zadd %s %u %b", p_it->first, idx_occurs, ridx_buf, idx_len);
        if (!rds_r) {
            ERROR_LOG("set key %s:r%d, errorstr: %s", p_it->first, idx_occurs, rds_c->errstr);
            return -1;
        }
        freeReplyObject(rds_r);
    } /*-- end of for --*/

    range_list_it r_it = ranges.begin();
    range_list_it last_range = r_it;
    KNOTI_LOG(did, "log un-dictionary phrases ...");
    for ( ; r_it != ranges.end() ; ++r_it) {
        KNOTI_LOG(did, "[%u, %u)", r_it->begin, r_it->end);
        if (last_range == r_it) {
            memcpy(ridx_buf, txt, r_it->begin);
            ridx_buf[r_it->begin] = 0;
        } else {
            uint32_t len = r_it->begin - last_range->end;
            memcpy(ridx_buf, txt + last_range->end , len);
            ridx_buf[len] = 0;
        }
        last_range = r_it;
        if (strlen(ridx_buf)) {
            KNOTI_LOG(did, "%s", ridx_buf);
        }
    } /*-- end of for --*/
    //log last part
    KNOTI_LOG(did, "%s", txt + last_range->end);

    return 0;
}

int set_redis_db(i_mysql_iface *db, redisContext *rds_c)
{
    if (!db || !rds_c) {
        return -1;
    }

    char buf[1024] = {0};
    snprintf(buf, 1023, "select id, i_time from t_redis_db order by i_time desc limit 1;");
    MYSQL_ROW row = NULL;
    int rows = db->select_first_row(&row, "%s", buf);
    int r_db = -1;
    if (rows > 0) {
        r_db = atoi(row[0]);
        r_db ^= 1; //change redis db 
    } else if (rows == 0) {
        r_db = 0;
    } else {
        ERROR_LOG("error to fetch redis db");
        return -1;
    }

    redisReply *rds_r = (redisReply *)redisCommand(rds_c, "select %u", r_db);
    if (!rds_r) {
        ERROR_LOG("error to set redis db %u, errorstr: %s", r_db, rds_c->errstr);
        return -1;
    }
    freeReplyObject(rds_r);

    return r_db;
}

int update_redis_db(i_mysql_iface *db, int dbid)
{
    if (!db) {
        return -1;
    }
    uint32_t cur_time = time(NULL); 
    int rows = db->execsql("insert into t_redis_db (id, i_time) values (%d, %u);", dbid, cur_time);
    if (rows != 1) {
        return -1;
    } else {
        return 0;
    }
}
