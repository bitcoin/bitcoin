/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_REAL_APRIL_18_2006_0850AM)
#define BOOST_SPIRIT_REAL_APRIL_18_2006_0850AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/detail/enable_lit.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/numeric/real_policies.hpp>
#include <boost/spirit/home/qi/numeric/numeric_utils.hpp>
#include <boost/spirit/home/qi/numeric/detail/real_impl.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit
{
    namespace qi
    {
        ///////////////////////////////////////////////////////////////////////
        // forward declaration only
        template <typename T>
        struct real_policies;

        ///////////////////////////////////////////////////////////////////////
        // This is the class that the user can instantiate directly in
        // order to create a customized real parser
        template <typename T = double, typename Policies = real_policies<T> >
        struct real_parser
          : spirit::terminal<tag::stateful_tag<Policies, tag::double_, T> >
        {
            typedef tag::stateful_tag<Policies, tag::double_, T> tag_type;

            real_parser() {}
            real_parser(Policies const& p)
              : spirit::terminal<tag_type>(p) {}
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <> // enables float_
    struct use_terminal<qi::domain, tag::float_>
      : mpl::true_ {};

    template <> // enables double_
    struct use_terminal<qi::domain, tag::double_>
      : mpl::true_ {};

    template <> // enables long_double
    struct use_terminal<qi::domain, tag::long_double>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, float> >::type>
      : mpl::true_ {};

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, double> >::type>
      : mpl::true_ {};

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, long double> >::type>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename A0> // enables float_(...)
    struct use_terminal<qi::domain
      , terminal_ex<tag::float_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0> // enables double_(...)
    struct use_terminal<qi::domain
      , terminal_ex<tag::double_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <typename A0> // enables long_double(...)
    struct use_terminal<qi::domain
      , terminal_ex<tag::long_double, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <> // enables *lazy* float_(...)
    struct use_lazy_terminal<qi::domain, tag::float_, 1>
      : mpl::true_ {};

    template <> // enables *lazy* double_(...)
    struct use_lazy_terminal<qi::domain, tag::double_, 1>
      : mpl::true_ {};

    template <> // enables *lazy* long_double_(...)
    struct use_lazy_terminal<qi::domain, tag::long_double, 1>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // enables custom real_parser
    template <typename T, typename Policies>
    struct use_terminal<qi::domain
        , tag::stateful_tag<Policies, tag::double_, T> >
      : mpl::true_ {};

    // enables custom real_parser(...)
    template <typename T, typename Policies, typename A0>
    struct use_terminal<qi::domain
        , terminal_ex<tag::stateful_tag<Policies, tag::double_, T>
        , fusion::vector1<A0> > >
      : mpl::true_ {};

    // enables *lazy* custom real_parser(...)
    template <typename T, typename Policies>
    struct use_lazy_terminal<
        qi::domain
      , tag::stateful_tag<Policies, tag::double_, T>
      , 1 // arity
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::float_;
    using spirit::double_;
    using spirit::long_double;
    using spirit::lit; // lit(1.0) is equivalent to 1.0
#endif

    using spirit::float_type;
    using spirit::double_type;
    using spirit::long_double_type;
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    // This is the actual real number parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename RealPolicies = real_policies<T> >
    struct any_real_parser
      : primitive_parser<any_real_parser<T, RealPolicies> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        template <typename Iterator, typename Context, typename Skipper>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , T& attr_) const
        {
            typedef detail::real_impl<T, RealPolicies> extract;
            qi::skip_over(first, last, skipper);
            return extract::parse(first, last, attr_, RealPolicies());
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_param) const
        {
            // this case is called when Attribute is not T
            T attr_;
            if (parse(first, last, context, skipper, attr_))
            {
                traits::assign_to(attr_, attr_param);
                return true;
            }
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("real");
        }
    };

    template <typename T, typename RealPolicies = real_policies<T>
            , bool no_attribute = true>
    struct literal_real_parser
      : primitive_parser<literal_real_parser<T, RealPolicies, no_attribute> >
    {
        template <typename Value>
        literal_real_parser(Value const& n) : n_(n) {}

        template <typename Context, typename Iterator>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, T>
        {};

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context&, Skipper const& skipper
          , Attribute& attr_param) const
        {
            typedef detail::real_impl<T, RealPolicies> extract;
            qi::skip_over(first, last, skipper);

            Iterator save = first;
            T attr_;

            if (extract::parse(first, last, attr_, RealPolicies()) &&
                (attr_ == n_))
            {
                traits::assign_to(attr_, attr_param);
                return true;
            }

            first = save;
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("real");
        }

        T n_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Policies = real_policies<T> >
    struct make_real
    {
        typedef any_real_parser<T, Policies> result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename T, typename Policies = real_policies<T> >
    struct make_direct_real
    {
        typedef literal_real_parser<T, Policies, false> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(T(fusion::at_c<0>(term.args)));
        }
    };

    template <typename T, typename Policies = real_policies<T> >
    struct make_literal_real
    {
        typedef literal_real_parser<T, Policies> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, float> >::type>
      : make_literal_real<float> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, double> >::type>
      : make_literal_real<double> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, long double> >::type>
      : make_literal_real<long double> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Policies, typename Modifiers>
    struct make_primitive<
        tag::stateful_tag<Policies, tag::double_, T>, Modifiers>
      : make_real<T, Policies> {};

    template <typename T, typename Policies, typename A0, typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::stateful_tag<Policies, tag::double_, T>
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_real<T, Policies> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::float_, Modifiers>
      : make_real<float> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::float_
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_real<float> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::double_, Modifiers>
      : make_real<double> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::double_
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_real<double> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::long_double, Modifiers>
      : make_real<long double> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::long_double
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_real<long double> {};
}}}

#endif
