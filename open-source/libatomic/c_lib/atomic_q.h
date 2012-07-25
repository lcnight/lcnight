/**

ATOMIC_QUEUE
------------
An Lock-Free Queue for Multi-threads. It Optimized for fast put and get. All
operate has an visible end.

 - Auther : d13 <dainan@staff.sina.com.cn>
 - Date   : 2009/05/19
 
 */
 

#ifndef _ATOMIC_Q_H
#define _ATOMIC_Q_H

#include "atomic.h"

struct node {
    struct node * next;
    void * content;
};

typedef struct node NODE;

struct queue {
    NODE * head ;
    NODE * tail ;
    
    NODE * realhead ;
    NODE * atfield ;
    NODE * fasttrack ;
    
    NODE * first ;
    
    atomic counter ;
    atomic clean ;
    atomic read_lock ;
};

typedef struct queue QUEUE;



NODE * NODE_new();
void NODE_delete( NODE * N );


QUEUE * Queue_new();
void Queue_delete( QUEUE * Q );

int  Queue_IsEmpty( QUEUE * Q );
long Queue_Length( QUEUE * Q );


void    AQSput( QUEUE * Q , void * v );
void  * AQSget( QUEUE * Q );
void  * AQSget_nowait( QUEUE * Q );

void    AQMput( QUEUE * Q , void * v );
void  * AQMget( QUEUE * Q );
void  * AQMget_nowait( QUEUE * Q );
void  * AQM2get( QUEUE * Q );
void  * AQM2get_nowait( QUEUE * Q );

#endif
