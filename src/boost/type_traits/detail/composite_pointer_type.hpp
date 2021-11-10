#ifndef BOOST_TYPE_TRAITS_DETAIL_COMPOSITE_POINTER_TYPE_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_DETAIL_COMPOSITE_POINTER_TYPE_HPP_INCLUDED

//
//  Copyright 2015 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/type_traits/copy_cv.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/config.hpp>
#include <cstddef>

namespace boost
{

namespace type_traits_detail
{

template<class T, class U> struct composite_pointer_type;

// same type

template<class T> struct composite_pointer_type<T*, T*>
{
    typedef T* type;
};

// nullptr_t

#if !defined( BOOST_NO_CXX11_NULLPTR )

#if !defined( BOOST_NO_CXX11_DECLTYPE ) && ( ( defined( __clang__ ) && !defined( _LIBCPP_VERSION ) ) || defined( __INTEL_COMPILER ) )

template<class T> struct composite_pointer_type<T*, decltype(nullptr)>
{
    typedef T* type;
};

template<class T> struct composite_pointer_type<decltype(nullptr), T*>
{
    typedef T* type;
};

template<> struct composite_pointer_type<decltype(nullptr), decltype(nullptr)>
{
    typedef decltype(nullptr) type;
};

#else

template<class T> struct composite_pointer_type<T*, std::nullptr_t>
{
    typedef T* type;
};

template<class T> struct composite_pointer_type<std::nullptr_t, T*>
{
    typedef T* type;
};

template<> struct composite_pointer_type<std::nullptr_t, std::nullptr_t>
{
    typedef std::nullptr_t type;
};

#endif

#endif // !defined( BOOST_NO_CXX11_NULLPTR )

namespace detail
{

template<class T, class U> struct has_common_pointee
{
private:

    typedef typename boost::remove_cv<T>::type T2;
    typedef typename boost::remove_cv<U>::type U2;

public:

    BOOST_STATIC_CONSTANT( bool, value =
        (boost::is_same<T2, U2>::value)
        || boost::is_void<T2>::value
        || boost::is_void<U2>::value
        || (boost::is_base_of<T2, U2>::value)
        || (boost::is_base_of<U2, T2>::value) );
};

template<class T, class U> struct common_pointee
{
private:

    typedef typename boost::remove_cv<T>::type T2;
    typedef typename boost::remove_cv<U>::type U2;

public:

    typedef typename boost::conditional<

        boost::is_same<T2, U2>::value || boost::is_void<T2>::value || boost::is_base_of<T2, U2>::value,
        typename boost::copy_cv<T, U>::type,
        typename boost::copy_cv<U, T>::type

    >::type type;
};

template<class T, class U> struct composite_pointer_impl
{
private:

    typedef typename boost::remove_cv<T>::type T2;
    typedef typename boost::remove_cv<U>::type U2;

public:

    typedef typename boost::copy_cv<typename boost::copy_cv<typename composite_pointer_type<T2, U2>::type const, T>::type, U>::type type;
};

//Old compilers like MSVC-7.1 have problems using boost::conditional in
//composite_pointer_type. Partially specializing on has_common_pointee<T, U>::value
//seems to make their life easier
template<class T, class U, bool = has_common_pointee<T, U>::value >
struct composite_pointer_type_dispatch
   : common_pointee<T, U>
{};

template<class T, class U>
struct composite_pointer_type_dispatch<T, U, false>
   : composite_pointer_impl<T, U>
{};


} // detail


template<class T, class U> struct composite_pointer_type<T*, U*>
{
    typedef typename detail::composite_pointer_type_dispatch<T, U>::type* type;
};

} // namespace type_traits_detail

} // namespace boost

#endif // #ifndef BOOST_TYPE_TRAITS_DETAIL_COMPOSITE_POINTER_TYPE_HPP_INCLUDED
