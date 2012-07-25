======================
|Atomic| 使用手册
======================


.. module:: Atomic
   :synopsis: Atomic Operate.
.. moduleauthor:: D13 <dainan@staff.sina.com.cn>


.. contents:: 索引

.. |Atomic| replace:: :strong:`Atomic`


|Atomic| 的使用
======================


|Atomic| 数据类型
----------------------

    Atomic，自动同于机器位长。volatile long 或 volatile int。
    
    +-------------+--------------------+
    | Atomic64    | volatile long      |
    +-------------+--------------------+
    | Atomic32    | volatile int       |
    +-------------+--------------------+
    
    
|Atomic| 函数
----------------------


.. cfunction:: long/int atomic_xchg( atomic x, atomic *ptr )

    用 x 原子交换ptr所指数值，返回值在64位下为long，32位下为int。
    
    
.. cfunction:: long/int atomic_cmpxchg( atomic *ptr, atomic old, atomic new )

    当 old 值与ptr所指数值相等时，用 new 的数值替换ptr所指数值。
    
    
.. cfunction:: void atomic_add( long/int i, atomic *v )

    v所指内存原子的加i。
    

.. cfunction:: void atomic_sub( long/int i, atomic *v )

    v所指内存原子的减i。
        

.. cfunction:: int atomic_sub_and_test( long/int i, atomic *v )

    v所指内存原子的减i，如果减后数值为0，则返回true，否则返回false。
    

.. cfunction:: void atomic_inc( atomic *v )
        
    v所指内存原子加1。
    

.. cfunction:: void atomic_dec( atomic *v )

    v所指内存原子减1。
    

.. cfunction:: int atomic_dec_and_test( atomic *v )
        
    v所指内存原子减1，如果减后数值为0，则返回true，否则返回false。


.. cfunction:: int atomic_inc_and_test( atomic *v )

    v所指内存原子加1，如果减后数值为0，则返回true，否则返回false。
    

.. cfunction:: int atomic_add_negative( long/int i, atomic *v )
        
    v所指内存原子的加i，如果结果为负数，则返回true，否则返回false。
        

.. cfunction:: long/int atomic_add_return( long/int i, atomic *v )

    v所指内存原子的加i，并返回结果。


.. cfunction:: long/int atomic_sub_return( long/int i, atomic *v )
        
    v所指内存原子的减i，并返回结果。
    

.. cfunction:: int atomic_add_unless( atomic *v, long/int a, long/int u )

    如果v所指内存的值与u相等，则v所指的内存加a。
    

.. cfunction:: void atomic_or_long(unsigned long *v1, unsigned long v2)

    原子的将v1所指的内存与v2进行或运算，并返回结果。
    

.. cfunction:: short int atomic_inc_short( short int *v )

    v所指内存原子加1。类型为short。
    
    