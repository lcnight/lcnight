/**
 * =====================================================================================
 *       @file  slab_allocator.cpp
 *      @brief  this is a file for allocarot fixed mem item just as memcache does
 *
 *     Created  05/29/2011 04:55:51 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, lc
 *
 *     @author  lc (l.c.), lc@taomee.com
 * =====================================================================================
 */
#include  "slab_allocator.h"
#include  <cstring>
#include  <cstdio>

//algin request size, = manager-header-len + algined(request-size)
#define     MIN_SIZE  (_min_item_size)
#define     ALGIN_UNIT_SIZE(size)     ((size + ITEM_ALGIN_SIZE -1 ) & ~(ITEM_ALGIN_SIZE - 1))
#define     ALGIN_SIZE(size)  \
            (size = size < MIN_SIZE ? MIN_SIZE : size,  ALGIN_UNIT_SIZE(size))
#define     GET_SLAB_PTR(it)  (it == NULL?NULL : &_slabs[it->slab_clsid])

int slab_alloc::instance_num = 0;
slab_alloc *slab_alloc::instance = NULL;

slab_alloc* slab_alloc::get_instance() {
    if(instance_num == 0 && instance == NULL ) {
        instance = new slab_alloc();
        instance_num ++;
    }
    return instance;
}

void slab_alloc::destroy() {
    instance_num ++;
    delete instance; 
}

inline void slab_alloc::init_slab_class (int i, int size)
{
    _slabs[i].itemsize = size;
    _slabs[i].perslab = _max_item_size / size;
    _slabs[i].head = 0;   /* head of used items */
    _slabs[i].tail =  0;   /* tail of used items */

    _slabs[i].free_list = 0;   /* to head of free list  */

    _slabs[i].end_page_ptr = 0;      /* pointer to next free item at end of page, or 0 */
    _slabs[i].end_page_free = 0;      /* number of items remaining free at end of last alloced page */

    _slabs[i].slab_list = 0;   /* array of slab pointers */
    _slabs[i].slab_list_size = 0;     /* size of prev array, can contain slabs max array */
    _slabs[i].nslabs = 0;         /* how many slabs were allocated for this class */

    _slabs[i].requested = 0; /* The number of total requested bytes for this slab class*/
    return ;
}/* -----  end of function init_slab_class  ----- */

void slab_alloc::init(){
    size_t i = MIN_SLAB_NUM;
    size_t size = sizeof(item_t) + _min_item_size;
    for(; i < MAX_SLAB_NUM && size <= _max_item_size/_factor; i++) {
        size = ALGIN_SIZE(size);  //inlude len of item_t header

        init_slab_class(i, size);

        size *= _factor;
    }

    _slab_num = i;
    init_slab_class(i, _max_item_size);
}

inline size_t slab_alloc::get_slabid(size_t size) { //size do not include length of item_t header
    size_t i = MIN_SLAB_NUM;
    for(; i < _slab_num ; i++) {
        if( _slabs[i].itemsize >= sizeof(item_t) + size) break;
    }
    if(i == MIN_SLAB_NUM) return MIN_SLAB_NUM;
    if(i == _slab_num) return _slab_num;
    return i;
}

bool slab_alloc::grow_slab(size_t slabid){  /* grow slab num for specific slab class */
    slab_t *p = &_slabs[slabid];
    if(p->slab_list_size == p->nslabs) {
        int newsize = p->slab_list_size == 0 ? 16 : 2*p->slab_list_size;
        void* ptr = ::realloc(p->slab_list, newsize*sizeof(void*));
        if(ptr == 0) return false;

        p->slab_list = (void**)ptr;
        p->slab_list_size = newsize;
    }
    return true;
}

