/**
 * =====================================================================================
 *       @file  slab_allocator.h
 *      @brief  header file for slab mem pool with fixed size items
 *
 *     Created  05/29/2011 05:01:00 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, lc
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#ifndef  SLAB_ALLOCATOR_H
#define  SLAB_ALLOCATOR_H

#include  <stdint.h>
#include  <cstddef>
#include  <cstdlib>
//#include  <cstdio>

#define     MIN_SLAB_NUM  0
#define     MAX_SLAB_NUM  255

#define     MAX_MEM_SIZE      64*1024*1024
#define     DEFAULT_FACTOR    1.25
#define     MIN_ITEM_SIZE     48
#define     MAX_ITEM_SIZE     1024*1024

#define     ITEM_ALGIN_SIZE   8

typedef   uint8_t   uchar;
typedef   unsigned int  uint;
typedef   unsigned int  real_time_t;
/*
 * =====================================================================================
 *        Class:  Slab_alloc
 *  Description:  get fixed size item for this mem pool allocator
 * =====================================================================================
 */
class slab_alloc
{
	private:
		/* ====================  LIFECYCLE    ======================================= */
		slab_alloc (size_t maxsize=MAX_MEM_SIZE, double factor=DEFAULT_FACTOR,
                            size_t min_item = MIN_ITEM_SIZE, size_t max_item = MAX_ITEM_SIZE ) {      /* constructor */
                    m_maxmemsize = maxsize;
                    _factor = factor;
                    _min_item_size = min_item;
                    _max_item_size = max_item;
                    init();
                }

                void init();
                void uninit();
        public:
		/* ====================  OPERATIONS   ======================================= */
                static slab_alloc* get_instance();
                static void destroy();

                ~slab_alloc() {
//                    fprintf(stderr, "why ~slab_alloc be called ... ");
                    uninit();
                }
                void* malloc(size_t size);
                void* calloc(size_t nmem, size_t size);

                void* reallocate(void *ptr, size_t newsize);
                void free(void* p);

                void inc_previlege(item_t *it); /* increase the important of item */

                void dump_slabs(bool usedonly = true); //dump the real memory allocated condition to screen for eye check:)
                void dump_oneslab(uint id); //dump only specified slab classid

		/* ====================  DATA MEMBERS ======================================= */
	private:
            typedef struct store_item {
                struct store_item  *prev; /* pointer to previous item in slab */
                struct store_item  *next; /* to next item in slab */

//                real_time_t last_time; /* last access time */
                uint slab_clsid;    /* class id of slabs */

                char end[0];  /* end of manager header, for fetch data */
            } item_t; /* ----------  end of struct store_item  ---------- */

            typedef struct slab_class {
                uint itemsize;  /* sizes of one item = len(manager-header + real-data) */
                uint perslab;   /* how many items per slab */

                item_t *head;   /* head of used items for a specific slab class */
                item_t *tail;   /* tail of used items for a specific slab class */

                item_t *free_list;   /* head of free list for a specific slab class */

                void *end_page_ptr;      /* pointer to next free item at end of page, or 0 */
                uint end_page_free;      /* number of items remaining free at end of last alloced page */

                void **slab_list;   /* array of slab pointers */
                uint slab_list_size;     /* size of prev array, can contain slabs max array */
                uint nslabs;         /* how many slabs were allocated for this class */

                size_t requested; /* The number of total requested bytes for this slab class*/
            } slab_t; /* ----------  end of struct slab_class  ---------- */

        private:
            size_t get_slabid(size_t size); /* get slabid for item with size(not include length of item_t header) bytes */
            bool grow_slab(size_t slabid);  /* grow slab num for specific slab class */
            bool do_newslab(size_t slabid); /* get a new slab for a specific slab class */

            void insert_usedlist(size_t id, item_t* it); /* insert one item to the head of used list */
            void insert_freelist(size_t id, item_t* it); /* insert one item to the head of free list */

            bool un_link(item_t* it);     /* delete one item from used list */

            void init_slab_class (int i, int size);
            size_t m_maxmemsize;  /* max memory size can be managed by this allocator */
            double _factor;      /* incremental factor */
            size_t _min_item_size;
            size_t _max_item_size;

            size_t _mem_allocated;  /* totally allocated memory bytes */
            size_t _slab_num;   /* record current slab number with max item = _max_item_size */

            static int instance_num;  /* record instance num of allocator class */
            static slab_alloc *instance; /* the only one instance handle of slab_alloc class */

            slab_t _slabs[MAX_SLAB_NUM];
};/* -----  end of class  Slab_alloc  ----- */

#endif  /*SLAB_ALLOCATOR_H*/
