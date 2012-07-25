#include <Python.h>
#include <atomic_q.h>


PyObject *_c_Queue_new( PyObject *self, PyObject *args )
{
    if ( !PyArg_ParseTuple( args, ":Queue_new" ) )
        return Py_None;
    
    QUEUE* q = Queue_new();
    
    if (q == NULL)
        return Py_None;
    
    return PyCObject_FromVoidPtr( (void *)Queue_new(), Queue_delete );
}

PyObject *_c_Queue_IsEmpty( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:Queue_IsEmpty", &pyq ) )
        return Py_None;
    
    return Py_BuildValue( "I", Queue_IsEmpty( PyCObject_AsVoidPtr(pyq) ) );
}

PyObject *_c_Queue_Length( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:Queue_Length", &pyq ) )
        return Py_None;
    
    return Py_BuildValue( "I", Queue_Length( PyCObject_AsVoidPtr(pyq) ) );
}

PyObject *_c_AQSput( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    PyObject * pyi = NULL ;
    
    if ( !PyArg_ParseTuple( args, "OO:AQSput", &pyq, &pyi ) )
        return Py_None;
    
    Py_INCREF(pyi);
    
    AQSput(PyCObject_AsVoidPtr(pyq),(void*)pyi);
    
    return Py_None;
}

PyObject *_c_AQSget( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:AQSget", &pyq ) )
        return Py_None;
    
    return AQSget(PyCObject_AsVoidPtr(pyq));
}

PyObject *_c_AQSget_nowait( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    PyObject * pyi = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:AQSget_nowait", &pyq ) )
        return Py_None;
    
    pyi =  AQSget(PyCObject_AsVoidPtr(pyq));
    
    if ( pyi == NULL )
    {
        return Py_None;
    }
    
    return pyi;
}

PyObject *_c_AQMput( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    PyObject * pyi = NULL ;
    
    if ( !PyArg_ParseTuple( args, "OO:AQMput", &pyq, &pyi ) )
        return Py_None;
    
    Py_INCREF(pyi);
    
    AQMput(PyCObject_AsVoidPtr(pyq),(void*)pyi);
    
    return Py_None;
}

PyObject *_c_AQMget( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:AQMget", &pyq ) )
        return Py_None;
    
    return AQMget(PyCObject_AsVoidPtr(pyq));
}

PyObject *_c_AQMget_nowait( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    PyObject * pyi = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:AQMget_nowait", &pyq ) )
        return Py_None;
    
    pyi =  AQMget(PyCObject_AsVoidPtr(pyq));
    
    if ( pyi == NULL )
    {
        return Py_None;
    }
    
    return pyi;
}

PyObject *_c_AQM2get( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:AQMget", &pyq ) )
        return Py_None;
    
    return AQMget(PyCObject_AsVoidPtr(pyq));
}

PyObject *_c_AQM2get_nowait( PyObject *self, PyObject *args )
{
    PyObject * pyq = NULL ;
    PyObject * pyi = NULL ;
    
    if ( !PyArg_ParseTuple( args, "O:AQM2get_nowait", &pyq ) )
        return Py_None;
    
    pyi =  AQMget(PyCObject_AsVoidPtr(pyq));
    
    if ( pyi == NULL )
    {
        return Py_None;
    }
    
    return pyi;
}

static PyMethodDef addMethods[] =
{ 
    // WARNING : the sentences below is maybe over 80 columns.
    // name             func                 flag           docstring
    { "Queue_new"      , _c_Queue_new      , METH_VARARGS, "init a queue. auto gc."       },
    { "Queue_IsEmpty"  , _c_Queue_IsEmpty  , METH_VARARGS, "dectect the queue is empty."  },
    { "Queue_Length"   , _c_Queue_Length   , METH_VARARGS, "return the queuelength."      },
    { "AQSput"         , _c_AQSput         , METH_VARARGS, "put a node to queue."         },
    { "AQSget"         , _c_AQSget         , METH_VARARGS, "get a node from queue."       },
    { "AQSget_nowait"  , _c_AQSget_nowait  , METH_VARARGS, "get a node from queue imm."   },
    { "AQMput"         , _c_AQMput         , METH_VARARGS, "put a node to queue."         },
    { "AQMget"         , _c_AQMget         , METH_VARARGS, "get a node from queue."       },
    { "AQMget_nowait"  , _c_AQMget_nowait  , METH_VARARGS, "get a node from queue imm."   },
    { "AQM2get"        , _c_AQM2get        , METH_VARARGS, "get a node from queue."       },
    { "AQM2get_nowait" , _c_AQM2get_nowait , METH_VARARGS, "get a node from queue imm."   },
    {  NULL            ,  NULL             ,            0,  NULL                          }
};

PyMODINIT_FUNC initatomic_queue()
{
    Py_InitModule( "atomic_queue", addMethods );
}
