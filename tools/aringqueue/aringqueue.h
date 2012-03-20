/**
 * =====================================================================================
 *       @file  aringqueue.h
 *      @brief  header file
 *
 *   @internal
 *     Created  09/15/2011 03:16:42 PM 
 *
 *     @author  lc (l.c.), lc@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef  __ARINGQUEUE_H__
#define  __ARINGQUEUE_H__

class aringqueue
{
    public:
        aringqueue();
        aringqueue(int buf_sz);
        ~aringqueue();

        int push(void *buf, int size);
        int pop(void *buf, int size);
        int peer(void *buf, int size);
        int get_capcity();
        int get_length();

#ifdef DEBUG 
        void dump();
#endif

    protected:

    private:
        //inhibit copy, assign operation
        aringqueue& operator=(const aringqueue obj);
        aringqueue(const aringqueue& obj);

        void resize(int buf_sz);

    private:
        bool stat_ok;
        bool is_full;
        int m_capcity;
        char *m_buf;
        char *m_read;
        char *m_write;
}; /* --  end of class  -- */


#endif  /*__ARINGQUEUE_H__*/
