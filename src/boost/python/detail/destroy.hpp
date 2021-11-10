// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef DESTROY_DWA2002221_HPP
# define DESTROY_DWA2002221_HPP

# include <boost/python/detail/type_traits.hpp>
# include <boost/detail/workaround.hpp>
namespace boost { namespace python { namespace detail { 

template <bool array> struct value_destroyer;
    
template <>
struct value_destroyer<false>
{
    template <class T>
    static void execute(T const volatile* p)
    {
        p->~T();
    }
};

template <>
struct value_destroyer<true>
{
    template <class A, class T>
    static void execute(A*, T const volatile* const first)
    {
        for (T const volatile* p = first; p != first + sizeof(A)/sizeof(T); ++p)
        {
            value_destroyer<
                is_array<T>::value
            >::execute(p);
        }
    }
    
    template <class T>
    static void execute(T const volatile* p)
    {
        execute(p, *p);
    }
};

template <class T>
inline void destroy_referent_impl(void* p, T& (*)())
{
    // note: cv-qualification needed for MSVC6
    // must come *before* T for metrowerks
    value_destroyer<
         (is_array<T>::value)
    >::execute((const volatile T*)p);
}

template <class T>
inline void destroy_referent(void* p, T(*)() = 0)
{
    destroy_referent_impl(p, (T(*)())0);
}

}}} // namespace boost::python::detail

#endif // DESTROY_DWA2002221_HPP
