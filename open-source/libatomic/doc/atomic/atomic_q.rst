======================
|AtomicQ| 使用手册
======================


.. module:: AtomicQueue
   :synopsis: Lock-Free Queue using Atomic.
.. moduleauthor:: D13 <dainan@staff.sina.com.cn>


.. contents:: 索引

.. |AtomicQ| replace:: :sup:`Atomic`\ :strong:`Queue`


设计概述
======================

    |AtomicQ| 是利用Atomic原子操作进行设计的无锁队列，目标是进行快速的读取和写入。



|AtomicQ| 的使用
======================


适用条件与局限
----------------------

    
    适用于大并发大流量情况下的队列操作，可以针对多读单取、单读多取、多读多取不同情
    况进行操作，操作对象仅限于指针，且指针不能为NULL。
    
    仅限于386以上CPU，gcc编译器。
    
.. warning::
    
        |AtomicQ| 只能在单读单取、多读单取、单读多取、多读多取中的一种状态下工作。
        一旦选定， |AtomicQ| 就不能转变其工作模式，但是在代码上并没有对此做出限制，
        不同的函数组合依然可能作用在同一个Queue上，因此在编写上必须对此多加注意。


编译
----------------------


    依赖atomic，pthread库，编译时需要连接-lpthread
    
    

建立一个队列
----------------------

.. cfunction:: QUEUE* Queue_new( void )

    在代码中创建一个Queue：
    ::

        #include<atomic_q.h>
        QUEUE * q = Queue_new();
        ...

    这样就建立一个Queue，这个过程内部会使用malloc来创建一个用于描述队列的数据结构。
    
    
写入一个元素
----------------------

.. cfunction:: void AQSput( void *c )
.. cfunction:: void AQMput( void *c )

    单线程情况下，在建立好的QUEUE中写入一个节点：
    ::
        
        ITEM * i = (ITEM *)malloc(sizeof(ITEM));
        ...
        AQSput(q,(void *)i);
        ...
        
        
    在多线程的情况下，写入节点：
    ::
        
        /* thread 0 */
        ITEM * i0 = (ITEM *)malloc(sizeof(ITEM));
        ...
        AQMput(q,(void *)i0);
        ...
        
        /* thread 1 */
        ITEM * i1 = (ITEM *)malloc(sizeof(ITEM));
        ...
        AQMput(q,(void *)i1);
        ...
        
        
    AQSput、AQMput函数不会阻塞线程的运行，在任何时刻Queue都是可写的。但是AQMput函
    数所写入的数据只有在其进入函数前所有线程调用的AQMput函数全部完成时才可读。例如，
    如果有线程1、2，线程1先执行AQMput函数，在完成之前时间片结束，线程2执行AQMput
    函数并完成，此时线程2写入的数据无法被读取的线程读到，只有在线程1结束AQMput函数
    后才可读到。
    
    
读取一个元素
----------------------
    
.. cfunction:: void* AQSget( QUEUE *Q )
.. cfunction:: void* AQSget_nowait( QUEUE *Q )
.. cfunction:: void* AQMget( QUEUE *Q )
.. cfunction:: void* AQMget_nowait( QUEUE *Q )
.. cfunction:: void* AQM2get( QUEUE *Q )
.. cfunction:: void* AQM2get_nowait( QUEUE *Q )
    
        
    同写入元素函数，以AQS开头的函数为单线程使用，AQM开头为多线程使用的函数，一个
    简单的示例如下：
    ::
        
        /* thread 0 */
        ITEM * i0 = (ITEM *)AQMget(q);
        ...
        
        /* thread 1 */
        ITEM * i1 = (ITEM *)AQMget(q);
        ...
        
    其中AQSget_nowait、AQMget_nowait函数不会阻塞线程的运行，在队列中没有元素可读
    的情况下将返回NULL，AQSget、AQMget函数在有元素可读的情况下不会阻塞线程，在没
    有元素可读情况下将进入轮询状态，默认的轮询间隔为10ms。
    
    AQM2get与AQM2get_nowait则采用阻塞线程的方式运行，在有别的线程同时读取元素时会
    使当前线程失去时间片，AQM2get在没有元素可读的情况下进入轮询状态，默认为10ms。
    
.. warning::
    
        AQMget系列函数与AQM2get系列函数不能够在一个Queue上进行混用，与单/多状态的问
        题相同，一旦选定使用哪个函数系列，便不可以再使用另外一个系列中的函数进行
        操作，同样这在代码上没有对此进行限定，因此编写时需要注意。
        
        另，同系列的函数在使用时是可以混用的，即AQMget、AQMget_nowait是可以同时操
        做一个Queue的。


判断是否为空/查看队列长度
--------------------------

.. cfunction:: int Queue_IsEmpty( QUEUE *Q )
.. cfunction:: long Queue_Length( QUEUE *Q )


    Queue_IsEmpty返回0,1表示是否为空，Queue_Length直接返回队列的长度。由于 |AtomicQ|
    在多数情况下操作并不阻塞线程，这些操作时一个过程而非一个点，因此这两个函数返
    回的结果并不能完全的反应当时的情况。
    
.. warning::
    
        Queue_Length只有在编译时加入ATOMICQ_COUNTER_ON宏定义才有效，否则将直接返
        回-1，而Queue_IsEmpty则可以在任何情况下使用。
    
    
删除一个队列
----------------------

.. cfunction:: void Queue_delete( QUEUE *Q )

    删除一个队列：
    ::
        
        Queue_delete(q);
        
    将队列删除，同时会释放队列中没有释放的节点。
    
    