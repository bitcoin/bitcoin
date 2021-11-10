// Copyright Cromwell D. Enage 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_PARENTHESIZED_RETURN_TYPE_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_PARENTHESIZED_RETURN_TYPE_HPP

namespace boost { namespace parameter { namespace aux {

    // A metafunction that transforms void(*)(T) -> identity<T>
    template <typename UnaryFunctionPointer>
    struct unaryfunptr_return_type;
}}} // namespace boost::parameter::aux

#include <boost/parameter/config.hpp>

#if defined(BOOST_PARAMETER_CAN_USE_MP11)
#include <boost/mp11/utility.hpp>
#else
#include <boost/mpl/identity.hpp>
#endif

namespace boost { namespace parameter { namespace aux {

    template <typename Arg>
    struct unaryfunptr_return_type<void(*)(Arg)>
    {
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using type = ::boost::mp11::mp_identity<Arg>;
#else
        typedef ::boost::mpl::identity<Arg> type;
#endif
    };

    template <>
    struct unaryfunptr_return_type<void(*)(void)>
    {
#if defined(BOOST_PARAMETER_CAN_USE_MP11)
        using type = ::boost::mp11::mp_identity<void>;
#else
        typedef ::boost::mpl::identity<void> type;
#endif
    };
}}} // namespace boost::parameter::aux

#if !defined(BOOST_NO_SFINAE)
#include <boost/core/enable_if.hpp>

namespace boost { namespace parameter { namespace aux {

    template <typename Pred, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::enable_if<Pred,Ret>)>
    {
        typedef ::boost::enable_if<Pred,Ret> type;
    };

    template <bool b, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::enable_if_c<b,Ret>)>
    {
        typedef ::boost::enable_if_c<b,Ret> type;
    };

    template <typename Pred, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::lazy_enable_if<Pred,Ret>)>
    {
        typedef ::boost::lazy_enable_if<Pred,Ret> type;
    };

    template <bool b, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::lazy_enable_if_c<b,Ret>)>
    {
        typedef ::boost::lazy_enable_if_c<b,Ret> type;
    };

    template <typename Pred, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::disable_if<Pred,Ret>)>
    {
        typedef ::boost::disable_if<Pred,Ret> type;
    };

    template <bool b, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::disable_if_c<b,Ret>)>
    {
        typedef ::boost::disable_if_c<b,Ret> type;
    };

    template <typename B, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::lazy_disable_if<B,Ret>)>
    {
        typedef ::boost::lazy_disable_if<B,Ret> type;
    };

    template <bool b, typename Ret>
    struct unaryfunptr_return_type<void(*)(::boost::lazy_disable_if_c<b,Ret>)>
    {
        typedef ::boost::lazy_disable_if_c<b,Ret> type;
    };
}}} // namespace boost::parameter::aux

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <type_traits>

namespace boost { namespace parameter { namespace aux {

    template <bool b, typename Ret>
    struct unaryfunptr_return_type<void(*)(::std::enable_if<b,Ret>)>
    {
        typedef ::std::enable_if<b,Ret> type;
    };
}}} // namespace boost::parameter::aux

#endif  // BOOST_NO_CXX11_HDR_TYPE_TRAITS
#endif  // BOOST_NO_SFINAE

// A macro that takes a parenthesized C++ type name (T) and transforms it
// into an un-parenthesized type expression equivalent to identity<T>.
#define BOOST_PARAMETER_PARENTHESIZED_RETURN_TYPE(x)                         \
    ::boost::parameter::aux::unaryfunptr_return_type< void(*)x >::type

#endif  // include guard

