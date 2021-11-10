//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_REAL_FEB_26_2007_0512PM)
#define BOOST_SPIRIT_KARMA_REAL_FEB_26_2007_0512PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config/no_tr1/cmath.hpp>
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
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/char.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/get_casetag.hpp>
#include <boost/spirit/home/karma/detail/extract_from.hpp>
#include <boost/spirit/home/karma/detail/enable_lit.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/numeric/real_policies.hpp>
#include <boost/spirit/home/karma/numeric/detail/real_utils.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/fusion/include/vector.hpp>

namespace boost { namespace spirit
{
    namespace karma
    {
        ///////////////////////////////////////////////////////////////////////
        // forward declaration only
        template <typename T>
        struct real_policies;

        ///////////////////////////////////////////////////////////////////////
        // This is the class that the user can instantiate directly in
        // order to create a customized real generator
        template <typename T = double, typename Policies = real_policies<T> >
        struct real_generator
          : spirit::terminal<tag::stateful_tag<Policies, tag::double_, T> >
        {
            typedef tag::stateful_tag<Policies, tag::double_, T> tag_type;

            real_generator() {}
            real_generator(Policies const& p)
              : spirit::terminal<tag_type>(p) {}
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, tag::float_>       // enables float_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::double_>      // enables double_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::long_double>  // enables long_double
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, float>             // enables lit(1.0f)
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, double>            // enables lit(1.0)
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, long double>       // enables lit(1.0l)
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename A0>
    struct use_terminal<karma::domain                   // enables float_(...)
      , terminal_ex<tag::float_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain                   // enables double_(...)
      , terminal_ex<tag::double_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain                   // enables long_double(...)
      , terminal_ex<tag::long_double, fusion::vector1<A0> >
    > : mpl::true_ {};

    // lazy float_(...), double_(...), long_double(...)
    template <>
    struct use_lazy_terminal<karma::domain, tag::float_, 1>
      : mpl::true_ {};

    template <>
    struct use_lazy_terminal<karma::domain, tag::double_, 1>
      : mpl::true_ {};

    template <>
    struct use_lazy_terminal<karma::domain, tag::long_double, 1>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // enables custom real generator
    template <typename T, typename Policies>
    struct use_terminal<karma::domain
          , tag::stateful_tag<Policies, tag::double_, T> >
      : mpl::true_ {};

    template <typename T, typename Policies, typename A0>
    struct use_terminal<karma::domain
          , terminal_ex<tag::stateful_tag<Policies, tag::double_, T>
          , fusion::vector1<A0> > >
      : mpl::true_ {};

    // enables *lazy* custom real generator
    template <typename T, typename Policies>
    struct use_lazy_terminal<
        karma::domain
      , tag::stateful_tag<Policies, tag::double_, T>
      , 1 // arity
    > : mpl::true_ {};

    // enables lit(double)
    template <typename A0>
    struct use_terminal<karma::domain
          , terminal_ex<tag::lit, fusion::vector1<A0> >
          , typename enable_if<traits::is_real<A0> >::type>
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::float_;
    using spirit::double_;
    using spirit::long_double;
#endif

    using spirit::float_type;
    using spirit::double_type;
    using spirit::long_double_type;

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization is used for real generators not having a direct
    //  initializer: float_, double_ etc. These generators must be used in
    //  conjunction with an attribute.
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T, typename Policies, typename CharEncoding, typename Tag>
    struct any_real_generator
      : primitive_generator<any_real_generator<T, Policies, CharEncoding, Tag> >
    {
        typedef typename Policies::properties properties;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef T type;
        };

        any_real_generator(Policies const& policies = Policies())
          : p_(policies) {}

        // double_/float_/etc. has an attached attribute
        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const& attr) const
        {
            if (!traits::has_optional_value(attr))
                return false;       // fail if it's an uninitialized optional

            typedef real_inserter<T, Policies, CharEncoding, Tag> inserter_type;
            return inserter_type::call(sink, traits::extract_from<T>(attr, context), p_) &&
                   karma::delimit_out(sink, d);    // always do post-delimiting
        }

        // this double_/float_/etc. has no attribute attached, it needs to have
        // been initialized from a direct literal
        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool generate(OutputIterator&, Context&, Delimiter const&
          , unused_type)
        {
            // It is not possible (doesn't make sense) to use numeric generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator, real_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("real");
        }

