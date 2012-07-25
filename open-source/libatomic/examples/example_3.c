#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "atomic.h"
#include "atomic_q.h"

#define RTN  100
#define CL   10000

int *gVar ;
QUEUE * gq;
atomic fincnt;
atomic fincnt2;

clock_t t0,t1,t2;

void *do_put( void * s )
{
    int *p = (int *)s;
    int i;
    
    for ( i=0; i < CL; i++, p++ )
    {
        AQMput( gq, (void *)p );
    }
    
    if ( atomic_dec_and_test(&fincnt) )
    {
        t1 = clock();
    }
    
    return NULL;
}

void *do_get()
{
    int i;
    int n;
    
    for ( i=0; i < CL; i++ )
    {
        n = *( (int *)AQMget( gq ) );
        gVar[n] = 0;
    }
    
    if ( atomic_dec_and_test(&fincnt2) )
    {
        t2 = clock();
    }
    
    return NULL;
}

void *do_get2()
{
    int i;
    int n;
    
    for ( i=0; i < CL; i++ )
    {
        n = *( (int *)AQM2get( gq ) );
        gVar[n] = 0;
    }
    
    if ( atomic_dec_and_test(&fincnt2) )
    {
        t2 = clock();
    }
    
    return NULL;
}

void single_get()
{
    int *p = gVar;
    int i;
    
    for ( i=0; i < CL*RTN; i++, p++ )
    {
        gVar[*( (int *)AQSget( gq ) )] = 0;
    }
}

void single_put()
{
    int *p = gVar;
    int i;
    
    for ( i=0; i < CL*RTN; i++, p++ )
    {
        *p = i;
        AQSput( gq, (void *)p );
    }
}

void inittest ()
{
    int *p = gVar;
    int i;
    
    for ( i=0; i < CL*RTN; i++, p++ )
    {
        *p = i;
    }
    
    fincnt = RTN;
    fincnt2 = RTN;
}

void start_multi( int z ) 
{
    pthread_t ntid;
    int i, ret;
    
    if ( z == 0 )
    {
        for ( i=0; i < RTN; i++ )
        {
            ret = pthread_create(&ntid, NULL, do_put, (void *)(gVar+i*CL) );
        }
    }
    else if ( z == 1 )
    {
        for ( i=0; i < RTN; i++ )
        {
            ret = pthread_create(&ntid, NULL, do_get, NULL );
        }
    }
    else if ( z == 2 )
    {
        for ( i=0; i < RTN; i++ )
        {
            ret = pthread_create(&ntid, NULL, do_get2, NULL );
        }
    }
}

void wait_multi()
{
    while( fincnt != 0 )
    {
        usleep(10000);
    }
}

void wait_multi2()
{
    while( fincnt2 != 0 )
    {
        usleep(10000);
    }
}


int checkresult()
{
    int i, ret;
    
    printf("Checking result...");
    
    ret = 0;
    for ( i=0; i < CL*RTN; i++ )
    {
        if ( gVar[i] != 0 )
        {
            ret += 1;
        }
    }
    
    if ( ret != 0 )
    {
        printf("failed (%d)\n",ret);
    }
    else
    {
        printf("passed\n");
    }
    
    return ret;
}

int main()
{
    gVar = (int *)malloc((sizeof(int))*CL*RTN);
    
    
    inittest();
    
    
    
    gq = Queue_new();
    
    t0 = clock();
    
    printf("Starting Test 1 ...\n");
    
    start_multi(0);
    
    wait_multi();
    
    //t2 = clock();
    
    //if ( checkresult != 0 )
    //    return -1;
    
    printf("TEST 1 , Multi In.\n");
    printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    //printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    t0 = clock();
    
    printf("Starting Test 2 ...\n");
    
    start_multi(1);
    
    wait_multi2();
    
    //t1 = clock();
    
    if ( checkresult() != 0 )
        return -1;
    
    printf("TEST 2 , Multi Out.\n");
    //printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    
    inittest();
    
    
    
    
    gq = Queue_new();
    
    t0 = clock();
    
    printf("Starting Test 3 ...\n");
    
    start_multi(0);
    
    single_get();
    
    t2 = clock();
    
    wait_multi();
    
    if ( checkresult() != 0 )
        return -1;
    
    printf("TEST 3 , Multi In Single Out.\n");
    printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    
    
    inittest();
    
    
    
    
    gq = Queue_new();
    
    t0 = clock();
    
    printf("Starting Test 4 ...\n");
    
    single_put();
    
    t1 = clock();

    printf("TEST 4 , Single In .\n");
    printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    //printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    t0 = clock();
    
    printf("Starting Test 5 ...\n");
    
    start_multi(2);
    
    wait_multi2();
    
    //t1 = clock();
    
    if ( checkresult() != 0 )
        return -1;
    
    printf("TEST 5 , Multi Out 2.\n");
    //printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    
    inittest();
    
    
    
    
    gq = Queue_new();
    
    t0 = clock();
    
    printf("Starting Test 6 ...\n");
    
    start_multi(0);
    start_multi(1);
    
    wait_multi();
    wait_multi2();
    
    if ( checkresult() != 0 )
        return -1;
    
    printf("TEST 6 , Multi In Multi Out.\n");
    printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    inittest();

    
    
    gq = Queue_new();
    
    t0 = clock();
    
    printf("Starting Test 7 ...\n");
    
    start_multi(0);
    start_multi(2);
    
    wait_multi();
    wait_multi2();
    
    if ( checkresult() != 0 )
        return -1;
    
    printf("TEST 7 , Multi In Multi Out 2.\n");
    printf("         PUT USE : %f seconds\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    printf("         GET USE : %f seconds\n", (double)(t2 - t0) / CLOCKS_PER_SEC);
    
    
    
    
    return 0;
}
