/**
 *      @brief  header file
 *
 *     Created  11/21/2011 10:16:58 AM 
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */

#ifndef  __CHARACTER_PROCESS_H__
#define  __CHARACTER_PROCESS_H__


int Unicode2Utf8(unsigned long unicode, unsigned char UTF8[]);
unsigned long Utf82Unicode(unsigned char UTF8[]);

/*#define     GET_CLEN(c)     _mblen_table_utf8[(c)]*/
unsigned char GET_CLEN(int c); 

//check acsii character is separate or not?
bool is_sep(int ch);

#endif  /*__CHARACTER_PROCESS_H__*/
