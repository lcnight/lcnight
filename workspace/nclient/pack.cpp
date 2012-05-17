/**
 * =====================================================================================
 *       @file  pack.cpp
 *      @brief  
 *
 *   @internal
 *     Created  10/17/2011 01:58:33 PM 
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <string.h>
#include <arpa/inet.h>

#include "pack.h"

void put_u8(uint8_t **ptr, uint8_t value)
{
    **ptr = value;
    *ptr += 1;
}

void put_u16(uint8_t **ptr, uint16_t value, bool align)
{
    if (align) {
        uint16_t tmp = htons(value);
        memcpy(*ptr, &tmp, u16_size_bytes);
    } else {
        memcpy(*ptr, &value, u16_size_bytes);
    }
    *ptr += u16_size_bytes;
}

void put_u32(uint8_t **ptr, uint32_t value, bool align)
{
    if (align) {
        uint32_t tmp = htons(value);
        memcpy(*ptr, &tmp, u32_size_bytes);
    } else {
        memcpy(*ptr, &value, u32_size_bytes);
    }
    *ptr += u32_size_bytes;
}

void get_u8(uint8_t **ptr, uint8_t *value)
{
    *value = **ptr;
    *ptr += 1; 
}

void get_u16(uint8_t **ptr, uint16_t *value, bool align = false)
{
    if (align) {
        *value = ntohs(*(uint16_t*)*ptr);
    } else {
        *value = *(uint16_t*)*ptr;
    }
    *ptr += u16_size_bytes;
}

void get_u32(uint8_t **ptr, uint32_t *value, bool align = false)
{
    if (align) {
        *value = ntohl(*(uint32_t*)*ptr);
    } else {
        *value = *(uint32_t*)*ptr;
    }
    *ptr += u32_size_bytes;
}
