//  Copyright (c) 2001-2012 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_INT_FEB_23_2007_0840PM)
#define BOOST_SPIRIT_KARMA_INT_FEB_23_2007_0840PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/limits.hpp>
#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/numeric_traits.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/detail/get_encoding.hpp>
#include <boost/spirit/home/support/detail/is_spirit_tag.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/get_casetag.hpp>
#include <boost/spirit/home/karma/detail/extract_from.hpp>
#include <boost/spirit/home/karma/detail/enable_lit.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/numeric/detail/numeric_utils.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/fusion/include/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    namespace tag
    {
        template <typename T, unsigned Radix, bool force_sign>
        struct int_generator
        {
            BOOST_SPIRIT_IS_TAG()
        };
    }

    namespace karma
    {
        ///////////////////////////////////////////////////////////////////////
        // This one is the class that the user can instantiate directly in
        // order to create a customized int generator
        template <typename T = int, unsigned Radix = 10, bool force_sign = false>
        struct int_generator
          : spirit::terminal<tag::int_generator<T, Radix, force_sign> >
        {};
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, tag::short_>    // enables short_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::int_>      // enables int_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::long_>     // enables long_
      : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <>
    struct use_terminal<karma::domain, tag::long_long> // enables long_long
      : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, short>    // enables lit(short(0))
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, int>      // enables lit(0)
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, long>     // enables lit(0L)
      : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <>
    struct use_terminal<karma::domain, boost::long_long_type> // enables lit(0LL)
      : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <typename A0>
    struct use_terminal<karma::domain         // enables short_(...)
      , terminal_ex<tag::short_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain         // enables int_(...)
      , terminal_ex<tag::int_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain         // enables long_(...)
      , terminal_ex<tag::long_, fusion::vector1<A0> >
    > : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename A0>
    struct use_terminal<karma::domain         // enables long_long(...)
      , terminal_ex<tag::long_long, fusion::vector1<A0> >
    > : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <>                               // enables *lazy* short_(...)
    struct use_lazy_terminal<karma::domain, tag::short_, 1>
      : mpl::true_ {};

    template <>                               // enables *lazy* int_(...)
    struct use_lazy_terminal<karma::domain, tag::int_, 1>
      : mpl::true_ {};

    template <>                               // enables *lazy* long_(...)
    struct use_lazy_terminal<karma::domain, tag::long_, 1>
      : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <>                               // enables *lazy* long_long(...)
    struct use_lazy_terminal<karma::domain, tag::long_long, 1>
      : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    // enables any custom int_generator
    template <typename T, unsigned Radix, bool force_sign>
    struct use_terminal<karma::domain, tag::int_generator<T, Radix, force_sign> >
      : mpl::true_ {};

    // enables any custom int_generator(...)
    template <typename T, unsigned Radix, bool force_sign, typename A0>
    struct use_terminal<karma::domain
      , terminal_ex<tag::int_generator<T, Radix, force_sign>
                  , fusion::vector1<A0> >
    > : mpl::true_ {};

    // enables *lazy* custom int_generator
    template <typename T, unsigned Radix, bool force_sign>
    struct use_lazy_terminal<
        karma::domain
      , tag::int_generator<T, Radix, force_sign>
      , 1 // arity
    > : mpl::true_ {};

    // enables lit(int)
    template <typename A0>
    struct use_terminal<karma::domain
          , terminal_ex<tag::lit, fusion::vector1<A0> >
          , typename enable_if<traits::is_int<A0> >::type>
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::short_;
    using spirit::int_;
    using spirit::long_;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::long_long;
#endif
    using spirit::lit;    // lit(1) is equivalent to 1
#endif

    using spirit::short_type;
    using spirit::int_type;
    using spirit::long_type;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::long_long_type;
#endif

    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization is used for int generators not having a direct
    //  initializer: int_, long_ etc. These generators must be used in
    //  conjunction with an Attribute.
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T, typename CharEncoding, typename Tag, unsigned Radix
      , bool force_sign>
    struct any_int_generator
      : primitive_generator<any_int_generator<T, CharEncoding, Tag, Radix
          , force_sign> >
    {
    private:
        template <typename OutputIterator, typename Attribute>
        static bool insert_int(OutputIterator& sink, Attribute const& attr)
        {
            return sign_inserter::call(sink, traits::test_zero(attr)
                      , traits::test_negative(attr), force_sign) &&
                   int_inserter<Radix, CharEncoding, Tag>::call(sink
                      , traits::get_absolute_value(attr));
        }

    public:
        template <typename Context, typename Unused>
        struct attribute
        {
            typedef T type;
        };

        // check template Attribute 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

        BOOST_SPIRIT_ASSERT_MSG(std::numeric_limits<T>::is_signed,
            signed_unsigned_mismatch, ());

        // int has a Attribute attached
        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        static bool
        generate(OutputIterator& sink, Context& context, Delimiter const& d
          , Attribute const& attr)
        {
            if (!traits::has_optional_value(attr))
                return false;       // fail if it's an uninitialized optional

            return insert_int(sink, traits::extract_from<T>(attr, context)) &&
                   delimit_out(sink, d);      // always do post-delimiting
        }

        // this int has no Attribute attached, it needs to have been
        // initialized from a direct literal
        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        generate(OutputIterator&, Context&, Delimiter const&, unused_type)
        {
            // It is not possible (doesn't make sense) to use numeric generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator, int_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("integer");
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization is used for int generators having a direct
    //  initializer: int_(10), long_(20) etc.
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T, typename CharEncoding, typename Tag, unsigned Radix
      , bool force_sign, bool no_attribute>
    struct literal_int_generator
      : primitive_generator<literal_int_generator<T, CharEncoding, Tag, Radix
          , force_sign, no_attribute> >
    {
    private:
        template <typename OutputIterator, typename Attribute>
        static bool insert_int(OutputIterator& sink, Attribute const& attr)
        {
            return sign_inserter::call(sink, traits::test_zero(attr)
                      , traits::test_negative(attr), force_sign) &&
                   int_inserter<Radix, CharEncoding, Tag>::call(sink
                      , traits::get_absolute_value(attr));
        }

    public:
        template <typename Context, typename Unused = unused_type>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, T>
        {};

        literal_int_generator(typename add_const<T>::type n)
          : n_(n) {}

        // check template Attribute 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

        BOOST_SPIRIT_ASSERT_MSG(std::numeric_limits<T>::is_signed,
            signed_unsigned_mismatch, ());

        // A int_(1) which additionally has an associated attribute emits
        // its immediate literal only if it matches the attribute, otherwise
        // it fails.
        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const& attr) const
        {
            typedef typename attribute<Context>::type attribute_type;
            if (!traits::has_optional_value(attr) ||
                n_ != traits::extract_from<attribute_type>(attr, context))
            {
                return false;
            }
            return insert_int(sink, n_) && delimit_out(sink, d);
        }

        // A int_(1) without any associated attribute just emits its
        // immediate literal
        template <typename OutputIterator, typename Context, typename Delimiter>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , unused_type) const
        {
            return insert_int(sink, n_) && delimit_out(sink, d);
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("integer");
        }

        T n_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T, typename Modifiers, unsigned Radix = 10
          , bool force_sign = false>
        struct make_int
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef any_int_generator<
                T
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , Radix
              , force_sign
            > result_type;

            result_type operator()(unused_type, unused_type) const
            {
                return result_type();
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::short_, Modifiers>
      : detail::make_int<short, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<tag::int_, Modifiers>
      : detail::make_int<int, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<tag::long_, Modifiers>
      : detail::make_int<long, Modifiers> {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers>
    struct make_primitive<tag::long_long, Modifiers>
      : detail::make_int<boost::long_long_type, Modifiers> {};
#endif

    template <typename T, unsigned Radix, bool force_sign, typename Modifiers>
    struct make_primitive<tag::int_generator<T, Radix, force_sign>, Modifiers>
      : detail::make_int<typename remove_const<T>::type
          , Modifiers, Radix, force_sign> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T, typename Modifiers, unsigned Radix = 10
          , bool force_sign = false>
        struct make_int_direct
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_int_generator<
                T
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , Radix, force_sign, false
            > result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                return result_type(fusion::at_c<0>(term.args));
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::short_, fusion::vector1<A0> >, Modifiers>
      : detail::make_int_direct<short, Modifiers> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::int_, fusion::vector1<A0> >, Modifiers>
      : detail::make_int_direct<int, Modifiers> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::long_, fusion::vector1<A0> >, Modifiers>
      : detail::make_int_direct<long, Modifiers> {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::long_long, fusion::vector1<A0> >, Modifiers>
      : detail::make_int_direct<boost::long_long_type, Modifiers> {};
#endif

    template <typename T, unsigned Radix, bool force_sign, typename A0
      , typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::int_generator<T, Radix, force_sign>
          , fusion::vector1<A0> >, Modifiers>
      : detail::make_int_direct<typename remove_const<T>::type
          , Modifiers, Radix, force_sign> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T, typename Modifiers>
        struct basic_int_literal
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_int_generator<
                T
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , 10, false, true
            > result_type;

            template <typename T_>
            result_type operator()(T_ i, unused_type) const
            {
                return result_type(i);
            }
        };
    }

    template <typename Modifiers>
    struct make_primitive<short, Modifiers>
      : detail::basic_int_literal<short, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<int, Modifiers>
      : detail::basic_int_literal<int, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<long, Modifiers>
      : detail::basic_int_literal<long, Modifiers> {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers>
    struct make_primitive<boost::long_long_type, Modifiers>
      : detail::basic_int_literal<boost::long_long_type, Modifiers> {};
#endif

    // lit(int)
    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::lit, fusion::vector1<A0> >
          , Modifiers
          , typename enable_if<traits::is_int<A0> >::type>
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef literal_int_generator<
            typename remove_const<A0>::type
          , typename spirit::detail::get_encoding_with_case<
                Modifiers, unused_type, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
          , 10, false, true
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

#endif
