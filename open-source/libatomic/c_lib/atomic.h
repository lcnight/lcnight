#ifndef _ATOMIC_H
#define _ATOMIC_H

#define LOCK_PREFIX "lock "

#define likely(x)        __builtin_expect(!!(x), 1)
#define unlikely(x)      __builtin_expect(!!(x), 0)

#if defined(__amd64__) || defined(__ia64__)

typedef volatile long atomic64;
typedef volatile int atomic32;
typedef atomic64 atomic;

#define atomic_xchg         atomic64_xchg
#define atomic_cmpxchg      atomic64_cmpxchg
#define atomic_add          atomic64_add
#define atomic_sub          atomic64_sub
#define atomic_sub_and_test atomic64_sub_and_test
#define atomic_inc          atomic64_inc
#define atomic_dec          atomic64_dec
#define atomic_dec_and_test atomic64_dec_and_test
#define atomic_inc_and_test atomic64_inc_and_test
#define atomic_add_negative atomic64_add_negative
#define atomic_add_return   atomic64_add_return
#define atomic_sub_return   atomic64_sub_return
#define atomic_add_unless   atomic64_add_unless

#define atomic64_inc_return(v)  (atomic64_add_return(1, (v)))
#define atomic64_dec_return(v)  (atomic64_sub_return(1, (v)))

#define atomic_inc_return   atomic64_inc_return
#define atomic_dec_return   atomic64_dec_return

#else

typedef volatile int atomic32;
typedef atomic32 atomic;

#define atomic_xchg         atomic32_xchg
#define atomic_cmpxchg      atomic32_cmpxchg
#define atomic_add          atomic32_add
#define atomic_sub          atomic32_sub
#define atomic_sub_and_test atomic32_sub_and_test
#define atomic_inc          atomic32_inc
#define atomic_dec          atomic32_dec
#define atomic_dec_and_test atomic32_dec_and_test
#define atomic_inc_and_test atomic32_inc_and_test
#define atomic_add_negative atomic32_add_negative
#define atomic_add_return   atomic32_add_return
#define atomic_sub_return   atomic32_sub_return
#define atomic_add_unless   atomic32_add_unless

#define atomic_inc_return   atomic32_inc_return
#define atomic_dec_return   atomic32_dec_return

#endif

#define atomic32_inc_return(v)  (atomic32_add_return(1, (v)))
#define atomic32_dec_return(v)  (atomic32_sub_return(1, (v)))

#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)


#if defined(__amd64__) || defined(__ia64__)


static inline long 
atomic64_xchg( atomic64 x, atomic64 *ptr )
{
    __asm__ __volatile__(
        LOCK_PREFIX "xchgq %0,%1"
        : "=r" (x)
        : "m" (*ptr), "0" (x)
        : "memory"
    );
    
    return x;
}

static inline long 
atomic64_cmpxchg( atomic64 *ptr, atomic64 old, atomic64 new )
{
    atomic64 prev;
    
    __asm__ __volatile__(
        LOCK_PREFIX "cmpxchgq %1,%2"
        : "=a"(prev)
        : "r"(new), "m"(*ptr), "0"(old)
        : "memory");

    return prev;
}

static inline void 
atomic64_add( long i, atomic64 *v )
{
	__asm__ __volatile__( 
        LOCK_PREFIX "addq %1,%0"
		: "=m" (*v)
		: "ir" (i), "m" (*v)
    );
}

static inline void 
atomic64_sub( long i, atomic64 *v )
{
	__asm__ __volatile__( 
        LOCK_PREFIX "subq %1,%0"
        : "=m" (*v)
        : "er" (i), "m" (*v)
    );
}

static inline int 
atomic64_sub_and_test( long i, atomic64 *v )
{
	unsigned char c;

	__asm__ __volatile__( 
        LOCK_PREFIX "subq %2,%0; sete %1"
		: "=m" (*v), "=qm" (c)
		: "er" (i), "m" (*v) 
        : "memory"
    );
    
	return c;
}

static inline void 
atomic64_inc( atomic64 *v )
{
    __asm__ __volatile__ (
        LOCK_PREFIX "incq %0"
        : "+m" (*v)
    );
}

static inline void 
atomic64_dec( atomic64 *v )
{
    __asm__ __volatile__ (
        LOCK_PREFIX "decq %0"
        : "+m" (*v)
    );
}

static inline int 
atomic64_dec_and_test( atomic64 *v )
{
	unsigned char c;

	__asm__ __volatile__ (
        LOCK_PREFIX "decq %0; sete %1"
		: "=m" (*v), "=qm" (c)
        : "m" (*v)
        : "memory"
    );
    
	return c != 0;
}

static inline int 
atomic64_inc_and_test( atomic64 *v )
{
	unsigned char c;

	__asm__ __volatile__ (
        LOCK_PREFIX "incq %0; sete %1"
		: "=m" (*v), "=qm" (c)
		: "m" (*v) 
        : "memory"
    );
    
	return c != 0;
}

