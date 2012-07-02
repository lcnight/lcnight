/**
 *      @brief impl of rank function
 *
 *     Created  03/07/2012 09:54:48 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include "doc_sort.hpp"

inline uint32_t get_newest_rank(uint32_t did)
{
    uint32_t ret = 0;
    uint32_t magic = 0xF0000000;
    for (int i = 0 ; i < 8 ; i++) {
        if ((did & (magic >> 4*i)) > 0) {
            ret = 8 - i;
            break;
        }
    } /*-- end of for --*/
    return ret;
}

inline uint32_t get_doc_offset_rank(doc_info& info)
{
    keywords uniqkeys;
    keyword_offset_it it;
    int title_keys_num = info.off_title.size();
    int title_key_offset_num = 0;
    uint32_t title_key_leftmost = 0;
    for (it = info.off_title.begin(); it != info.off_title.end() ; it++) {
        uniqkeys.insert(it->first);
        int offnum = it->second.size();
        title_key_offset_num += offnum;
        if (offnum == 0) {
            continue;
        }
        uint32_t maxoff = *(it->second.rbegin());
        for(offsets_it oit = it->second.begin(); oit != it->second.end(); ++ oit) {
            title_key_leftmost += (maxoff - *oit);
        }
    } /*-- end of for --*/
    
    int content_keys_num = info.off_content.size();
    int content_key_offset_num = 0;
    for (it = info.off_content.begin(); it != info.off_content.end() ; it++) {
        uniqkeys.insert(it->first);
        content_key_offset_num += it->second.size();
    } /*-- end of for --*/

    // rank = title_keys  + content_keys + title_keys_leftmost + uniqkeys
    uint32_t rank = 5 * (8 * title_keys_num + 2 * title_key_offset_num) 
        + 8 * content_keys_num + 2 * content_key_offset_num
        +  3 * title_key_leftmost + (uniqkeys.size() - 1) * 95;
    
    return rank;
}

//#define get_user_rank(vv) (vv.view + 2 * vv.vote)
inline uint32_t get_user_rank(view_vote& vv)
{
    //return (vv.view / 2 + 2 * vv.vote);
    return (vv.vote);
}

uint32_t get_rank(uint32_t did, doc_info& info, view_vote& vv)
{
    uint32_t new_rank = get_newest_rank(did);
    uint32_t idx_rank = get_doc_offset_rank(info);
    uint32_t use_rank = get_user_rank(vv);

    return new_rank + idx_rank + use_rank;
}


