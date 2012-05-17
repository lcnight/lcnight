/**
 *      @brief  header file for fls string
 *
 *     Created  11/18/2011 11:50:44 AM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#ifndef  __INDEXER_FLS_H__
#define  __INDEXER_FLS_H__

#define     CAPCITY_SIZE     100
typedef char *fcls_s;
typedef char **fcls_it;
typedef struct fixed_cap_linked_string {
    char *list[CAPCITY_SIZE];
    int capcity;
    int count;
    bool freed;
    fcls_it it;
    struct fixed_cap_linked_string *next;
} fcls_st, * fcls_t;
/* define iterator for fcls_st */

fcls_t fcls_new();

/* add string reference, and return head fcls_t of slist */
fcls_t fcls_add_ref(fcls_t slist, const char *str);

/* clear strings and status flags for fcls_t */
fcls_t fcls_clear(fcls_t slist);

/* add string value and copy instance, and return head fcls_t of slist */
fcls_t fcls_add_cpy(fcls_t slist, const char *str);

/* free strings contained in fcls, the slist can be reuse */
fcls_t fcls_free_strings(fcls_t slist);

/* free strings contained and fcls itself */
void fcls_free_all(fcls_t *slist);

/* define iterator operation interface for fcls_st */
void fcls_rewind(fcls_t slist);
/* psavelist save fcls list context information */
fcls_s fcls_next(fcls_t *psavelist);

#endif  /*__INDEXER_FLS_H__*/

