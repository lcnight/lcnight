#include <Python.h>
#include <atomic.h>

static void pyatomic_destruct( void *p )
{
    free(p);
}

static PyObject* pyatomic_create(PyObject *self, PyObject *args)
{
    PyObject *pp;
    atomic *p;
    
    p = (atomic *)malloc(sizeof(atomic));
    
    if ( p == NULL )
        return Py_None;
    
    *p = 0;
    
    pp = PyCObject_FromVoidPtr( (void *)p, pyatomic_destruct );
    
    return pp;
}

static PyObject* pyatomic_inc(PyObject *self, PyObject *args)
{
    PyObject *pp;
    atomic *p;
    
    if ( ! PyArg_ParseTuple(args, "O", &pp) )
        return NULL;
    
    if ( pp == NULL )
        return NULL;
    
    p = (atomic *)PyCObject_AsVoidPtr(pp);
    
    if ( p == NULL )
        return NULL;
    
    atomic_inc(p);
    
    return Py_None;
}

static PyObject* pyatomic_dec(PyObject *self, PyObject *args)
{
    PyObject *pp;
    atomic *p;
    
    if ( ! PyArg_ParseTuple(args, "O", &pp) )
        return NULL;
    
    if ( pp == NULL )
        return NULL;
    
    p = (atomic *)PyCObject_AsVoidPtr(pp);
    
    if ( p == NULL )
        return NULL;
    
    atomic_dec(p);
    
    return Py_None;
}

static PyObject* pyatomic_xchg(PyObject *self, PyObject *args)
{
    PyObject *pp;
    atomic *p;
    atomic v;
    
    #if defined(__amd64__) || defined(__ia64__)
    if ( ! PyArg_ParseTuple(args, "O!k", &pp, &v) )
        return NULL;
    #else
    if ( ! PyArg_ParseTuple(args, "O!I", &pp, &v) )
        return NULL;
    #endif

    p = (atomic *)PyCObject_AsVoidPtr(pp);
    
    if ( p == NULL )
        return NULL;
    
    v = atomic_xchg( v, p );
    
    #if defined(__amd64__) || defined(__ia64__)
    return Py_BuildValue( "k", v );
    #else
    return Py_BuildValue( "I", v );
    #endif
}

static PyObject* pyatomic_get(PyObject *self, PyObject *args)
{
    PyObject *pp;
    atomic *p;
    
    if ( ! PyArg_ParseTuple(args, "O", &pp) )
        return NULL;
    
    if ( pp == NULL )
        return NULL;
    
    p = (atomic *)PyCObject_AsVoidPtr(pp);
    
    if ( p == NULL )
        return NULL;
        
    #if defined(__amd64__) || defined(__ia64__)
    return Py_BuildValue( "k", *p );
    #else
    return Py_BuildValue( "I", *p );
    #endif
}

static PyMethodDef addMethods[] =
{ 
    // WARNING : the sentences below is maybe over 80 columns.
    // name   func             flag           docstring
    { "new" , pyatomic_create, METH_VARARGS, "new a counter."  },
    { "inc" , pyatomic_inc   , METH_VARARGS, "inc the counter."  },
    { "dec" , pyatomic_dec   , METH_VARARGS, "dec the counter."  },
    { "xchg", pyatomic_xchg  , METH_VARARGS, "read the counter and set it as zero." },
    { "get" , pyatomic_get   , METH_VARARGS, "get the counter num."  },
    {  NULL , NULL           ,            0,  NULL  }
};

PyMODINIT_FUNC initatomic()
{
    Py_InitModule( "atomic", addMethods );
}
