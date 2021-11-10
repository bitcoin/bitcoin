//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_BOOL_SEP_28_2009_1113AM)
#define BOOST_SPIRIT_KARMA_BOOL_SEP_28_2009_1113AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/limits.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/string_traits.hpp>
#include <boost/spirit/home/support/numeric_traits.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/get_casetag.hpp>
#include <boost/spirit/home/karma/detail/extract_from.hpp>
#include <boost/spirit/home/karma/detail/enable_lit.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/numeric/bool_policies.hpp>
#include <boost/spirit/home/karma/numeric/detail/bool_utils.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    namespace karma
    {
        ///////////////////////////////////////////////////////////////////////
        // forward declaration only
        template <typename T>
        struct bool_policies;

        ///////////////////////////////////////////////////////////////////////
        // This is the class that the user can instantiate directly in
        // order to create a customized bool generator
        template <typename T = bool, typename Policies = bool_policies<T> >
        struct bool_generator
          : spirit::terminal<tag::stateful_tag<Policies, tag::bool_, T> >
        {
            typedef tag::stateful_tag<Policies, tag::bool_, T> tag_type;

            bool_generator() {}
            bool_generator(Policies const& data)
              : spirit::terminal<tag_type>(data) {}
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<karma::domain, tag::bool_>    // enables bool_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::true_>    // enables true_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, tag::false_>    // enables false_
      : mpl::true_ {};

    template <>
    struct use_terminal<karma::domain, bool>          // enables lit(true)
      : mpl::true_ {};

    template <typename A0>
    struct use_terminal<karma::domain                 // enables bool_(...)
      , terminal_ex<tag::bool_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <>                                       // enables *lazy* bool_(...)
    struct use_lazy_terminal<karma::domain, tag::bool_, 1>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // enables any custom bool_generator
    template <typename Policies, typename T>
    struct use_terminal<karma::domain
          , tag::stateful_tag<Policies, tag::bool_, T> >
      : mpl::true_ {};

    // enables any custom bool_generator(...)
    template <typename Policies, typename T, typename A0>
    struct use_terminal<karma::domain
          , terminal_ex<tag::stateful_tag<Policies, tag::bool_, T>
          , fusion::vector1<A0> > >
      : mpl::true_ {};

    // enables *lazy* custom bool_generator
    template <typename Policies, typename T>
    struct use_lazy_terminal<karma::domain
          , tag::stateful_tag<Policies, tag::bool_, T>, 1>
      : mpl::true_ {};

    // enables lit(bool)
    template <typename A0>
    struct use_terminal<karma::domain
          , terminal_ex<tag::lit, fusion::vector1<A0> >
          , typename enable_if<traits::is_bool<A0> >::type>
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::bool_;
    using spirit::true_;
    using spirit::false_;
    using spirit::lit;    // lit(true) is equivalent to true
#endif

    using spirit::bool_type;
    using spirit::true_type;
    using spirit::false_type;
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization is used for bool generators not having a direct
    //  initializer: bool_. These generators must be used in conjunction with
    //  an Attribute.
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename CharEncoding, typename Tag, typename Policies>
    struct any_bool_generator
      : primitive_generator<any_bool_generator<T, CharEncoding, Tag, Policies> >
    {
    public:
        any_bool_generator(Policies const& p = Policies())
          : p_(p) {}

        typedef typename Policies::properties properties;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef T type;
        };

        // bool_ has a Attribute attached
        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool
        generate(OutputIterator& sink, Context& context, Delimiter const& d
          , Attribute const& attr) const
        {
            if (!traits::has_optional_value(attr))
                return false;       // fail if it's an uninitialized optional

            return bool_inserter<T, Policies, CharEncoding, Tag>::call(
                        sink, traits::extract_from<T>(attr, context), p_) &&
                   delimit_out(sink, d);      // always do post-delimiting
        }

        // this bool_ has no Attribute attached, it needs to have been
        // initialized from a direct literal
        template <typename OutputIterator, typename Context, typename Delimiter>
        static bool
        generate(OutputIterator&, Context&, Delimiter const&, unused_type)
        {
            // It is not possible (doesn't make sense) to use boolean generators
            // without providing any attribute, as the generator doesn't 'know'
            // what to output. The following assertion fires if this situation
            // is detected in your code.
            BOOST_SPIRIT_ASSERT_FAIL(OutputIterator, bool_not_usable_without_attribute, ());
            return false;
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("bool");
        }

        Policies p_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization is used for bool generators having a direct
    //  initializer: bool_(true), bool_(0) etc.
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename CharEncoding, typename Tag
      , typename Policies, bool no_attribute>
    struct literal_bool_generator
      : primitive_generator<literal_bool_generator<T, CharEncoding, Tag
          , Policies, no_attribute> >
    {
    public:
        typedef typename Policies::properties properties;

        template <typename Context, typename Unused = unused_type>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, T>
        {};

        literal_bool_generator(typename add_const<T>::type n
              , Policies const& p = Policies())
          : n_(n), p_(p) {}

        // A bool_() which additionally has an associated attribute emits
        // its immediate literal only if it matches the attribute, otherwise
        // it fails.
        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& context
          , Delimiter const& d, Attribute const& attr) const
        {
            typedef typename attribute<Context>::type attribute_type;
            if (!traits::has_optional_value(attr) ||
                bool(n_) != bool(traits::extract_from<attribute_type>(attr, context)))
            {
                return false;
            }
            return bool_inserter<T, Policies, CharEncoding, Tag>::
                      call(sink, n_, p_) && delimit_out(sink, d);
        }

        // A bool_() without any associated attribute just emits its
        // immediate literal
        template <typename OutputIterator, typename Context, typename Delimiter>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , unused_type) const
        {
            return bool_inserter<T, Policies, CharEncoding, Tag>::
                      call(sink, n_) && delimit_out(sink, d);
        }

        template <typename Context>
        static info what(Context const& /*context*/)
        {
            return info("bool");
        }

        T n_;
        Policies p_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Modifiers, typename T = bool
          , typename Policies = bool_policies<T> >
        struct make_bool
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef any_bool_generator<
                T
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , Policies
            > result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                typedef tag::stateful_tag<Policies, tag::bool_, T> tag_type;
                using spirit::detail::get_stateful_data;
                return result_type(get_stateful_data<tag_type>::call(term));
            }
        };

        template <typename Modifiers, bool b>
        struct make_bool_literal
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_bool_generator<
                bool
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , bool_policies<>, false
            > result_type;

            result_type operator()(unused_type, unused_type) const
            {
                return result_type(b);
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::bool_, Modifiers>
      : detail::make_bool<Modifiers> {};

    template <typename Modifiers>
    struct make_primitive<tag::true_, Modifiers>
      : detail::make_bool_literal<Modifiers, true> {};

    template <typename Modifiers>
    struct make_primitive<tag::false_, Modifiers>
      : detail::make_bool_literal<Modifiers, false> {};

    template <typename T, typename Policies, typename Modifiers>
    struct make_primitive<
            tag::stateful_tag<Policies, tag::bool_, T>, Modifiers>
      : detail::make_bool<Modifiers
          , typename remove_const<T>::type, Policies> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Modifiers, typename T = bool
          , typename Policies = bool_policies<T> >
        struct make_bool_direct
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_bool_generator<
                T
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , Policies, false
            > result_type;

            template <typename Terminal>
            result_type operator()(Terminal const& term, unused_type) const
            {
                typedef tag::stateful_tag<Policies, tag::bool_, T> tag_type;
                using spirit::detail::get_stateful_data;
                return result_type(fusion::at_c<0>(term.args)
                  , get_stateful_data<tag_type>::call(term.term));
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::bool_, fusion::vector1<A0> >, Modifiers>
      : detail::make_bool_direct<Modifiers> {};

    template <typename T, typename Policies, typename A0, typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::stateful_tag<Policies, tag::bool_, T>
          , fusion::vector1<A0> >
          , Modifiers>
      : detail::make_bool_direct<Modifiers
          , typename remove_const<T>::type, Policies> {};

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Modifiers>
        struct basic_bool_literal
        {
            static bool const lower =
                has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
            static bool const upper =
                has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

            typedef literal_bool_generator<
                bool
              , typename spirit::detail::get_encoding_with_case<
                    Modifiers, unused_type, lower || upper>::type
              , typename detail::get_casetag<Modifiers, lower || upper>::type
              , bool_policies<>, true
            > result_type;

            template <typename T_>
            result_type operator()(T_ i, unused_type) const
            {
                return result_type(i);
            }
        };
    }

    template <typename Modifiers>
    struct make_primitive<bool, Modifiers>
      : detail::basic_bool_literal<Modifiers> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
            terminal_ex<tag::lit, fusion::vector1<A0> >
          , Modifiers
          , typename enable_if<traits::is_bool<A0> >::type>
      : detail::basic_bool_literal<Modifiers>
    {
        static bool const lower =
            has_modifier<Modifiers, tag::char_code_base<tag::lower> >::value;
        static bool const upper =
            has_modifier<Modifiers, tag::char_code_base<tag::upper> >::value;

        typedef literal_bool_generator<
            bool
          , typename spirit::detail::get_encoding_with_case<
                Modifiers, unused_type, lower || upper>::type
          , typename detail::get_casetag<Modifiers, lower || upper>::type
          , bool_policies<>, true
        > result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

#endif
