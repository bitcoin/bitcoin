//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of Dragon Systems, Inc., in
//  producing this work.

#ifndef NONE_DWA_052000_H_
# define NONE_DWA_052000_H_

# include <boost/python/detail/prefix.hpp>

namespace boost { namespace python { namespace detail {

inline PyObject* none() { Py_INCREF(Py_None); return Py_None; }
    
}}} // namespace boost::python::detail

#endif // NONE_DWA_052000_H_
