/**
 *      @brief  header file for utility
 *
 *     Created  11/17/2011 08:11:25 PM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#ifndef  __INDEXER_UTIL_H__
#define  __INDEXER_UTIL_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "fcls.h"
#include "character.h"

#define     PRINT(fmt, args...) printf(fmt"\n", ##args)

#define     DEBUG_MODE  1
#ifdef  DEBUG_MODE
#define     DEBUG_SHOW(fmt, args...)        DEBUG_LOG(fmt, ##args)
#define     KNOTI_SHOW(key, fmt, args...)        KNOTI_LOG(key, fmt, ##args)
#else //no debug
#define     DEBUG_SHOW(fmt, args...)
#define     KNOTI_SHOW(key, fmt, args...)
#endif

bool path_exist(const char*path);

void pack(uint8_t **dst, void *src, int len);

uint32_t bytes2char_num(const char *txt, uint32_t offset);

#endif  /*__INDEXER_UTIL_H__*/

