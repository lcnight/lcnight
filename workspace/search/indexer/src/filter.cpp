/**
 *      @brief  filter raw documents in database
 *              //remove: html tags(eg:<img/>), character entities(), control characters
 *
 *     Created  11/17/2011 07:57:52 PM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#include <unistd.h>
#include <ctype.h>
#include "filter.h"

filter::filter(int batch_num):
    src_buf(NULL), dst_buf(NULL), src_title(NULL), dst_title(NULL),
    src_db(NULL), dst_db(NULL), batch_items(batch_num), last_src_did(0), last_dst_did(0)
{
    if (resize(&src_buf, DOC_BUFSZ) || resize(&dst_buf, DOC_BUFSZ)
            || resize(&src_title, COMMON_BUFSZ) || resize(&dst_title, COMMON_BUFSZ)) {
        ERROR_LOG("fail to resize filter title/content buffer");
        return;
    }
}

filter::filter(i_mysql_iface *src_db, i_mysql_iface *dst_db):
    src_buf(NULL), dst_buf(NULL), src_title(NULL), dst_title(NULL),
    src_db(src_db), dst_db(dst_db), batch_items(BATCH_NUM), last_src_did(0), last_dst_did(0)
{
    if (resize(&src_buf, DOC_BUFSZ) || resize(&dst_buf, DOC_BUFSZ)
            || resize(&src_title, COMMON_BUFSZ) || resize(&dst_title, COMMON_BUFSZ)) {
        ERROR_LOG("fail to resize filter title/content buffer");
        return;
    }
}

filter::~filter()
{
    if (src_buf) {
        free(src_buf);
    }
    if (dst_buf) {
        free(dst_buf);
    }
    if (src_title) {
        free(src_title);
    }
    if (dst_title) {
        free(dst_title);
    }
}

int filter::set_src_db(i_mysql_iface *db)
{
    if (db) {
        src_db = db;
        return 0;
    } else {
        return -1;
    }
}

int filter::set_dst_db(i_mysql_iface *db)
{
    if (dst_db) {
        dst_db = db;
        return 0;
    } else {
        return -1;
    }
}

int filter::run(const char* src_tname, const char* dst_tname)
{
    if (!src_tname || !dst_tname) {
        return -1;
    }
    int sleep_interval = 2;
    int timeout = 30;
    docs_map src_docs;
    docs_map dst_docs;
    last_src_did  = last_dst_did = 0;
    strcpy(this->src_tname, src_tname);
    strcpy(this->dst_tname, dst_tname);

    while (true) {
        int src_docs_num = get_batch_src_docs(src_docs);
        int dst_docs_num = get_batch_dst_docs(dst_docs);
        if(src_docs_num > 0 && dst_docs_num > 0) {
            filter_update(src_docs, dst_docs);
        }
        else if (src_docs_num == 0 || dst_docs_num  == 0) {
            int ret = filter_update(src_docs, dst_docs, true);
            return ret;
        } 
        else {
            sleep(sleep_interval);
            sleep_interval = sleep_interval * 2;
            ERROR_LOG("db error %s %s, sleep %d seconds retry", 
                    src_db->get_last_errstr(), dst_db->get_last_errstr(), sleep_interval);
            if (sleep_interval >= timeout) { //timeout break
                goto OP_ERR;
            }
        }
    } /*-- end of while --*/
    return 0;

OP_ERR:
    filter_update(src_docs, dst_docs, true);
    return -1;
}

