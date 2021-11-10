// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef REFCOUNT_DWA2002615_HPP
# define REFCOUNT_DWA2002615_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/cast.hpp>

namespace boost { namespace python { 

template <class T>
inline T* incref(T* p)
{
    Py_INCREF(python::upcast<PyObject>(p));
    return p;
}

template <class T>
inline T* xincref(T* p)
{
    Py_XINCREF(python::upcast<PyObject>(p));
    return p;
}

template <class T>
inline void decref(T* p)
{
    assert( Py_REFCNT(python::upcast<PyObject>(p)) > 0 );
    Py_DECREF(python::upcast<PyObject>(p));
}

template <class T>
inline void xdecref(T* p)
{
    assert( !p || Py_REFCNT(python::upcast<PyObject>(p)) > 0 );
    Py_XDECREF(python::upcast<PyObject>(p));
}

}} // namespace boost::python

#endif // REFCOUNT_DWA2002615_HPP
