/**
 *      @brief  struct used for document sorting
 *
 *     Created  03/07/2012 02:03:21 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#ifndef  __DOC_SORT_HPP__
#define  __DOC_SORT_HPP__

#include <stdlib.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include <map>
#include <set>

using std::string;
using std::vector;
using std::multimap;
using std::map;
using std::set;
using std::pair;

typedef struct doc_view_vote {
    doc_view_vote():view(0), vote(0) 
    { }
    uint32_t view;
    uint32_t vote;
} view_vote, *view_vote_t;

typedef set<uint32_t> offsets;
typedef set<uint32_t>::iterator offsets_it;
typedef map<string, offsets> keyword_offset; 
typedef map<string, offsets>::iterator keyword_offset_it; 

typedef struct doc_info {
    doc_info(): did(0), snippt_title(0), snippt_content(0) {}
    ~doc_info() {
        if (snippt_title) {
            free(snippt_title);
        }
        if (snippt_content) {
            free(snippt_content);
        }
    }
    uint32_t did;
    keyword_offset off_title; 
    char *snippt_title;
    keyword_offset off_content; 
    char *snippt_content;
} doc_info, *doc_info_t;

// map ::= { <doc_id => doc_info>, ... }
typedef map<uint32_t, doc_info> sorted_docs;
typedef map<uint32_t, doc_info>::iterator sorted_docs_it;

// map ::= { <doc_rank, doc_id> }
typedef multimap<uint32_t, uint32_t> rank_docid;
typedef multimap<uint32_t, uint32_t>::iterator rank_docid_it;
typedef multimap<uint32_t, uint32_t>::reverse_iterator re_rank_docid_it;

// map ::= { <doc_rank => doc_info>, ... }
typedef multimap<uint32_t, doc_info> rank_doc;
typedef multimap<uint32_t, doc_info>::iterator rank_doc_it;
typedef multimap<uint32_t, doc_info>::reverse_iterator  re_rank_doc_it;

// map ::= { <doc_id => view & vote>, ... }
typedef map<uint32_t, view_vote> doc_vv;
typedef map<uint32_t, view_vote>::iterator doc_vv_it;

typedef vector<uint32_t> docid_list;
typedef vector<uint32_t>::iterator docid_list_it;

#define MAX_OFFSET  0xFFFFFFF
#define TO_STR(var) #var

uint32_t get_rank(uint32_t did, doc_info& info, view_vote& vv);


#endif  /*__DOC_SORT_HPP__*/