static char sql_buf[COMMON_BUFSZ] = {0};
int filter::get_batch_src_docs(docs_map &docs)
{
    static char *nptr;
    static int base = 10;
    snprintf(sql_buf, COMMON_BUFSZ - 1,
            "select did,time,lastedit,votes,views from %s where did>'%u' order by did limit %d;",
            this->src_tname, last_src_did, batch_items);

    MYSQL_ROW row = NULL;
    int rows = src_db->select_first_row(&row, "%s", sql_buf);
    DEBUG_SHOW("get %d rows documents from src db", rows);
    if (rows > 0) {
        while (row) {
            doc_key did = strtoul(row[0], &nptr, base);
            doc_token doc_tk = {0, 0, 0, 0};
            doc_tk.create_time = strtoul(row[1], &nptr, base);
            doc_tk.edit_time = strtoul(row[2], &nptr, base);
            doc_tk.vote_times = strtoul(row[3], &nptr, base);
            doc_tk.view_times = strtoul(row[4], &nptr, base);
            docs.insert(std::pair<doc_key, doc_token>(did, doc_tk));
            row = src_db->select_next_row(false);
            if (did > last_src_did) {
                last_src_did = did;
            }
        }
    } else {
        DEBUG_LOG("return %d rows documents from src db, read to the end with db info %s",
                rows, src_db->get_last_errstr());
    }
    return rows;
}

int filter::get_batch_dst_docs(docs_map &docs)
{
    static char *nptr;
    static int base = 10;
    snprintf(sql_buf, COMMON_BUFSZ - 1,
            "select did,c_time,e_time,vote_ts,view_ts from %s where did>'%u' and d_flag=0 order by did limit %d;",
            this->dst_tname, last_dst_did, batch_items);
    MYSQL_ROW row = NULL;
    int rows = dst_db->select_first_row(&row, "%s", sql_buf);
    DEBUG_SHOW("get %d rows documents from dst db", rows);
    if (rows > 0) {
        while (row) {
            doc_key did = strtoul(row[0], &nptr, base);
            doc_token doc_tk = {0, 0, 0, 0};
            doc_tk.create_time = strtoul(row[1], &nptr, base);
            doc_tk.edit_time = strtoul(row[2], &nptr, base);
            doc_tk.vote_times = strtoul(row[3], &nptr, base);
            doc_tk.view_times = strtoul(row[4], &nptr, base);
            docs.insert(std::pair<doc_key, doc_token>(did, doc_tk));
            row = dst_db->select_next_row(false);
            if (did > last_dst_did) {
                last_dst_did = did;
            }
        }
    } else {
        DEBUG_LOG("return %d rows documents from dst db, read to the end with db info %s",
                rows, src_db->get_last_errstr());
    }
    return rows;
}

