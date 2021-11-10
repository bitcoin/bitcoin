// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef UNWRAP_TYPE_ID_DWA2004722_HPP
# define UNWRAP_TYPE_ID_DWA2004722_HPP

# include <boost/python/type_id.hpp>

# include <boost/mpl/bool.hpp>

namespace boost { namespace python {

template <class T> class wrapper;

namespace detail { 

template <class T>
inline type_info unwrap_type_id(T*, ...)
{
    return type_id<T>();
}

template <class U, class T>
inline type_info unwrap_type_id(U*, wrapper<T>*)
{
    return type_id<T>();
}

}}} // namespace boost::python::detail

#endif // UNWRAP_TYPE_ID_DWA2004722_HPP