        Policies p_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization is used for real generators having a direct
    //  initializer: float_(10.), double_(20.) etc.
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T, typename Policies, typename CharEncoding, typename Tag
      , bool no_attribute>
    struct literal_real_generator
      : primitive_generator<literal_real_generator<T, Policies, CharEncoding
          , Tag, no_attribute> >
    {
        typedef typename Policies::properties properties;

        template <typename Context, typename Unused = unused_type>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, T>
        {};

        literal_real_generator(typename add_const<T>::type n
              , Policies const& policies = Policies())
          : n_(n), p_(policies) {}

        // A double_(1.0) which additionally has an associated attribute emits
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

            typedef real_inserter<T, Policies, CharEncoding, Tag> inserter_type;
            return inserter_type::call(sink, n_, p_) &&
                   karma::delimit_out(sink, d);    // always do post-delimiting
        }

        // A double_(1.0) without any associated attribute just emits its
        // immediate literal
        template <typename OutputIterator, typename Context, typename Delimiter>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , unused_type) const
        {
            typedef real_inserter<T, Policies, CharEncoding, Tag> inserter_type;
            return inserter_type::call(sink, n_, p_) &&
                   karma::delimit_out(sink, d);    // always do post-delimiting
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("real");
        }

        T n_;
        Policies p_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T, typename Modifiers
          , typename Policies = real_policies<T> >
        struct make_real
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef any_real_generator<
                T, Policies
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
            > result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                typedef tag::stateful_tag<Policies, tag::double_, T> tag_type;
                using spirit::detail::get_stateful_data;
                return result_type(get_stateful_data<tag_type>::call(term));
            }
        };
    }

    template <typename Modifiers>
    struct make_primitive<tag::float_, Modifiers>
      : detail::make_real<float, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<tag::double_, Modifiers>
      : detail::make_real<double, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<tag::long_double, Modifiers>
      : detail::make_real<long double, Modifiers> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Policies, typename Modifiers>
    struct make_primitive<
            tag::stateful_tag<Policies, tag::double_, T>, Modifiers>
      : detail::make_real<typename remove_const<T>::type
          , Modifiers, Policies> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T, typename Modifiers
          , typename Policies = real_policies<T> >
        struct make_real_direct
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_real_generator<
                T, Policies
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , false
            > result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                typedef tag::stateful_tag<Policies, tag::double_, T> tag_type;
                using spirit::detail::get_stateful_data;
                return result_type(T(fusion::at_c<0>(term.args))
                  , get_stateful_data<tag_type>::call(term.term));
            }
        };
    }

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::float_, fusion::vector1<A0> >, Modifiers>
      : detail::make_real_direct<float, Modifiers> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::double_, fusion::vector1<A0> >, Modifiers>
      : detail::make_real_direct<double, Modifiers> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::long_double, fusion::vector1<A0> >, Modifiers>
      : detail::make_real_direct<long double, Modifiers> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Policies, typename A0, typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::stateful_tag<Policies, tag::double_, T>
          , fusion::vector1<A0> >
          , Modifiers>
      : detail::make_real_direct<typename remove_const<T>::type
          , Modifiers, Policies> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T, typename Modifiers>
        struct basic_real_literal
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_real_generator<
                T, real_policies<T>
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , true
            > result_type;

            template <typename T_>
            result_type operator()(T_ i, unused_type) const
            {
                return result_type(T(i));
            }
        };
    }

    template <typename Modifiers>
    struct make_primitive<float, Modifiers>
      : detail::basic_real_literal<float, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<double, Modifiers>
      : detail::basic_real_literal<double, Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<long double, Modifiers>
      : detail::basic_real_literal<long double, Modifiers> {};

    // lit(double)
    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::lit, fusion::vector1<A0> >
          , Modifiers
          , typename enable_if<traits::is_real<A0> >::type>
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef literal_real_generator<
            typename remove_const<A0>::type, real_policies<A0>
          , typename spirit::detail::get_encoding_with_case<
                Modifiers, unused_type, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
          , true
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

#endif // defined(BOOST_SPIRIT_KARMA_REAL_FEB_26_2007_0512PM)
