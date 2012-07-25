import atomic_queue

class QueueBase(object):
    
    def __init__( self ):
        self.__q = atomic_queue.Queue_new()
        
    def is_empty( self ):
        return atomic_queue.Queue_IsEmpty( self.__q )
        
    def __len__( self ):
        return atomic_queue.Queue_Length( self.__q )
        
        
class QueueSiSo( QueueBase ):
    
    def put( self, value ):
        return atomic_queue.AQSput( self._QueueBase__q, value )
        
    def get( self, ):
        return atomic_queue.AQSget( self._QueueBase__q )
        
class QueueSiMo( QueueBase ):
    
    def put( self, value ):
        return atomic_queue.AQSput( self._QueueBase__q, value )
        
    def get( self, ):
        return atomic_queue.AQMget( self._QueueBase__q )
        
class QueueMiSo( QueueBase ):
    
    def put( self, value ):
        return atomic_queue.AQMput( self._QueueBase__q, value )
        
    def get( self, ):
        return atomic_queue.AQSget( self._QueueBase__q )
        
class QueueMiMo( QueueBase ):
    
    def put( self, value ):
        return atomic_queue.AQMput( self._QueueBase__q, value )
        
    def get( self, ):
        return atomic_queue.AQMget( self._QueueBase__q )
        