static inline int 
atomic64_add_negative( long i, atomic64 *v )
{
	unsigned char c;

	__asm__ __volatile__ (
        LOCK_PREFIX "addq %2,%0; sets %1"
		: "=m" (*v), "=qm" (c)
		: "er" (i), "m" (*v) 
        : "memory"
    );
    
	return c;
}

static inline long 
atomic64_add_return( long i, atomic64 *v )
{
	long __i = i;
    
	__asm__ __volatile__ (
        LOCK_PREFIX "xaddq %0, %1;"
        : "+r" (i), "+m" (*v)
        : 
        : "memory"
    );
    
	return i + __i;
}

static inline long 
atomic64_sub_return( long i, atomic64 *v )
{
	return atomic64_add_return(-i, v);
}

static inline int 
atomic64_add_unless( atomic64 *v, long a, long u )
{
	long c, old;
    
	c = *v;
    
	for (;;) {
		if (unlikely(c == (u)))
			break;
		old = atomic64_cmpxchg((v), c, c + (a));
		if (likely(old == c))
			break;
		c = old;
	}
    
	return c != (u);
}

static inline void 
atomic_or_long(unsigned long *v1, unsigned long v2)
{
	__asm__ __volatile__ (
        LOCK_PREFIX "orq %1, %0" 
        : "+m" (*v1) 
        : "r" (v2)
    );
}

#endif

static inline int 
atomic32_xchg( atomic32 x, atomic32 *ptr )
{
    __asm__ __volatile__(
        LOCK_PREFIX "xchgl %0,%1"
        : "=r" (x)
        : "m" (*ptr), "0" (x)
        : "memory"
    );
    
    return x;
}

static inline int 
atomic32_cmpxchg( atomic32 *ptr, atomic32 old, atomic32 new )
{
    atomic32 prev;
    
    __asm__ __volatile__(
        LOCK_PREFIX "cmpxchgl %1,%2"
        : "=a"(prev)
        : "r"(new), "m"(*ptr), "0"(old)
        : "memory");

    return prev;
}

static inline void 
atomic32_add( int i, atomic32 *v )
{
	__asm__ __volatile__( 
        LOCK_PREFIX "addl %1,%0"
		: "=m" (*v)
		: "ir" (i), "m" (*v)
    );
}

static inline void 
atomic32_sub( int i, atomic32 *v )
{
	__asm__ __volatile__( 
        LOCK_PREFIX "subl %1,%0"
        : "=m" (*v)
        : "er" (i), "m" (*v)
    );
}

static inline int 
atomic32_sub_and_test( int i, atomic32 *v )
{
	unsigned char c;

	__asm__ __volatile__( 
        LOCK_PREFIX "subl %2,%0; sete %1"
		: "=m" (*v), "=qm" (c)
		: "er" (i), "m" (*v) 
        : "memory"
    );
    
	return c;
}

static inline void 
atomic32_inc( atomic32 *v )
{
    __asm__ __volatile__ (
        LOCK_PREFIX "incl %0"
        : "+m" (*v)
    );
}

static inline void 
atomic32_dec( atomic32 *v )
{
    __asm__ __volatile__ (
        LOCK_PREFIX "decl %0"
        : "+m" (*v)
    );
}

static inline int 
atomic32_dec_and_test( atomic32 *v )
{
	unsigned char c;

	__asm__ __volatile__ (
        LOCK_PREFIX "decl %0; sete %1"
		: "=m" (*v), "=qm" (c)
        : "m" (*v)
        : "memory"
    );
    
	return c != 0;
}

static inline int 
atomic32_inc_and_test( atomic32 *v )
{
	unsigned char c;

	__asm__ __volatile__ (
        LOCK_PREFIX "incl %0; sete %1"
		: "=m" (*v), "=qm" (c)
		: "m" (*v) 
        : "memory"
    );
    
	return c != 0;
}

static inline int 
atomic32_add_negative( int i, atomic32 *v )
{
	unsigned char c;

	__asm__ __volatile__ (
        LOCK_PREFIX "addl %2,%0; sets %1"
		: "=m" (*v), "=qm" (c)
		: "er" (i), "m" (*v) 
        : "memory"
    );
    
	return c;
}

static inline int 
atomic32_add_return( int i, atomic32 *v )
{
	int __i = i;
    
	__asm__ __volatile__ (
        LOCK_PREFIX "xaddq %0, %1;"
        : "+r" (i), "+m" (*v)
        : 
        : "memory"
    );
    
	return i + __i;
}

static inline int 
atomic32_sub_return( int i, atomic32 *v )
{
	return atomic32_add_return(-i, v);
}

static inline int 
atomic32_add_unless( atomic32 *v, int a, int u )
{
	int c, old;
    
	c = *v;
    
	for (;;) {
		if (unlikely(c == (u)))
			break;
		old = atomic32_cmpxchg((v), c, c + (a));
		if (likely(old == c))
			break;
		c = old;
	}
    
	return c != (u);
}




static inline short int 
atomic_inc_short( short int *v )
{
	__asm__ __volatile__( 
        LOCK_PREFIX "addw $1, %0"
        : "+m" (*v)
    );
    
	return *v;
}

#endif
