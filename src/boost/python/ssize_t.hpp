// Copyright Ralf W. Grosse-Kunstleve & David Abrahams 2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PYTHON_SSIZE_T_RWGK20060924_HPP
# define BOOST_PYTHON_SSIZE_T_RWGK20060924_HPP

# include <boost/python/detail/prefix.hpp>

namespace boost { namespace python {

#if PY_VERSION_HEX >= 0x02050000

typedef Py_ssize_t ssize_t;
ssize_t const ssize_t_max = PY_SSIZE_T_MAX;
ssize_t const ssize_t_min = PY_SSIZE_T_MIN;

#else

typedef int ssize_t;
ssize_t const ssize_t_max = INT_MAX;
ssize_t const ssize_t_min = INT_MIN;

#endif

}} // namespace boost::python

#endif // BOOST_PYTHON_SSIZE_T_RWGK20060924_HPP
