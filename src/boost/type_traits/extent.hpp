
//  (C) Copyright John Maddock 2005.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_EXTENT_HPP_INCLUDED
#define BOOST_TT_EXTENT_HPP_INCLUDED

#include <cstddef> // size_t
#include <boost/type_traits/integral_constant.hpp>
#include <boost/detail/workaround.hpp>

namespace boost {

namespace detail{

#if defined( BOOST_CODEGEARC )
    // wrap the impl as main trait provides additional MPL lambda support
    template < typename T, std::size_t N >
    struct extent_imp {
        static const std::size_t value = __array_extent(T, N);
    };

#else

template <class T, std::size_t N>
struct extent_imp
{
   BOOST_STATIC_CONSTANT(std::size_t, value = 0);
};
#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template <class T, std::size_t R, std::size_t N>
struct extent_imp<T[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};

template <class T, std::size_t R, std::size_t N>
struct extent_imp<T const[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};

template <class T, std::size_t R, std::size_t N>
struct extent_imp<T volatile[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};

template <class T, std::size_t R, std::size_t N>
struct extent_imp<T const volatile[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};

template <class T, std::size_t R>
struct extent_imp<T[R],0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = R);
};

template <class T, std::size_t R>
struct extent_imp<T const[R], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = R);
};

template <class T, std::size_t R>
struct extent_imp<T volatile[R], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = R);
};

template <class T, std::size_t R>
struct extent_imp<T const volatile[R], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = R);
};

#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600) && !defined(__IBMCPP__) &&  !BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840)) && !defined(__MWERKS__)
template <class T, std::size_t N>
struct extent_imp<T[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};
template <class T, std::size_t N>
struct extent_imp<T const[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};
template <class T, std::size_t N>
struct extent_imp<T volatile[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};
template <class T, std::size_t N>
struct extent_imp<T const volatile[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::extent_imp<T, N-1>::value));
};
template <class T>
struct extent_imp<T[], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = 0);
};
template <class T>
struct extent_imp<T const[], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = 0);
};
template <class T>
struct extent_imp<T volatile[], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = 0);
};
template <class T>
struct extent_imp<T const volatile[], 0>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = 0);
};
#endif
#endif

#endif  // non-CodeGear implementation
}   // ::boost::detail

template <class T, std::size_t N = 0>
struct extent
   : public ::boost::integral_constant<std::size_t, ::boost::detail::extent_imp<T,N>::value>
{
};

} // namespace boost

#endif // BOOST_TT_IS_MEMBER_FUNCTION_POINTER_HPP_INCLUDED
