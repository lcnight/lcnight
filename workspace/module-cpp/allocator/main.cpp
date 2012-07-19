/**
 * =====================================================================================
 *       @file  test.cpp
 *      @brief  main file for test
 *
 *     Created  05/29/2011 05:01:47 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, lc
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#include  <unistd.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include    "slab_allocator.h"

int main(int argc, char *argv[])
{
    slab_alloc *alloc = slab_alloc::get_instance();
    printf("pass init\n");
    char* ptr = (char*)alloc->malloc(13);
//    alloc->free(ptr);
    printf("prt:%p\n", ptr);
    ptr = (char*)alloc->malloc(73);
    printf("prt:%p\n", ptr);

    ptr = (char*)alloc->reallocate(ptr, 100);
    for(int i = 0; i<50; i++) {
        alloc->free(ptr);
        int size = rand() & MAX_ITEM_SIZE/100;
        printf("rand() : %d\n", size);
        ptr = (char*)alloc->malloc(size);
    }
    alloc->free(ptr);

    alloc->dump_slabs();

    //donot forget to destory
    alloc->destroy();
    return 0;
}
