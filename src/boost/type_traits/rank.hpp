
//  (C) Copyright John Maddock 2005.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_RANK_HPP_INCLUDED
#define BOOST_TT_RANK_HPP_INCLUDED

#include <cstddef> // size_t
#include <boost/type_traits/integral_constant.hpp>

namespace boost {

#if !defined( BOOST_CODEGEARC )

namespace detail{

template <class T, std::size_t N>
struct rank_imp
{
   BOOST_STATIC_CONSTANT(std::size_t, value = N);
};
#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template <class T, std::size_t R, std::size_t N>
struct rank_imp<T[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};

template <class T, std::size_t R, std::size_t N>
struct rank_imp<T const[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};

template <class T, std::size_t R, std::size_t N>
struct rank_imp<T volatile[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};

template <class T, std::size_t R, std::size_t N>
struct rank_imp<T const volatile[R], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};

#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600) && !defined(__IBMCPP__) &&  !BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840))
template <class T, std::size_t N>
struct rank_imp<T[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};
template <class T, std::size_t N>
struct rank_imp<T const[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};
template <class T, std::size_t N>
struct rank_imp<T volatile[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};
template <class T, std::size_t N>
struct rank_imp<T const volatile[], N>
{
   BOOST_STATIC_CONSTANT(std::size_t, value = (::boost::detail::rank_imp<T, N+1>::value));
};
#endif
#endif
}

#endif // !defined( BOOST_CODEGEARC )

#if defined( BOOST_CODEGEARC )
template <class T> struct rank : public integral_constant<std::size_t, __array_rank(T)>{};
#else
template <class T> struct rank : public integral_constant<std::size_t, (::boost::detail::rank_imp<T, 0>::value)>{};
#endif

} // namespace boost

#endif // BOOST_TT_IS_MEMBER_FUNCTION_POINTER_HPP_INCLUDED
