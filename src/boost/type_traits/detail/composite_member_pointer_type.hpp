#ifndef BOOST_TYPE_TRAITS_DETAIL_COMPOSITE_MEMBER_POINTER_TYPE_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_DETAIL_COMPOSITE_MEMBER_POINTER_TYPE_HPP_INCLUDED

//
//  Copyright 2015 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/type_traits/detail/composite_pointer_type.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/config.hpp>
#include <cstddef>

namespace boost
{

namespace type_traits_detail
{

template<class T, class U> struct composite_member_pointer_type;

// nullptr_t

#if !defined( BOOST_NO_CXX11_NULLPTR )

#if !defined( BOOST_NO_CXX11_DECLTYPE ) && ( ( defined( __clang__ ) && !defined( _LIBCPP_VERSION ) ) || defined( __INTEL_COMPILER ) )

template<class C, class T> struct composite_member_pointer_type<T C::*, decltype(nullptr)>
{
    typedef T C::* type;
};

template<class C, class T> struct composite_member_pointer_type<decltype(nullptr), T C::*>
{
    typedef T C::* type;
};

template<> struct composite_member_pointer_type<decltype(nullptr), decltype(nullptr)>
{
    typedef decltype(nullptr) type;
};

#else

template<class C, class T> struct composite_member_pointer_type<T C::*, std::nullptr_t>
{
    typedef T C::* type;
};

template<class C, class T> struct composite_member_pointer_type<std::nullptr_t, T C::*>
{
    typedef T C::* type;
};

template<> struct composite_member_pointer_type<std::nullptr_t, std::nullptr_t>
{
    typedef std::nullptr_t type;
};

#endif

#endif // !defined( BOOST_NO_CXX11_NULLPTR )

template<class C1, class C2> struct common_member_class;

template<class C> struct common_member_class<C, C>
{
    typedef C type;
};

template<class C1, class C2> struct common_member_class
{
    typedef typename boost::conditional<

        boost::is_base_of<C1, C2>::value,
        C2,
        typename boost::conditional<boost::is_base_of<C2, C1>::value, C1, void>::type

    >::type type;
};

//This indirection avoids compilation errors on some older 
//compilers like MSVC 7.1
template<class CT, class CB>
struct common_member_class_pointer_to_member
{
    typedef CT CB::* type;
};

template<class C1, class T1, class C2, class T2> struct composite_member_pointer_type<T1 C1::*, T2 C2::*>
{
private:

    typedef typename composite_pointer_type<T1*, T2*>::type CPT;
    typedef typename boost::remove_pointer<CPT>::type CT;

    typedef typename common_member_class<C1, C2>::type CB;

public:

    typedef typename common_member_class_pointer_to_member<CT, CB>::type type;
};

} // namespace type_traits_detail

} // namespace boost

#endif // #ifndef BOOST_TYPE_TRAITS_DETAIL_COMPOSITE_MEMBER_POINTER_TYPE_HPP_INCLUDED
