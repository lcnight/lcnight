/**
 *      @brief  implimentation of AC machine and article to phrases and ranges
 *
 *     Created  11/17/2011 08:18:31 PM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#include "acindex.h"


acidx::acidx(const char* dicts): dict_list(NULL), root(NULL), inited(false)
{
    dict_list = fcls_new();
    root = new trie_node_st();
    if (!dicts) {
        return;
    }

    char *buf = (char*)malloc(COMMON_BUFSZ);
    strcpy(buf, dicts);
    char *saveptr = NULL;
    char *ptr = strtok_r(buf, ",", &saveptr);
    while (ptr) {
        while(*ptr == ' ' || *ptr == '\t') ptr++;
        fcls_add_cpy(dict_list, ptr);
        ptr = strtok_r(NULL, ",", &saveptr);
    } /*-- end of while --*/

    // do check,  test the dicts file existed?
    fcls_t it_fcls = dict_list;
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
    free(buf);
}

acidx::~acidx()
{
    fcls_free_all(&dict_list);
    if (root) {
        delete root;
        root = NULL;
    }
}

int acidx::set_dict(const char* dict_path)
{
    if (dict_path) {
        fcls_add_cpy(this->dict_list, dict_path);
    }
    return 0;
}

int acidx::init()
{
    if (inited) {
        return 0;
    }
    fcls_t it_dicts = dict_list;
    fcls_rewind(it_dicts);
    char *dict = fcls_next(&it_dicts);
    while (dict) {
        DEBUG_LOG("load dict: %s", dict);
        if (build_ac_trie(dict)) {
            return -1;
        }
        dict = fcls_next(&it_dicts);
    }

    if (build_fail_path()) {
        return -1;
    } else {
        inited = true;
        return 0;
    }
}

bool acidx::query_phrase(const char* phrase)
{
    if ((!inited && init()) || !root) {
        return false;
    }

    trie_node_t p_node = root;
    uint8_t *ptr = (uint8_t*)phrase;
    while (*ptr) {
        if (p_node->children[*ptr]) {
            p_node = p_node->children[*ptr];
            ptr ++;
        } else {
            break;
        }
    } /*-- end of while --*/

    if (!(*ptr) && (p_node->flag & PHRASE_FULL)) {
        return true;
    } else {
        return false;
    }
}

void pick_phrase_range(uint8_t *txt, uint8_t *ptr, const uint8_t *phrase, 
        phrase_list &phrases, range_list &ranges)
{
    uint32_t begin = ptr - txt - strlen((const char*)phrase) + 1;
    uint32_t end = ptr - txt + 1;

    phrase_list_it p_it = phrases.find((const char*)phrase);
    if (p_it == phrases.end()) {
        offset_list tmp;
        tmp.push_back(begin);
        phrases.insert(std::pair<phrase_t, offset_list>((phrase_t)phrase, tmp));
    } else {
        p_it->second.push_back(begin);
    }

    if (ranges.size() != 0) {
        range_st *r_it = &ranges.back();
        if (r_it->end > begin) {
            r_it->end = end;
        } else {
            ranges.push_back(range_st(begin, end));
        }
    } else {
        ranges.push_back(range_st(begin, end));
    }
}

