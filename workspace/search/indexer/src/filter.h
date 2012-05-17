/**
 *      @brief  header file for filter
 *
 *     Created  11/17/2011 08:01:44 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#ifndef  __FILTER_H__
#define  __FILTER_H__
#include <inttypes.h>
#include <map>
#include "config.h"
#include "character.h"

#define     BATCH_NUM       100
typedef uint32_t doc_key;
typedef struct document_token {
    uint32_t create_time;
    uint32_t edit_time;
    uint32_t vote_times;
    uint32_t view_times;
} doc_token;
typedef std::map<doc_key, doc_token> docs_map;
typedef std::map<doc_key, doc_token>::iterator docs_map_it;
/*typedef std::queue<doc_token> docs_que;*/

class filter
{
public:
    filter(int batch_num = BATCH_NUM);
    filter(i_mysql_iface *src_db, i_mysql_iface *dst_db);
    ~filter();
    int set_src_db(i_mysql_iface *db);
    int set_dst_db(i_mysql_iface *db);
    int run(const char* src_tname, const char* dst_tname);

private:
    int resize(uint8_t **buf, int size);
    
    /**
     * @brief  get batch-num documents from src db
     * @param t_name    table contain source documents
     * @param docs      container for documents fetched
     * @return >=0 => fetch documents num,  <0 => fetch db error
     */
    int get_batch_src_docs(docs_map &docs);
    int get_batch_dst_docs(docs_map &docs);

    int filter_update(docs_map &src, docs_map &dst, bool to_end = false);


    /**
     * @brief  select, insert, update, delete operation to the src/dst db
     * @param  did  document id
     * @return effect rows, <0 => error
     */
    int get_src_db_doc(doc_key did);
    int insert_dst_db_doc(doc_key did, doc_token& tk);
    int update_dst_db_doc(doc_key did, doc_token& tk);
    int delete_dst_doc(doc_key did);
private:
    uint8_t *src_buf;
    uint8_t *dst_buf;
    uint8_t *src_title;
    uint8_t *dst_title;
    i_mysql_iface *src_db;
    i_mysql_iface *dst_db;
    uint32_t batch_items;
    doc_key last_src_did;
    doc_key last_dst_did;
    char src_tname[CFG_STRLEN];
    char dst_tname[CFG_STRLEN];
}; /* --  end of class  -- */


#endif  /*__FILTER_H__*/