void inc_previlege(item_t *it){ /* increase the important of item, swap(it, it->prev)*/
    slab_t *p = &_slabs[slabid];
    if(p == NULL || it == NULL 
            || p->head == NULL || p->tail == NULL || (p->head == p->tail)
            || it == p->head || it->prev == p->head)
        return ;
    
    item_t *new_tail = NULL;
    if(it == p->tail) 
        new_tail = it->prev;

    item_t *new_head = NULL;
    if(it->prev == p->head)
        new_head = it->prev;

    //swap
    item_t *it_prev = it->prev;
    it_prev->next = it->next;
    it->prev = it_prev->prev;
    it->next = it_prev;
    it_prev->prev = it;

    if(new_tail != NULL)
        p->tail = new_tail;
    if(new_head != NULL)
        p->head = new_head;
}

bool slab_alloc::do_newslab(size_t slabid){ /* get a new slab for a specific slab class */
    if(m_maxmemsize < _mem_allocated)
        return false;

    slab_t *p = &_slabs[slabid];
    size_t len = p->itemsize*p->perslab;
    void *ptr = ::malloc(len);
    if(ptr == 0) return false;

    p->slab_list[p->nslabs] = ptr;
    p->nslabs ++;

    p->end_page_ptr = ptr;
    p->end_page_free = p->perslab;

    _mem_allocated += len;

    return true;
}

inline void slab_alloc::insert_usedlist(size_t id, item_t* it){ /* insert one item to the head of used list */
    slab_t *p = &_slabs[id];
    it->prev = it->next = NULL;
    if(p->head == NULL && p->tail == NULL) {
        p->head = p->tail = it;
    } else {
        it->next = p->head;
        p->head->prev = it;
        p->head = it;
    }
}

inline void slab_alloc::insert_freelist(size_t id, item_t* it){ /* insert one item to the head of free list */
    slab_t *p = &_slabs[id];
    it->next = p->free_list;
    p->free_list = it;
}

inline bool slab_alloc::un_link( item_t * it ) {     /* delete one item from used list */
    if(it == NULL) return false;

    slab_t *p = &_slabs[it->slab_clsid];
    if(it != p->head && it != p->tail) {
        it->prev->next = it->next;
        it->next->prev = it->prev;
    } else if( it == p->head && it == p->tail ) {
        p->head = 0;
        p->tail = 0;
    } else if( it == p->head ) {
        p->head = it->next;
        it->next->prev = 0;
    } else if( it == p->tail ) {
       p->tail = it->prev;
       it->prev->next = 0;
    } else {
        return false;
    }
    it->next = 0;
    it->prev = 0;
    return true;
}

void* slab_alloc::malloc(size_t size){
    if(size == 0) return 0;

    size = ALGIN_SIZE(size);
    if( size > _max_item_size ) {
        return 0;
    }
    size_t slabid = get_slabid(size);

    slab_t *p = &_slabs[slabid];
//    size_t len = p->itemsize * p->perslab;

    item_t* ret = NULL;
    if(p->free_list != NULL) { /* has free item for this slab class */
        ret = p->free_list;
        p->free_list = p->free_list->next;
    } else  if(p->end_page_free > 0) {  /* has free unallocated mem */
        ret = (item_t*)p->end_page_ptr;
        p->end_page_ptr = ((char*)p->end_page_ptr) + p->itemsize;
        p->end_page_free --;
    } else {   /* grow nslabs num and get one new slab */
        grow_slab(slabid);
        if(do_newslab(slabid) == true) //handle can not get new slab: max memory reached //return NULL;
        {
            ret = (item_t*)p->end_page_ptr;
            p->end_page_ptr = ((char*)p->end_page_ptr) + p->itemsize;
            p->end_page_free --;
        } else {
            ret = p->tail;
            bool un_link_flag = un_link(p->tail);
            if(un_link_flag != true)  return NULL; //un_link the last used item fail, maybe can not get one item
        }
    }
//    item_t *t = (item_t*)ret;
    ret->slab_clsid = slabid;
    insert_usedlist(slabid, ret);

    //add requested size
    p->requested += p->itemsize;

    return (void*)ret->end;
}

void* slab_alloc::calloc(size_t nmem, size_t size){
    void *ret = this->malloc(nmem*size);
    if(ret == NULL) return NULL;

    item_t *it =  (item_t*)((char*)ret - sizeof(item_t));
    slab_t *p = &_slabs[it->slab_clsid];
    memset(ret, 0, p->itemsize - sizeof(item_t));
    return ret;
}

