/**
 *      @brief  fixed capcity linked string list implication
 *
 *     Created  11/18/2011 11:49:56 AM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#include <stdlib.h>
#include <string.h>
#include "fcls.h"

#define CHK_SLIST(slist) do { \
    if (!(slist)) {\
        return NULL;\
    } \
} while(0)

fcls_t fcls_new()
{
    fcls_t new_one = (fcls_t)calloc(sizeof(fcls_st), 1);
    new_one->capcity = CAPCITY_SIZE;
    new_one->count = 0;
    new_one->freed = false;
    new_one->next = NULL;
    new_one->it = &new_one->list[0];
    return new_one;
}

/* add string reference, and return head fcls_t of slist */
fcls_t fcls_add_ref(fcls_t slist, const char *str)
{
    CHK_SLIST(slist);

    fcls_t p_list = slist;
    while(p_list->next) p_list = p_list->next;

    if (p_list->count < p_list->capcity) {
        p_list->list[slist->count ++] = (char*)str;
    } else {
        fcls_t tmp = fcls_new();
        fcls_add_ref(tmp, str);
        p_list->next = tmp;
    }
    return slist;
}


/* clear strings and status flags for fcls_t */
fcls_t fcls_clear(fcls_t slist)
{
    CHK_SLIST(slist);

    fcls_t p_list = slist;
    do {
        p_list->count = 0;
        p_list->capcity = CAPCITY_SIZE;
        p_list->freed = false;
        memset(p_list->list, 0 , sizeof(p_list->list));
        p_list->it = &p_list->list[0];
        p_list = p_list->next;
    } while(p_list);

    return slist;
}

/* add string value and copy instance, and return head fcls_t of slist */
fcls_t fcls_add_cpy(fcls_t slist, const char *str)
{
    CHK_SLIST(slist);

    char *ptr = (char*)calloc(strlen(str) + 1, 1);
    strcpy(ptr, str);
    return fcls_add_ref(slist, ptr);
}

/* free strings contained in fcls, the slist can be reuse */
fcls_t fcls_free_strings(fcls_t slist)
{
    CHK_SLIST(slist);

    fcls_t p_list = slist;
    do {
        p_list->count = 0;
        p_list->capcity = CAPCITY_SIZE;
        p_list->freed = false;
        for (int i=0 ; i <  p_list->capcity; i++) {
            free(p_list->list[i]);
            p_list->list[i] = NULL;
        }
        p_list->it = &p_list->list[0];
        p_list = p_list->next;
    } while(p_list);

    return slist;
}

/* free strings contained and fcls itself */
void fcls_free_all(fcls_t *pslist)
{
    if (!pslist || *pslist) { return; }

    fcls_t p_list = *pslist;
    fcls_free_strings(p_list);
    free(p_list);
    *pslist = NULL;
}

/* define iterator operation interface for fcls_st */
void fcls_rewind(fcls_t slist)
{
    if (!slist) { return; }

    slist->it = &slist->list[0];
}

fcls_s fcls_next(fcls_t *pslist)
{
    CHK_SLIST(pslist);
    CHK_SLIST(*pslist);

    fcls_t p_list = *pslist;
    if (p_list->it - &p_list->list[0] < p_list->count) {
        return *(p_list->it ++);
    } else {
        if (p_list->count < p_list->capcity) {
            return NULL;
        } else {
            *pslist = p_list->next;
            fcls_rewind(*pslist);
            return *((*pslist)->it ++);
        }
    }
}