void filter_txt(uint8_t *src_txt, uint8_t *dst_txt);
int filter::filter_update(docs_map &src, docs_map &dst, bool to_end)
{
    docs_map_it src_it = src.begin();
    docs_map_it dst_it = dst.begin();
    while (src_it != src.end() && dst_it != dst.end()) {
        if (src_it->first == dst_it->first) { //doc matched
            if (src_it->second.edit_time > dst_it->second.edit_time) { //doc updated
                DEBUG_SHOW("update did %u, edit time src %u > dst %u",
                        src_it->first, src_it->second.edit_time, dst_it->second.edit_time);
                if (get_src_db_doc(src_it->first) <= 0) {
                    return -1;
                }
                filter_txt(src_title, dst_title);
                DEBUG_SHOW("document id %u title\nsrc: %s\ndst: %s", src_it->first, src_title, dst_title);
                filter_txt(src_buf, dst_buf);
                DEBUG_SHOW("document id %u content\nsrc: %s\ndst: %s", src_it->first, src_buf, dst_buf);
                if (update_dst_db_doc(src_it->first, src_it->second) <= 0) {
                    return -1;
                }
            }
            else {  //doc unchanged, continue next
                DEBUG_SHOW("did %u, src e_time %u <= dst doc e_tiem %u, to next",
                        src_it->first, src_it->second.edit_time, dst_it->second.edit_time);
            }
            src.erase(src_it ++);
            dst.erase(dst_it ++);
        }
        else if (src_it->first > dst_it->first) { //src documents deleted, delete dst docs
            while (true) {
                if (dst_it == dst.end() || dst_it->first == src_it->first) {
                    break;
                } else {
                    delete_dst_doc(dst_it->first);
                    dst.erase(dst_it ++);
                }
            } /*-- end of while --*/
        }
        else {  //dst documents deleted manually, insert src ones
            WARN_LOG("dst docid %u > src docid %u, dst document may be deleted", dst_it->first, src_it->first);
            while (true) {
                if (src_it == src.end() || dst_it->first == src_it->first) {
                    break;
                } else {
                    if (get_src_db_doc(src_it->first) < 0) { //source db error
                        return -1;
                    }
                    filter_txt(src_title, dst_title);
                    DEBUG_SHOW("title src: %s\ndst: %s", src_title, dst_title);
                    filter_txt(src_buf, dst_buf);
                    DEBUG_SHOW("content src: %s\ndst: %s", src_buf, dst_buf);
                    if (insert_dst_db_doc(src_it->first, src_it->second) < 0) { //destination db error
                        return -1;
                    }
                    src.erase(src_it ++);
                }
            } /*-- end of while --*/
        }
    } /*-- end of while --*/

    //prcess end, new doc inserted, insert new docs
    if (to_end) { //insert all new src docs
OP_TO_END:
        while(src_it != src.end()) {
            doc_key key = src_it->first;
            if (get_src_db_doc(key) <= 0) {
                return -1;
            }
            filter_txt(src_title, dst_title);
            DEBUG_SHOW("did %u title\nsrc: %s\ndst: %s", key, src_title, dst_title);
            filter_txt(src_buf, dst_buf);
            DEBUG_SHOW("did %u content\nsrc: %s\ndst: %s", key, src_buf, dst_buf);
            if (insert_dst_db_doc(src_it->first, src_it->second) <= 0) {
                return -1;
            }
            src.erase(src_it ++);
        }
        int src_docs_num = get_batch_src_docs(src);
        if (src_docs_num > 0) {
            src_it = src.begin();
            goto OP_TO_END;
        }
        return 0;
    } else {
        return 0;
    }
}

int filter::resize(uint8_t **buf, int size)
{
    *buf = (uint8_t*)realloc(*buf, size);

    return 0;
}


inline void memcpy_symbol(uint8_t **ptr, const char *sym, int len) {
    memcpy(*ptr, sym, len);
    *ptr += len;
}
inline void append_space(uint8_t **ptr) 
{
    if (!ptr || !*ptr) {
        return;
    }
    if (*(*ptr - 1) != ' ') {
        //memcpy(*ptr, " ", 1);
        **ptr = ' ';
        *ptr += 1;
    }
}

void filter_txt(uint8_t *src_txt, uint8_t *dst_txt)
{
    uint8_t *sp = src_txt;
    uint8_t *dp = dst_txt;
    int ch = 0;
    while (*sp) {
        ch = *sp;
        if (ch < 0x20) {  //control character
            ++sp;
        }
        else if (ch == ' ') {  //space 
            //if (*(dp-1) != ' ') {
                //memcpy_symbol(&dp, " ", 1);
            //}
            append_space(&dp);
            ++sp;
        }
        else if (ch == '<') {  //html tags
            while (*sp++ != '>') ;
        }
        else if (ch == '&') {  //html entities
            uint8_t *beg = sp;
            while (*sp++ != ';') {
                if (sp - beg > 9) {
                    memcpy_symbol(&dp, "&", 1);
                    //*dp = '&';
                    //++dp;
                    sp = beg + 1;
                    break;
                }
            }
        }
        else if (isupper(ch)) {  //upper to lower
            *dp = (uint8_t)(ch + 32);
            ++dp;
            ++sp;
        }
        else { //filter chinese characters
            int clen = GET_CLEN(*sp);
            unsigned long unicode = Utf82Unicode(sp);
            switch (unicode)
            {
            case 12289 : //、 12289
            case 65292 : //， 65292
            case 65306 : //： 65306
            case 65307 : //； 65307
            case 65374 : //～ 65374
                memcpy_symbol(&dp, ",", 1);
                break;
            case 12290 : //。 12290
                memcpy_symbol(&dp, ".", 1);
                break;
            case 65281 : //！ 65281
                memcpy_symbol(&dp, "!", 1);
                break;
            case 65311 : //？ 65311
                memcpy_symbol(&dp, "?", 1);
                break;
            case 12304 : //【 12304
                memcpy_symbol(&dp, "[", 1);
                break;
            case 12305 : //】 12305
                memcpy_symbol(&dp, "]", 1);
                break;
            case 65288 : //（ 65288
                memcpy_symbol(&dp, "(", 1);
                break;
            case 65289 : //） 65289
                memcpy_symbol(&dp, ")", 1);
                break;
            case '"':
                memcpy_symbol(&dp, "\\\"", 2);
                break;
            case '\'':
                memcpy_symbol(&dp, "\\\'", 2);
                break;
            case 12288 :  //　12288 chinese space
                append_space(&dp);
                break;
                //un-processed chinese Punctuation
                //《 》“”
                //《 12298
                // 》 12299
            default :
                memcpy_symbol(&dp, (const char*)sp, clen);
                break;
            }  /* end of switch */
            sp += clen;
        }
    } /*-- end of while --*/
    *dp = 0;
}