int acidx::query_phrases(const char* txt, phrase_list &phrases, range_list &ranges)
{
    if ((!inited && init()) || !root) {
        return -1;
    }

    trie_node_t p_node = root;
    uint8_t *txt_beg = (uint8_t*) txt;
    uint8_t *ptr = (uint8_t*)txt;
    int clen = 0;
    while (*ptr) {
        clen = GET_CLEN(*ptr);
        trie_node_t temp = p_node; //pointer to trie node of last byte of last character(chinese/english)
        int i=0 ;
        while (i < clen) { // check one character (english/chinese)
            if (p_node->children[*ptr]) {
                p_node = p_node->children[*ptr];
                if (p_node->flag & PHRASE_FULL) {
                    pick_phrase_range(txt_beg, ptr, p_node->word, phrases, ranges);
                    DEBUG_SHOW("get %s", p_node->word);
                }
                ++ptr;
            } else {
                break;
            }
            i++;
        } /*-- end of while --*/
        if (i < clen) {  // half chinese char || not in first level english char, draw back
            if (temp == root) {
                ptr += clen - i;
            } else {
                ptr -= i;
            }
            while (temp->fail) {
                bool full_next_char = true;
                trie_node_t full_char_node = temp->fail;
                int full_len = GET_CLEN(*ptr);
                for (int j=0; j < full_len ; j++) {  //full character match
                    if (!full_char_node->children[*(ptr + j)]) {
                        full_next_char = false;
                        break;
                    }
                    full_char_node = full_char_node->children[*(ptr + j)];
                } /*-- end of for --*/
                if (!full_next_char) {
                    temp = temp->fail;
                    if (temp->flag & PHRASE_FULL) {
                        pick_phrase_range(txt_beg, ptr - 1, temp->word, phrases, ranges);
                        DEBUG_SHOW("get|%s", temp->word);
                    }
                } else {
                    break;
                }
            }
            p_node = (temp == root) ? root : temp->fail;
        } 
        else {   // full char match, to next char
            while (p_node != root && i == clen) {
                bool full_next_char = true;
                trie_node_t full_char_node = p_node;
                int full_len = GET_CLEN(*ptr);
                for (int j=0 ; j < full_len ; j++) {  //full character match
                    if (!full_char_node->children[*(ptr + j)]) {
                        full_next_char = false;
                        break;
                    }
                    full_char_node = full_char_node->children[*(ptr + j)];
                }
                if (!full_next_char) {
                    p_node = p_node->fail;
                    if (p_node->flag & PHRASE_FULL) {
                        pick_phrase_range(txt_beg, ptr - 1, p_node->word, phrases, ranges);
                        DEBUG_SHOW("get:%s", p_node->word);
                    }
                } else {
                    break;
                }
            }/*-- end of while --*/
        } /*-- end of if --*/
    }/*-- end of while --*/
    return 0;
}

int acidx::build_ac_trie(const char* dict_path)
{
    if (!root) {
        return -1;
    }

    FILE *fd = fopen(dict_path, "r");
    if (!fd) {
        ERROR_LOG("acidx fopen dict fail, %s", strerror(errno));
        return -1;
    }

    char *buf = (char*) malloc(COMMON_BUFSZ);
    uint8_t *beg = NULL;
    uint8_t *ptr = NULL;
    trie_node_t p_node = NULL;
    while (fgets(buf, COMMON_BUFSZ, fd)) {
        beg = (uint8_t*)(buf + strspn(buf, " \r\t\n"));
        if (*beg == '#') continue;
        char *pos = strpbrk((char*)beg, " \r\t\n");
        if (pos) *pos = 0;
        pos = (char*) beg;
        ptr = (uint8_t*)beg;

        p_node = root;
        while(*ptr) {
            if (!p_node->children[*ptr]) {
                p_node->children[*ptr] = new trie_node_st;
                p_node->children[*ptr]->value = *ptr;
            }
            p_node = p_node->children[*ptr];
            ptr ++;
        }

        p_node->flag = PHRASE_FULL; //whole word
        p_node->set_word(pos, strlen(pos));
    }
    free(buf);
    fclose(fd);
    return 0;
}

int acidx::build_fail_path()
{
    if (!root) {
        return -1;
    }
    trie_node_t p_node = NULL;
    std::queue<trie_node_t> que;
    root->fail = NULL;
    que.push(root);
    while(!que.empty()) {
        p_node = que.front();
        que.pop();
        trie_node_t temp = NULL;
        for (int i=0 ; i < MAX_CHILD_LEN ; i++) {
            if (p_node->children[i]) { /* current child node not null */
                if (p_node == root) {
                    //if(p_node->children[i])
                    p_node->children[i]->fail = root;
                } else {
                    temp = p_node->fail;
                    while (temp) {
                        if (temp->children[i]) {
                            p_node->children[i]->fail = temp->children[i];
                            break;
                        }
                        temp = temp->fail;
                    } /*-- end of while --*/
                    if (temp == NULL) {
                        p_node->children[i]->fail = root;
                    }
                }
                que.push(p_node->children[i]);
            } /*-- end of if --*/
        } /*-- end of for --*/
    } /*-- end of while --*/
    return 0;
}

