/**
 *      @brief  character set process
 *
 *     Created  11/21/2011 10:16:28 AM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#include <stdlib.h>
#include "character.h"

/* Unicode to UTF-8 mapping:
Unicode Range        : UTF-8
---------------------:------------------------------------------------------
U00000000 - U0000007F: 0xxxxxxx
U00000080 - U000007FF: 110xxxxx 10xxxxxx
U00000800 - U0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
U00010000 - U001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
U00200000 - U03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
U04000000 - U7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

Para: 0x00000000 <= unicode <= 0x7FFFFFFF
*/
int Unicode2Utf8(unsigned long unicode, unsigned char UTF8[])
{
    if(UTF8 == NULL) return -1;

    if ( 0x00000000 <= unicode && unicode <= 0x0000007F )
    {
        /*UTF8[MAX] = 1;*/
        UTF8[0]   = (unsigned char)(unicode);
        return 1;
    }

    if ( 0x00000080 <= unicode && unicode <= 0x000007FF )
    {
        /*UTF8[MAX] = 2;*/
        UTF8[0]   = (unsigned char)(0xC0 | (unicode>>6));
        UTF8[1]   = (unsigned char)(0x80 | (unicode & 0x3F));
        return 2;
    }

    if ( 0x00000800 <= unicode && unicode <= 0x0000FFFF )
    {
        /*UTF8[MAX] = 3;*/
        UTF8[0]   = (unsigned char)(0xE0 | (unicode>>12));
        UTF8[1]   = (unsigned char)(0x80 | (unicode>>6 & 0x3F));
        UTF8[2]   = (unsigned char)(0x80 | (unicode & 0x3F));
        return 3;
    }

    if ( 0x00010000 <= unicode && unicode <= 0x001FFFFF )
    {
        /*UTF8[MAX] = 4;*/
        UTF8[0]   = (unsigned char)(0xF0 | (unicode>>18));
        UTF8[1]   = (unsigned char)(0x80 | (unicode>>12 & 0x3F));
        UTF8[2]   = (unsigned char)(0x80 | (unicode>>6 & 0x3F));
        UTF8[3]   = (unsigned char)(0x80 | (unicode & 0x3F));
        return 4;
    }

    if ( 0x00200000 <= unicode && unicode <= 0x03FFFFFF )
    {
        /*UTF8[MAX] = 5;*/
        UTF8[0]   = (unsigned char)(0xF8 | (unicode>>24));
        UTF8[1]   = (unsigned char)(0x80 | (unicode>>18 & 0x3F));
        UTF8[2]   = (unsigned char)(0x80 | (unicode>>12 & 0x3F));
        UTF8[3]   = (unsigned char)(0x80 | (unicode>>6 & 0x3F));
        UTF8[4]   = (unsigned char)(0x80 | (unicode & 0x3F));
        return 5;
    }

    if ( 0x04000000 <= unicode && unicode <= 0x7FFFFFFF )
    {
        /*UTF8[MAX] = 6;*/
        UTF8[0]   = (unsigned char)(0xFC | (unicode>>30));
        UTF8[1]   = (unsigned char)(0x80 | (unicode>>24 & 0x3F));
        UTF8[2]   = (unsigned char)(0x80 | (unicode>>18 & 0x3F));
        UTF8[3]   = (unsigned char)(0x80 | (unicode>>12 & 0x3F));
        UTF8[4]   = (unsigned char)(0x80 | (unicode>>6 & 0x3F));
        UTF8[5]   = (unsigned char)(0x80 | (unicode & 0x3F));
        return 6;
    }

    return -1; /* impossible */
}

/* Unicode to UTF-8 mapping:
Unicode Range        : UTF-8
---------------------:------------------------------------------------------
U00000000 - U0000007F: 0xxxxxxx
U00000080 - U000007FF: 110xxxxx 10xxxxxx
U00000800 - U0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
U00010000 - U001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
U00200000 - U03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
U04000000 - U7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

Para: 0x00000000 <= unicode <= 0x7FFFFFFF
*/
unsigned long Utf82Unicode(unsigned char UTF8[])
{
    unsigned long unicode = 0;

    if ( 0x00 == (unsigned char)(UTF8[0])>>7 )
    {
        /*UTF8[MAX] = 1;*/
        unicode   = UTF8[0];
        return unicode;
    }

    if ( 0x06 == (unsigned char)(UTF8[0])>>5 )
    {
        /*UTF8[MAX] = 2;*/
        unicode   = (UTF8[0]&0x1F) << 6;
        unicode  |= (UTF8[1]&0x3F);
        return unicode;
    }

    if ( 0x0E == (unsigned char)(UTF8[0])>>4 )
    {
        /*UTF8[MAX] = 3;*/
        unicode   = (UTF8[0]&0x0F) << 12;
        unicode  |= (UTF8[1]&0x3F) << 6;
        unicode  |= (UTF8[2]&0x3F);
        return unicode;
    }

    if ( 0x1E == (unsigned char)(UTF8[0])>>3 )
    {
        /*UTF8[MAX] = 4;*/
        unicode   = (UTF8[0]&0x07) << 18;
        unicode  |= (UTF8[1]&0x3F) << 12;
        unicode  |= (UTF8[2]&0x3F) << 6;
        unicode  |= (UTF8[3]&0x3F);
        return unicode;
    }

    if ( 0x3E == (unsigned char)(UTF8[0])>>2 )
    {
        /*UTF8[MAX] = 5;*/
        unicode   = (UTF8[0]&0x03) << 24;
        unicode  |= (UTF8[1]&0x3F) << 18;
        unicode  |= (UTF8[2]&0x3F) << 12;
        unicode  |= (UTF8[3]&0x3F) << 6;
        unicode  |= (UTF8[4]&0x3F);
        return unicode;
    }

    if ( 0x7E == (unsigned char)(UTF8[0])>>1 )
    {
        /*UTF8[MAX] = 6;*/
        unicode   = (UTF8[0]&0x01) << 30;
        unicode  |= (UTF8[1]&0x3F) << 24;
        unicode  |= (UTF8[2]&0x3F) << 18;
        unicode  |= (UTF8[3]&0x3F) << 12;
        unicode  |= (UTF8[4]&0x3F) << 6;
        unicode  |= (UTF8[5]&0x3F);
        return unicode;
    }

    return 0; /* Impossible */
}

static unsigned char _mblen_table_utf8[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

unsigned char GET_CLEN(int c) {
    return _mblen_table_utf8[(c)];
}

bool is_sep(int ch) {
    if ((ch >= 0x20 && ch <= 0x2F ) || (ch >= 0x3A && ch <= 0x40) 
            || (ch >= 0x5B && ch <= 0x60) || (ch >= 0x7B && ch <= 0x7F)) { 
        return true;
    } else {
        return false;
    }
}

#if 0

 /* gbk(gb2312|big5): 0x81 ~ 0xfe */
static unsigned char _mblen_table_gbk[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1
};

struct mblen_tab
{
    const char *name;
    unsigned char *table;
};

static struct mblen_tab mblen_tab_list[] =
{
    {   "utf8",     _mblen_table_utf8   },
    {   "utf-8",    _mblen_table_utf8   },
    {   "gb2312",   _mblen_table_gbk    },
    {   "gbk",      _mblen_table_gbk    },
    {   "big5",     _mblen_table_gbk    },
    {   "big-5",    _mblen_table_gbk    },
    {   NULL,       NULL }
};

#endif