int filter::get_src_db_doc(doc_key did)
{
    snprintf(sql_buf, COMMON_BUFSZ - 1, "select title, content from %s where did = %u;", this->src_tname, did);
    MYSQL_ROW row = NULL;
    int rows = src_db->select_first_row(&row, "%s", sql_buf);
    if (rows > 0) {
        strcpy((char*)src_title, row[0]);
        strcpy((char*)src_buf, row[1]);
    } else if (rows == 0) {
        ERROR_LOG("get %u documents from src db, %s", rows, src_db->get_last_errstr());
    }
    return rows;
}

#define     REPLACE_BUFSZ     (2*DOC_BUFSZ)
static char replace_sql_buf[REPLACE_BUFSZ] = {0};
int filter::insert_dst_db_doc(doc_key did, doc_token& tk)
{
    snprintf(replace_sql_buf, REPLACE_BUFSZ - 1, "insert into %s(did,title,content,view_ts,vote_ts,c_time,e_time,i_time) "
            "values (%u, \"%s\", \"%s\", %u, %u, %u, %u, %u);", this->dst_tname, did, dst_title, dst_buf, tk.view_times, tk.vote_times,
            tk.create_time, tk.edit_time, 0 );
    int rows = dst_db->execsql("%s", replace_sql_buf);
    if (rows <= 0) {
        ERROR_LOG("insert dst db effect %u rows, %s", rows, dst_db->get_last_errstr());
    }
    return  rows;
}

int filter::update_dst_db_doc(doc_key did, doc_token& tk)
{
    snprintf(replace_sql_buf, REPLACE_BUFSZ - 1, "update %s set title=\"%s\",content=\"%s\","
            " view_ts=%u,vote_ts=%u,c_time=%u,e_time=%u where did=%u;", this->dst_tname, dst_title, dst_buf,
            tk.view_times, tk.vote_times, tk.create_time, tk.edit_time, did);
    int rows = dst_db->execsql("%s", replace_sql_buf);
    if (rows <= 0) {
        ERROR_LOG("update dst db docid %u effect %u rows, %s", did, rows, dst_db->get_last_errstr());
    }
    return  rows;
}

int filter::delete_dst_doc(doc_key did)
{
    snprintf(sql_buf, COMMON_BUFSZ - 1, "update %s set d_flag = %u where did=%u;", this->dst_tname, 1, did);
    int rows = dst_db->execsql("%s", sql_buf);
    if (rows < 0) {
        ERROR_LOG("delete dst db docid %u effect %u rows, %s", did, rows, dst_db->get_last_errstr());
    }
    return  rows;
}
