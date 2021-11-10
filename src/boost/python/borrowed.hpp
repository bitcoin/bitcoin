// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BORROWED_DWA2002614_HPP
# define BORROWED_DWA2002614_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/borrowed_ptr.hpp>

namespace boost { namespace python { 

template <class T>
inline python::detail::borrowed<T>* borrowed(T* p)
{
    return (detail::borrowed<T>*)p;
}
    
}} // namespace boost::python

#endif // BORROWED_DWA2002614_HPP
