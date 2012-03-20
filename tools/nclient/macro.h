/**
 * =====================================================================================
 *       @file  macro.h
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#ifndef  __MACRO_H__
#define  __MACRO_H__

#define     PRINT(fmt, args...)  printf(fmt"\n", ##args)
#define     D_LOG(fmt, args...)  printf("FL(%s) LN(%d):"fmt"\n", __FILE__, __LINE__ ##args)

#define     CALL_API(state, result)  { \
    if((state) != result) PRINT(" errno %d, str: %m", errno); \
    else PRINT(" call API "#state" success");\
}

#define     DEBUG
#ifdef DEBUG
    #define     DEBUG_SQL(sql)  PRINT("[DEBUG] SQL: %s", sql)
#else
    #define     DEBUG_SQL(sql)
#endif

#endif  /*__MACRO_H__*/
