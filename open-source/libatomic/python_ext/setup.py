#! /usr/bin/python

from distutils.core import setup, Extension

m = Extension( 'atomic_queue', 
               sources = [ 'py_atomic_q.c', '../c_lib/atomic_q.c' ],
               include_dirs = ['../c_lib/'],
)

n = Extension( 'atomic', 
               sources = [ 'py_atomic.c' ],
               include_dirs = ['../c_lib/'],
)

setup ( name = 'atomic', 
        version = '1.0',
        description = 'atomic operations', 
        ext_modules = [ m, n ],
        scripts = ['AtomicQueue.py']
)
