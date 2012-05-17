/**
 *      @brief  check index in redis
 *
 *     Created  11/24/2011 05:21:18 PM
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <errno.h>
#include <hiredis/hiredis.h>
#include "util.h"

typedef struct keyword_index_value {
    uint32_t did;
    uint32_t flag;          // denote reverse index relate document title/content
    uint32_t offset[0];
}__attribute__((packed)) keyword_idx_st, *keyword_idx_t;

const char *DOC_TYPE[] = {
    "", "title", "content"
};

void show_reply(redisContext *c, redisReply *reply);
int main(int argc, char *argv[])
{
    int opt = 0;
    char host[256] ={"localhost"};
    int port = 6379;

    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt)
        {
        case 'h' :
            strcpy(host, optarg);
            break;
        case 'p' :
            port = atoi(optarg);
            break;
        default :
            fprintf(stderr, "Usage: %s [-h host] [-p port] dbid\n",
                    argv[0]);
            exit(EXIT_FAILURE);
            break;
        }  /* end of switch */
    } /*-- end of while --*/

    int dbid = 0;
    if (argv[optind]) {
        dbid = atoi(argv[optind]);
    }
    if (dbid<0 || dbid>15) {
        PRINT("dbid must [0, 15]");
        return -1;
    }
    PRINT("set to redis dbid %d\n", dbid);
    redisContext * c = redisConnect(host, port);
    redisReply *reply = (redisReply *)redisCommand(c, "select %d", dbid);
    freeReplyObject(reply);
#define     DELIMITER       " \t\n\r"
#define     PROMPT          "idx-cli> "
    char *phrase_buf = (char*)calloc(4096, 1);
    while (true) {
        printf(PROMPT);
        char *beg = fgets(phrase_buf, 4093, stdin);
        beg += strspn(phrase_buf, DELIMITER);
        strtok(beg, DELIMITER);
        if (!strcasecmp(beg, "quit")) {
            break;
        }
        //reply = (redisReply *)redisCommand(c, "zrange %s 0 -1 WITHSCORES", phrase_buf);
        reply = (redisReply *)redisCommand(c, "zrange %s 0 -1", phrase_buf);
        if (!reply) {
            PRINT("null reply, %d %s", c->err, c->errstr);
        } else {
            show_reply(c, reply);
            freeReplyObject(reply);
        } /*-- end of while --*/
    }

    return 0;
}/* -- end of main  -- */

static char* idx_buf[4096] = {0};
static keyword_idx_t idx_item = NULL;
void show_reply(redisContext *c, redisReply *reply) 
{

    if (!reply) {
        return;
    }
    switch (reply->type)
    {
    case REDIS_REPLY_STATUS :
        PRINT("STATUS %s", reply->str);
        break;
    case REDIS_REPLY_ERROR :
        PRINT("ERROR %s", c->errstr);
        break;
    case REDIS_REPLY_INTEGER :
        PRINT("INTEGER %lld", reply->integer);
        break;
    case REDIS_REPLY_STRING :
        {
            PRINT("STRING %%%d", reply->len);
            memcpy(idx_buf, reply->str, reply->len);
            idx_item = (keyword_idx_t)idx_buf;
            PRINT("\tdid: %u", idx_item->did);
            PRINT("\ttype: %s", DOC_TYPE[idx_item->flag]);
            printf("\toffset: ");
            for (int cnt = 0, i = 8 ; i < reply->len ; i += 4, ++cnt) {
                printf("%u, ", idx_item->offset[cnt]);
            } /*-- end of for --*/
            PRINT("");
        }
        break;
    case REDIS_REPLY_NIL :
        PRINT("(NIL)");
        break;
    case REDIS_REPLY_ARRAY :
        {
            int num = (int)reply->elements;
            PRINT("ARRAY *%u:", num);
            for (int i=0 ; i < num; ++i) {
                show_reply(c, reply->element[i]);
            } /*-- end of for --*/
        }
        break;
    default :
        PRINT("un handle type %d", reply->type);
        break;
    }  /* end of switch */
}