void* slab_alloc::reallocate(void *ptr, size_t newsize){
    if(newsize == 0) {
        this->free(ptr);
        return 0;
    }
    newsize = ALGIN_SIZE(newsize);
    item_t *it = (item_t*)((char*)ptr - sizeof(item_t));
    slab_t *p = &_slabs[it->slab_clsid];
    void *ret = this->malloc(newsize);
    if(ptr != NULL) {
        memcpy(ret, ptr, p->itemsize - sizeof(item_t));
        this->free(ptr);
    }
    return ret;
}

void slab_alloc::free(void* ptr){
    if(ptr == 0) return;

    item_t *it =  (item_t*)((char*)ptr - sizeof(item_t));
    slab_t *p = &_slabs[it->slab_clsid];

    un_link(it);
    insert_freelist(it->slab_clsid, it);

    //minus request size
    p->requested -= p->itemsize;
}

void slab_alloc::uninit(){
    //release resources of every used slab class
    for( size_t i = MIN_SLAB_NUM; i < _slab_num ; i++ ) {
        //release each slabs for corresponding slab class
        for(uint x = 0; x < _slabs[i].nslabs; x++) {
            ::free(_slabs[i].slab_list[x]);
        }
        //release slablist array
        ::free(_slabs[i].slab_list);
    }
}
#define     OUT   printf
void slab_alloc::dump_oneslab(uint id){
        slab_t *p = &_slabs[id];
        OUT("slab class id:%u\n", id );
        OUT("   item size   : %u\n", p->itemsize);
        OUT("   item perslab: %u\n", p->perslab);
        OUT("   requested   : %lu\n", p->requested);
        OUT("   slab num    : %u\n", p->nslabs);
        OUT("   used slab address list:\n");
        for(uint x = 0; x < p->nslabs; x++) {  //for each slab item under one class
            OUT("       %p", p->slab_list[x]);
        }
        OUT("\n   used items address list :");
        item_t *s = p->head;
        OUT("   %p", s);
        for(; s != p->tail; s = s->next) {  //for usedlist
            OUT("   %p", s);
        }
        OUT("\n   free items address list :");
        
        for(s = p->free_list; s != NULL; s = s->next) {  //for freelist
           OUT("   %p", s);
        }
        OUT("\n\n");
}

void slab_alloc::dump_slabs(bool usedonly){ //dump the real memory allocated condition to screen for eye check:)
    size_t totalused_size = 0;
    size_t totalused_slab = 0;
    OUT("slab class id| item size| perslab| requested| slab num| used slab address list \n");
    for( uint i = MIN_SLAB_NUM; i < _slab_num ; i++ ) {        //for each slab_class
        slab_t *p = &_slabs[i];
        if(usedonly == true && p->nslabs == 0)
            continue;
        OUT("%13u|%10u|%8u|%10lu|%9u|", i, p->itemsize, p->perslab, p->requested, p->nslabs);
        for(uint x = 0; x < p->nslabs; x++) {  //for each slab item under one class
            OUT(" %p", p->slab_list[x]);
        }
        OUT("\n   used items address list :");
        item_t *s = p->head;
        OUT("   %p", s);
        for(; s != p->tail; s = s->next) {  //for usedlist
            OUT("   %p", s);
        }
        OUT("\n   free items address list :");
        
        for(s = p->free_list; s != NULL; s = s->next) {  //for freelist
           OUT("   %p", s);
        }
        OUT("\n\n");
        totalused_size += p->requested;
        totalused_slab++;
    }
    OUT("total allocated memory:%lu\n", _mem_allocated );
    OUT("total bytes in used memory:%lu\n",totalused_size);
    OUT("total slab classes in used:%lu\n",totalused_slab);
    OUT("min item size:%lu\n", _min_item_size );
    OUT("max item size:%lu\n", _max_item_size );
}
