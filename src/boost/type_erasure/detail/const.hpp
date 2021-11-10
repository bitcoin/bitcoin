// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_CONST_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_CONST_HPP_INCLUDED

#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_erasure/placeholder_of.hpp>
#include <boost/type_erasure/derived.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

template<class T>
struct is_non_const_ref : boost::mpl::false_ {};
template<class T>
struct is_non_const_ref<T&> : boost::mpl::true_ {};
template<class T>
struct is_non_const_ref<const T&> : boost::mpl::false_ {};

template<class Placeholder, class Base>
struct should_be_const :
    ::boost::mpl::or_<
        ::boost::is_const<Placeholder>,
        ::boost::type_erasure::detail::is_non_const_ref<
            typename ::boost::type_erasure::placeholder_of<Base>::type
        >
    >
{};

template<class Placeholder, class Base>
struct should_be_non_const :
    ::boost::mpl::and_<
        ::boost::mpl::not_< ::boost::is_const<Placeholder> >,
        ::boost::mpl::not_<
            ::boost::is_reference<
                typename ::boost::type_erasure::placeholder_of<Base>::type
            >
        >
    >
{};

template<class Base>
struct non_const_this_param
{
    typedef typename ::boost::type_erasure::placeholder_of<Base>::type placeholder;
    typedef typename ::boost::type_erasure::derived<Base>::type plain_type;
    typedef typename ::boost::mpl::if_<
        ::boost::is_same<
            placeholder,
            typename ::boost::remove_cv<
                typename ::boost::remove_reference<placeholder>::type
            >::type&
        >,
        const plain_type,
        plain_type
    >::type type;
};

template<class T>
struct uncallable {};

template<class Placeholder, class Base>
struct maybe_const_this_param
{
    typedef typename ::boost::type_erasure::derived<Base>::type plain_type;
    typedef typename ::boost::remove_reference<Placeholder>::type plain_placeholder;
    typedef typename ::boost::mpl::if_< ::boost::is_reference<Placeholder>,
        typename ::boost::mpl::if_<
            ::boost::type_erasure::detail::should_be_non_const<plain_placeholder, Base>,
            plain_type&,
            typename ::boost::mpl::if_<
                ::boost::type_erasure::detail::should_be_const<plain_placeholder, Base>,
                const plain_type&,
                uncallable<plain_type>
            >::type
        >::type,
        plain_type
    >::type type;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class Placeholder, class Base>
struct maybe_const_this_param<Placeholder&&, Base>
{
    typedef typename ::boost::type_erasure::derived<Base>::type plain_type;
    typedef typename ::boost::remove_reference<Placeholder>::type plain_placeholder;
    typedef typename ::boost::type_erasure::placeholder_of<plain_type>::type self_placeholder;
    typedef typename ::boost::mpl::if_< ::boost::is_lvalue_reference<self_placeholder>,
        ::boost::type_erasure::detail::uncallable<plain_type>,
        typename ::boost::mpl::if_< ::boost::is_rvalue_reference<self_placeholder>,
            const plain_type&,
            plain_type&&
        >::type
    >::type type;
};

#endif

}
}
}

#endif
