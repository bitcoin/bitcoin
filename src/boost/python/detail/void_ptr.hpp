// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef VOID_PTR_DWA200239_HPP
# define VOID_PTR_DWA200239_HPP

# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python { namespace detail { 

template <class U>
inline U& void_ptr_to_reference(void const volatile* p, U&(*)())
{
    return *(U*)p;
}

template <class T>
inline void write_void_ptr(void const volatile* storage, void* ptr, T*)
{
    *(T**)storage = (T*)ptr;
}

// writes U(ptr) into the storage
template <class U>
inline void write_void_ptr_reference(void const volatile* storage, void* ptr, U&(*)())
{
    // stripping CV qualification suppresses warnings on older EDGs
    typedef typename remove_cv<U>::type u_stripped; 
    write_void_ptr(storage, ptr, u_stripped(0));
}

}}} // namespace boost::python::detail

#endif // VOID_PTR_DWA200239_HPP
