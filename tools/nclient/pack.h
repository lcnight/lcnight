/**
 * =====================================================================================
 *       @file  pack.h
 *      @brief  
 *
 *   @internal
 *     Created  10/17/2011 02:00:05 PM 
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#ifndef  __PACK_H__
#define  __PACK_H__

#include <stdlib.h>
#include <inttypes.h>

#define     u16_size_bytes      2
#define     u32_size_bytes      4
void put_u8(uint8_t **ptr, uint8_t value);
void put_u16(uint8_t **ptr, uint16_t value, bool align = false);
void put_u32(uint8_t **ptr, uint32_t value, bool align = false);

void get_u8(uint8_t **ptr, uint8_t value);
void get_u16(uint8_t **ptr, uint16_t value, bool align = false);
void get_u32(uint8_t **ptr, uint32_t value, bool align = false);


#endif  /*__PACK_H__*/
