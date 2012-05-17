/**
 *      @brief  header file
 *
 *     Created  11/17/2011 08:20:10 PM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#ifndef  __ACINDEX_H__
#define  __ACINDEX_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <map>
#include <list>
#include <queue>
#include "util.h"
#include "config.h"
#include "character.h"

typedef struct __range {
    uint32_t begin;
    uint32_t end;
    __range(uint32_t b, uint32_t e): begin(b), end(e) { }
} range_st, *range_t;
typedef std::list<range_st>  range_list;
typedef std::list<range_st>::iterator  range_list_it;

typedef  const char *                               phrase_t;
typedef  std::list<uint32_t>                        offset_list;
typedef  std::list<uint32_t>::iterator              offset_list_it;
typedef  std::map<phrase_t, offset_list >           phrase_list;
typedef  std::map<phrase_t, offset_list >::iterator phrase_list_it;

#define     MAX_CHILD_LEN       256
class acidx
{
private: //private type
#define     PHRASE_PART     1
#define     PHRASE_FULL     2
    typedef struct __trie_tree_node {
        uint8_t value;
        uint8_t flag;
        uint8_t *word;
        struct __trie_tree_node *fail;
        struct __trie_tree_node *children[MAX_CHILD_LEN];

        __trie_tree_node(): value(0), flag(PHRASE_PART), word(NULL), fail(NULL){
            memset(children, 0, MAX_CHILD_LEN*sizeof(struct __trie_tree_node *));
        }

        int set_word(char* str, int len) {
            if (word) {   free(word);   word = NULL; }
            word = (uint8_t*)calloc(len + 1, 1);
            if (!word) {
                return -1;
            }
            memcpy(word, str, len);
            /*strcpy(word, str);*/
            return 0;
        }

        ~__trie_tree_node() {
            for (int i=0 ; i < MAX_CHILD_LEN ; i++) {
                delete children[i];
            }
            delete word;
        }
    } trie_node_st, *trie_node_t;

public:
    acidx(const char* dicts = NULL);
    ~acidx();

    /**
     * @brief  add dict file, can be called many times, suggest a few times
     */
    int set_dict(const char* dict_path);

    /**
     * @brief  only test if one phrase contain in dictionary
     */
    bool query_phrase(const char* phrase);

    /**
     * @brief  feed article text to ac-machine, get phrase and list list
     *         phrase list: contain phrase in dictionary, and phrase offets in article
     *         range list: contain ranges include phrases in dictionary
     * @param  txt  feeded article
     * @param  phrases  phrases in dictionary and offets in article
     * @param  ranges   ranges contain phrases merged
     * @return  0 => success, other => error code
     */
    int query_phrases(const char* txt, phrase_list &phrases, range_list &ranges);

    /**
     * @brief  interface used to build ac machine and fail path
     */
    int init();

private:
    int build_ac_trie(const char* dict_path);
    int build_fail_path();

private: //private function

private: //private member
    fcls_t  dict_list;
    trie_node_t root;
    bool inited;
}; /* --  end of class  -- */


#endif  /*__ACINDEX_H__*/

