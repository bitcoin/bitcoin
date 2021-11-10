// Copyright Gottfried Ganßauge 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef BOOST_PYTHON_DETAIL_DEALLOC_HPP_
# define BOOST_PYTHON_DETAIL_DEALLOC_HPP_
namespace boost { namespace python { namespace detail {
    extern "C"
    {
        inline void dealloc(PyObject* self)
        {
          PyObject_Del(self);
        }
    }
}}} // namespace boost::python::detail
# endif    // BOOST_PYTHON_DETAIL_DEALLOC_HPP_
