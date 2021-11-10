#ifndef BOOST_TYPE_TRAITS_IS_LIST_CONSTRUCTIBLE_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_IS_LIST_CONSTRUCTIBLE_HPP_INCLUDED

//  Copyright 2017 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/declval.hpp>
#include <boost/type_traits/is_complete.hpp>
#include <boost/static_assert.hpp>

namespace boost
{

#if defined(BOOST_NO_SFINAE_EXPR) || defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_DECLTYPE) \
   || defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX) || defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)\
   || BOOST_WORKAROUND(BOOST_GCC, < 40700)

template<class T, class = void, class = void, class = void, class = void, class = void, class = void> struct is_list_constructible: false_type
{
   BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_list_constructible must be complete types");
};

#else

namespace type_traits_detail
{

template<class T, class... A, class = decltype( T{declval<A>()...} )> true_type is_list_constructible_impl( int );
template<class T, class... A> false_type is_list_constructible_impl( ... );

} // namespace type_traits_detail

template<class T, class... A> struct is_list_constructible: decltype( type_traits_detail::is_list_constructible_impl<T, A...>(0) )
{
   BOOST_STATIC_ASSERT_MSG(boost::is_complete<T>::value, "Arguments to is_list_constructible must be complete types");
};

#endif

} // namespace boost

#endif // #ifndef BOOST_TYPE_TRAITS_IS_LIST_CONSTRUCTIBLE_HPP_INCLUDED
