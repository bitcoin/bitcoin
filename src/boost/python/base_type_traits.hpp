// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BASE_TYPE_TRAITS_DWA2002614_HPP
# define BASE_TYPE_TRAITS_DWA2002614_HPP

# include <boost/python/detail/prefix.hpp>

namespace boost { namespace python { 

namespace detail
{
  struct unspecialized {};
}

// Derive from unspecialized so we can detect whether traits are
// specialized
template <class T> struct base_type_traits
  : detail::unspecialized
{};

template <>
struct base_type_traits<PyObject>
{
    typedef PyObject type;
};

template <>
struct base_type_traits<PyTypeObject>
{
    typedef PyObject type;
};

template <>
struct base_type_traits<PyMethodObject>
{
    typedef PyObject type;
};

}} // namespace boost::python

#endif // BASE_TYPE_TRAITS_DWA2002614_HPP
