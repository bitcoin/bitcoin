//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_META_CREATE_NOV_21_2009_0425PM)
#define BOOST_SPIRIT_KARMA_META_CREATE_NOV_21_2009_0425PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/auto/meta_create.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/config.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/proto/tags.hpp>
#include <boost/type_traits/is_same.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    // compatible STL containers
    template <typename Container>
    struct meta_create_container
    {
        typedef make_unary_proto_expr<
            typename Container::value_type
          , proto::tag::dereference, karma::domain
        > make_proto_expr;

        typedef typename make_proto_expr::type type;

        static type call()
        {
            return make_proto_expr::call();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // String types
    template <typename String>
    struct meta_create_string
    {
        typedef spirit::standard::string_type type;
        static type const call() { return type(); }
    };

    template <>
    struct meta_create_string<wchar_t*>
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    template <>
    struct meta_create_string<wchar_t const*>
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    template <int N>
    struct meta_create_string<wchar_t[N]>
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    template <int N>
    struct meta_create_string<wchar_t const[N]>
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    template <int N>
    struct meta_create_string<wchar_t(&)[N]>
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    template <int N>
    struct meta_create_string<wchar_t const(&)[N]>
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    template <typename Traits, typename Allocator>
    struct meta_create_string<std::basic_string<wchar_t, Traits, Allocator> >
    {
        typedef spirit::standard_wide::string_type type;
        static type const call() { return type(); }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Fusion sequences
    template <typename Sequence>
    struct meta_create_sequence
    {
        // create a mpl sequence from the given fusion sequence
        typedef typename mpl::fold<
            typename fusion::result_of::as_vector<Sequence>::type
          , mpl::vector<>, mpl::push_back<mpl::_, mpl::_>
        >::type sequence_type;

        typedef make_nary_proto_expr<
            sequence_type, proto::tag::shift_left, karma::domain
        > make_proto_expr;

        typedef typename make_proto_expr::type type;

        static type call()
        {
            return make_proto_expr::call();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // the default is to use the standard streaming operator unless it's a
    // STL container or a fusion sequence

    // The default implementation will be chosen if no predefined mapping of
    // the data type T to a Karma component is defined.
    struct no_auto_mapping_exists {};

    template <typename T, typename Enable = void>
    struct meta_create_impl : mpl::identity<no_auto_mapping_exists> {};

    template <typename T>
    struct meta_create_impl<T
          , typename enable_if<
                mpl::and_<
                    traits::is_container<T>
                  , mpl::not_<traits::is_string<T> >
                  , mpl::not_<fusion::traits::is_sequence<T> >
                > >::type>
      : meta_create_container<T> {};

    template <typename T>
    struct meta_create_impl<T
          , typename enable_if<traits::is_string<T> >::type>
      : meta_create_string<T> {};

    template <typename T>
    struct meta_create_impl<T, typename enable_if<
                spirit::detail::is_fusion_sequence_but_not_proto_expr<T>
            >::type>
      : meta_create_sequence<T> {};

    template <typename T, typename Enable = void>
    struct meta_create : meta_create_impl<T> {};

    ///////////////////////////////////////////////////////////////////////////
    // optional
    template <typename T>
    struct meta_create<boost::optional<T> >
    {
        typedef make_unary_proto_expr<
            T, proto::tag::negate, karma::domain
        > make_proto_expr;

        typedef typename make_proto_expr::type type;

        static type call()
        {
            return make_proto_expr::call();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // alternatives
    template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct meta_create<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
    {
        typedef make_nary_proto_expr<
            typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types
          , proto::tag::bitwise_or, karma::domain
        > make_proto_expr;

        typedef typename make_proto_expr::type type;

        static type call()
        {
            return make_proto_expr::call();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // predefined specializations for primitive components

    // character generator
    template <>
    struct meta_create<char>
    {
        typedef spirit::standard::char_type type;
        static type const call() { return type(); }
    };
    template <>
    struct meta_create<signed char>
    {
        typedef spirit::standard::char_type type;
        static type const call() { return type(); }
    };
    template <>
    struct meta_create<wchar_t>
    {
        typedef spirit::standard_wide::char_type type;
        static type const call() { return type(); }
    };

    template <>
    struct meta_create<unsigned char>
    {
        typedef spirit::standard::char_type type;
        static type const call() { return type(); }
    };

    // boolean generator
    template <>
    struct meta_create<bool>
    {
        typedef spirit::bool_type type;
        static type call() { return type(); }
    };

    // integral generators
    template <>
    struct meta_create<int>
    {
        typedef spirit::int_type type;
        static type call() { return type(); }
    };
    template <>
    struct meta_create<short>
    {
        typedef spirit::short_type type;
        static type call() { return type(); }
    };
    template <>
    struct meta_create<long>
    {
        typedef spirit::long_type type;
        static type call() { return type(); }
    };
    template <>
    struct meta_create<unsigned int>
    {
        typedef spirit::uint_type type;
        static type call() { return type(); }
    };
#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    template <>
    struct meta_create<unsigned short>
    {
        typedef spirit::ushort_type type;
        static type call() { return type(); }
    };
#endif
    template <>
    struct meta_create<unsigned long>
    {
        typedef spirit::ulong_type type;
        static type call() { return type(); }
    };

#ifdef BOOST_HAS_LONG_LONG
    template <>
    struct meta_create<boost::long_long_type>
    {
        typedef spirit::long_long_type type;
        static type call() { return type(); }
    };
    template <>
    struct meta_create<boost::ulong_long_type>
    {
        typedef spirit::ulong_long_type type;
        static type call() { return type(); }
    };
#endif

    // floating point generators
    template <>
    struct meta_create<float>
    {
        typedef spirit::float_type type;
        static type call() { return type(); }
    };
    template <>
    struct meta_create<double>
    {
        typedef spirit::double_type type;
        static type call() { return type(); }
    };
    template <>
    struct meta_create<long double>
    {
        typedef spirit::long_double_type type;
        static type call() { return type(); }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // main customization point for create_generator
    template <typename T, typename Enable = void>
    struct create_generator : karma::meta_create<T> {};

    ///////////////////////////////////////////////////////////////////////////
    // dispatch this to the karma related specializations
    template <typename T>
    struct meta_create<karma::domain, T>
      : create_generator<typename spirit::detail::remove_const_ref<T>::type> {};

    ///////////////////////////////////////////////////////////////////////////
    // Check whether a valid mapping exits for the given data type to a Karma
    // component
    template <typename T>
    struct meta_create_exists<karma::domain, T>
      : mpl::not_<is_same<
            karma::no_auto_mapping_exists
          , typename meta_create<karma::domain, T>::type
        > > {};
}}}

#endif
