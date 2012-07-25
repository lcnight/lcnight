/**

ATOMIC_QUEUE
------------
An Lock-Free Queue for Multi-threads. It Optimized for fast put and get. All
operate has an visible end.

 - Auther : d13 <dainan@staff.sina.com.cn>
 - Date   : 2009/05/19
 
 */
 

#include "atomic_q.h"

#include <stdlib.h>

#include <sched.h>
#define AtomicQ_YIELD sched_yield()
#include <unistd.h>
#define AtomicQ_SLEEP usleep(10000)

NODE * NODE_new()
{
    NODE * N = (NODE *) malloc(sizeof(NODE));
    
    N->next = NULL;
    N->content = NULL;

    return N;
}

void NODE_delete( NODE * N )
{
    free(N);
    
    return;
}

QUEUE * Queue_new()
{
    QUEUE * Q = (QUEUE *) malloc(sizeof(QUEUE));
    
    NODE * N = NODE_new();
    
    Q->realhead = N;
    Q->atfield = N;
    
    Q->head = N;
    Q->tail = N;
    Q->fasttrack = N;
    
    Q->clean = 1;
    
    Q->first = N;
    
#ifdef ATOMICQ_COUNTER_ON
    Q->counter = 0;
#else
    Q->counter = -1;
#endif
    
    return Q;
}

void Queue_delete( QUEUE * Q )
{
    NODE * N = (Q->fasttrack == Q->first)?(Q->head):(Q->realhead);
    NODE * P;
    
    while ( N == NULL )
    {
        P = N;
        N = N->next;
        NODE_delete( P );
    }
    
    free(Q);
    
    return;
}

int Queue_IsEmpty( QUEUE * Q )
{    
    return ( (Q->fasttrack == Q->first)?(Q->head):(Q->fasttrack) ) == Q->tail ;
}

long Queue_Length( QUEUE * Q )
{
    return Q->counter;
}

void AQSput( QUEUE * Q , void * v )
{
#ifdef ATOMICQ_COUNTER_ON
    atomic_inc( &(Q->counter) );
#endif
    
    NODE * P = Q->tail;
        
    Q->tail = NODE_new();
    Q->tail->content = v;
    
    P->next = Q->tail;
    
    return;
}

void * AQSget( QUEUE * Q )
{
    NODE * N = NULL;
    void * c = NULL;
    
    if ( Q->head->next == NULL )
    {
        AtomicQ_YIELD;
    }
    while ( Q->head->next == NULL )
    {
        AtomicQ_SLEEP;
    }
    
    c = Q->head->next->content;
    
    N = Q->head;
    Q->head = N->next;
    
    NODE_delete(N);
    
#ifdef ATOMICQ_COUNTER_ON
    atomic_dec( &(Q->counter) );
#endif
    
    return c;
}

void * AQSget_nowait( QUEUE * Q )
{
    NODE * N = NULL;
    void * c = NULL;
    
    if ( Q->head->next == NULL )
    {
        return NULL;
    }
    
    c = Q->head->next->content;
    
    N = Q->head;
    Q->head = N->next;
    
    NODE_delete(N);
    
#ifdef ATOMICQ_COUNTER_ON
    atomic_dec( &(Q->counter) );
#endif
    
    return c;
}

void AQMput( QUEUE * Q , void * v )
{
#ifdef ATOMICQ_COUNTER_ON
    atomic_inc( &(Q->counter) );
#endif
    
    NODE * N = NODE_new();
    NODE * P = N;
    
    N->content = v;
    
    P = (NODE *)atomic_xchg( (atomic)P , (atomic *)(&(Q->tail)) );
    
    P->next = N;
    
    return;
}

static void * AQMget_real( QUEUE * Q , NODE** n )
{
    NODE * N = NULL;
    void * c = NULL;
    
    N = Q->fasttrack;
    
    if ( unlikely( N->next == NULL ) )
    {
        return c;
    }
    
    c = (void *)atomic_xchg( (atomic)c, (atomic *)( &(N->next->content) ) );
    
    while ( c == NULL )
    {
        N = N->next;
        
        if ( unlikely( N->next == NULL ) )
            break;
        
        c = (void *)atomic_xchg( (atomic)c, (atomic *)( &(N->next->content) ) );
    }
    
    *n = N;
    
    if ( likely( N!=NULL ) )
    {
        N = (void *)atomic_xchg( (atomic)N, (atomic *)( &(Q->fasttrack) ) );
    }
    
#ifdef ATOMICQ_COUNTER_ON
    if ( c != NULL )
    {
        atomic_dec( &(Q->counter) );
    }
#endif
    
    return c;
}

static void AQMget_cleaner( QUEUE * Q , NODE * N )
{
    atomic hF = 0;
    NODE * P = Q->realhead;
    
    while ( P != NULL && P->content == NULL )
    {
        if ( unlikely( P == Q->head ) )
        {
            hF = 1;
        }
        
        if ( unlikely( P == N ) )
        {
            break;
        }
        
        P = P->next;
    }
    
    if ( P == N && hF == 1 )
    {
        N = (NODE *)atomic_xchg( (atomic)N, (atomic *)( &(Q->head) ) );
        
        while( Q->realhead != Q->atfield )
        {
            P = Q->realhead->next;
            free(Q->realhead);
            Q->realhead = P;
        }
        
        Q->atfield = N;
    }
    
    return ;
}

void * AQMget_nowait( QUEUE * Q )
{
    NODE * N = NULL;
    void * c = NULL;
    
    atomic_inc( &(Q->read_lock) );
    
    c = AQMget_real( Q, &N );
    
    if ( atomic_dec_and_test( &(Q->read_lock) ) )
    {
        if ( atomic_xchg( 0, &(Q->clean) ) )
        {
            AQMget_cleaner( Q, (N!=NULL)?N:Q->head );
            
            atomic_xchg( 1, &(Q->clean) );
        }
    }
    
    return c;
}

void * AQMget( QUEUE * Q )
{
    void * c = AQMget_nowait( Q );
    
    if ( c != NULL )
        return c;
    
    AtomicQ_YIELD;
    
    c = AQMget_nowait( Q );
    
    while( c == NULL ) 
    {
        AtomicQ_SLEEP;
        c = AQMget_nowait( Q );
    }
    
    return c;
}

void * AQM2get_nowait( QUEUE * Q )
{
    NODE * N = NULL;
    void * c = NULL;
    
    N = (NODE *)atomic_xchg( (atomic)NULL, (atomic *)( &(Q->head) ) );
    
    while( N == NULL )
    {
        AtomicQ_YIELD;
        N = (NODE *)atomic_xchg( (atomic)NULL, (atomic *)( &(Q->head) ) );
    }
    
    if ( N->next != NULL )
    {
        NODE * N0;
        
        c = N->next->content;
        
        N0 = N;
        N = N->next;
        
        NODE_delete(N0);
    }
    
    N = (NODE *)atomic_xchg( (atomic)N, (atomic *)( &(Q->head) ) );
    
#ifdef ATOMICQ_COUNTER_ON
    if ( c != NULL )
    {
        atomic_dec( &(Q->counter) );
    }
#endif
    
    return c;
}

void * AQM2get( QUEUE * Q )
{
    void * c = AQM2get_nowait(Q);
    
    while( c == NULL )
    {
        AtomicQ_SLEEP;
        c = AQM2get_nowait(Q);
    }
    
    return c;
}
