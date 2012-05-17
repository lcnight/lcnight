/**
 *      @brief  utility functions and tools for indexer
 *
 *     Created  11/17/2011 08:11:07 PM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#include <unistd.h>
#include "util.h"

bool path_exist(const char*path)
{
    if (path == NULL) return false;

    if (!access(path, F_OK|R_OK)) {
        return true;
    } else {
        return false;
    }
}

void pack(uint8_t **dst, void *src, int len)
{
    if (!dst || !(*dst) || !src || !len) {
        return;
    }
    memcpy(*dst, src, len);
    *((uint8_t**)dst) += len;
}

uint32_t bytes2char_num(const char *txt, uint32_t offset)
{
    if (!txt) {
        return 0;
    }
    uint8_t *beg = (uint8_t *)txt;
    uint32_t char_num = 0;
    uint32_t b_off = 0;
    while(b_off < offset && (*(beg + b_off))) {
        b_off += GET_CLEN(*(beg + b_off));
        ++char_num;
    }
    return char_num;
}
