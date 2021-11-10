// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef DECREF_GUARD_DWA20021220_HPP
# define DECREF_GUARD_DWA20021220_HPP

namespace boost { namespace python { namespace detail { 

struct decref_guard
{
    decref_guard(PyObject* o) : obj(o) {}
    ~decref_guard() { Py_XDECREF(obj); }
    void cancel() { obj = 0; }
 private:
    PyObject* obj;
};

}}} // namespace boost::python::detail

#endif // DECREF_GUARD_DWA20021220_HPP
