//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_RULE_MAR_05_2007_0455PM)
#define BOOST_SPIRIT_KARMA_RULE_MAR_05_2007_0455PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/as_list.hpp>
#include <boost/fusion/include/as_vector.hpp>

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/context.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/nonterminal/extract_param.hpp>
#include <boost/spirit/home/support/nonterminal/locals.hpp>
#include <boost/spirit/home/karma/reference.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/nonterminal/nonterminal_fwd.hpp>
#include <boost/spirit/home/karma/nonterminal/detail/generator_binder.hpp>
#include <boost/spirit/home/karma/nonterminal/detail/parameterized.hpp>

#include <boost/static_assert.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/traits.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
# pragma warning(disable: 4355) // 'this' : used in base member initializer list warning
#endif

namespace boost { namespace spirit { namespace karma
{
    BOOST_PP_REPEAT(SPIRIT_ATTRIBUTES_LIMIT, SPIRIT_USING_ATTRIBUTE, _)

    using spirit::_pass_type;
    using spirit::_val_type;
    using spirit::_a_type;
    using spirit::_b_type;
    using spirit::_c_type;
    using spirit::_d_type;
    using spirit::_e_type;
    using spirit::_f_type;
    using spirit::_g_type;
    using spirit::_h_type;
    using spirit::_i_type;
    using spirit::_j_type;

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

    using spirit::_pass;
    using spirit::_val;
    using spirit::_a;
    using spirit::_b;
    using spirit::_c;
    using spirit::_d;
    using spirit::_e;
    using spirit::_f;
    using spirit::_g;
    using spirit::_h;
    using spirit::_i;
    using spirit::_j;

#endif

    using spirit::info;
    using spirit::locals;

    template <
        typename OutputIterator, typename T1, typename T2, typename T3
      , typename T4>
    struct rule
      : proto::extends<
            typename proto::terminal<
                reference<rule<OutputIterator, T1, T2, T3, T4> const>
            >::type
          , rule<OutputIterator, T1, T2, T3, T4>
        >
      , generator<rule<OutputIterator, T1, T2, T3, T4> >
    {
        typedef mpl::int_<generator_properties::all_properties> properties;

        typedef OutputIterator iterator_type;
        typedef rule<OutputIterator, T1, T2, T3, T4> this_type;
        typedef reference<this_type const> reference_;
        typedef typename proto::terminal<reference_>::type terminal;
        typedef proto::extends<terminal, this_type> base_type;
        typedef mpl::vector<T1, T2, T3, T4> template_params;

        // the output iterator is always wrapped by karma
        typedef detail::output_iterator<OutputIterator, properties>
            output_iterator;

        // locals_type is a sequence of types to be used as local variables
        typedef typename
            spirit::detail::extract_locals<template_params>::type
        locals_type;

        // The delimiter-generator type
        typedef typename
            spirit::detail::extract_component<
                karma::domain, template_params>::type
        delimiter_type;

        // The rule's encoding type
        typedef typename
            spirit::detail::extract_encoding<template_params>::type
        encoding_type;

        // The rule's signature
        typedef typename
            spirit::detail::extract_sig<template_params, encoding_type, karma::domain>::type
        sig_type;

        // This is the rule's attribute type
        typedef typename
            spirit::detail::attr_from_sig<sig_type>::type
        attr_type;
        BOOST_STATIC_ASSERT_MSG(
            !is_reference<attr_type>::value && !is_const<attr_type>::value,
            "Const/reference qualifiers on Karma rule attribute are meaningless");
        typedef attr_type const& attr_reference_type;

        // parameter_types is a sequence of types passed as parameters to the rule
        typedef typename
            spirit::detail::params_from_sig<sig_type>::type
        parameter_types;

        static size_t const params_size =
            fusion::result_of::size<parameter_types>::type::value;

        // the context passed to the right hand side of a rule contains
        // the attribute and the parameters for this particular rule invocation
        typedef context<
            fusion::cons<attr_reference_type, parameter_types>
          , locals_type>
        context_type;

        typedef function<
            bool(output_iterator&, context_type&, delimiter_type const&)>
        function_type;

        typedef typename
            mpl::if_<
                is_same<encoding_type, unused_type>
              , unused_type
              , tag::char_code<tag::encoding, encoding_type>
            >::type
        encoding_modifier_type;

        explicit rule(std::string const& name_ = "unnamed-rule")
          : base_type(terminal::make(reference_(*this)))
          , name_(name_)
        {
        }

        rule(rule const& rhs)
          : base_type(terminal::make(reference_(*this)))
          , name_(rhs.name_)
          , f(rhs.f)
        {
        }

        template <typename Auto, typename Expr>
        static void define(rule& /* lhs */, Expr const& /* expr */, mpl::false_)
        {
            // Report invalid expression error as early as possible.
            // If you got an error_invalid_expression error message here,
            // then the expression (expr) is not a valid spirit karma expression.
            BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        }

        template <typename Auto, typename Expr>
        static void define(rule& lhs, Expr const& expr, mpl::true_)
        {
            lhs.f = detail::bind_generator<Auto>(
                compile<karma::domain>(expr, encoding_modifier_type()));
        }

        template <typename Expr>
        rule (Expr const& expr, std::string const& name_ = "unnamed-rule")
          : base_type(terminal::make(reference_(*this)))
          , name_(name_)
        {
            define<mpl::false_>(*this, expr, traits::matches<karma::domain, Expr>());
        }

        rule& operator=(rule const& rhs)
        {
            // The following assertion fires when you try to initialize a rule
            // from an uninitialized one. Did you mean to refer to the right
            // hand side rule instead of assigning from it? In this case you
            // should write lhs = rhs.alias();
            BOOST_ASSERT(rhs.f && "Did you mean rhs.alias() instead of rhs?");

            f = rhs.f;
            name_ = rhs.name_;
            return *this;
        }

        std::string const& name() const
        {
            return name_;
        }

        void name(std::string const& str)
        {
            name_ = str;
        }

        template <typename Expr>
        rule& operator=(Expr const& expr)
        {
            define<mpl::false_>(*this, expr, traits::matches<karma::domain, Expr>());
            return *this;
        }

// VC7.1 has problems to resolve 'rule' without explicit template parameters
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1400)
        // g++ 3.3 barfs if this is a member function :(
        template <typename Expr>
        friend rule& operator%=(rule& r, Expr const& expr)
        {
            define<mpl::true_>(r, expr, traits::matches<karma::domain, Expr>());
            return r;
        }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        // non-const version needed to suppress proto's %= kicking in
        template <typename Expr>
        friend rule& operator%=(rule& r, Expr& expr)
        {
            return r %= static_cast<Expr const&>(expr);
        }
#else
        // for rvalue references
        template <typename Expr>
        friend rule& operator%=(rule& r, Expr&& expr)
        {
            define<mpl::true_>(r, expr, traits::matches<karma::domain, Expr>());
            return r;
        }
#endif

#else
        // both friend functions have to be defined out of class as VC7.1
        // will complain otherwise
        template <typename OutputIterator_, typename T1_, typename T2_
          , typename T3_, typename T4_, typename Expr>
        friend rule<OutputIterator_, T1_, T2_, T3_, T4_>& operator%=(
            rule<OutputIterator_, T1_, T2_, T3_, T4_>&r, Expr const& expr);

        // non-const version needed to suppress proto's %= kicking in
        template <typename OutputIterator_, typename T1_, typename T2_
          , typename T3_, typename T4_, typename Expr>
        friend rule<OutputIterator_, T1_, T2_, T3_, T4_>& operator%=(
            rule<OutputIterator_, T1_, T2_, T3_, T4_>& r, Expr& expr);
#endif

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef attr_type type;
        };

        template <typename Context, typename Delimiter, typename Attribute>
        bool generate(output_iterator& sink, Context&, Delimiter const& delim
          , Attribute const& attr) const
        {
            if (f)
            {
                // Create an attribute if none is supplied.
                typedef traits::transform_attribute<
                    Attribute const, attr_type, domain>
                transform;

                typename transform::type attr_ = transform::pre(attr);

                // If you are seeing a compilation error here, you are probably
                // trying to use a rule or a grammar which has inherited
                // attributes, without passing values for them.
                context_type context(attr_);

                // If you are seeing a compilation error here stating that the
                // third parameter can't be converted to a karma::reference
                // then you are probably trying to use a rule or a grammar with
                // an incompatible delimiter type.
                if (f(sink, context, delim))
                {
                    // do a post-delimit if this is an implied verbatim
                    if (is_same<delimiter_type, unused_type>::value)
                        karma::delimit_out(sink, delim);

                    return true;
                }
            }
            return false;
        }

        template <typename Context, typename Delimiter, typename Attribute
          , typename Params>
        bool generate(output_iterator& sink, Context& caller_context
          , Delimiter const& delim, Attribute const& attr
          , Params const& params) const
        {
            if (f)
            {
                // Create an attribute if none is supplied.
                typedef traits::transform_attribute<
                    Attribute const, attr_type, domain>
                transform;

                typename transform::type attr_ = transform::pre(attr);

                // If you are seeing a compilation error here, you are probably
                // trying to use a rule or a grammar which has inherited
                // attributes, passing values of incompatible types for them.
                context_type context(attr_, params, caller_context);

                // If you are seeing a compilation error here stating that the
                // third parameter can't be converted to a karma::reference
                // then you are probably trying to use a rule or a grammar with
                // an incompatible delimiter type.
                if (f(sink, context, delim))
                {
                    // do a post-delimit if this is an implied verbatim
                    if (is_same<delimiter_type, unused_type>::value)
                        karma::delimit_out(sink, delim);

                    return true;
                }
            }
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info(name_);
        }

        reference_ alias() const
        {
            return reference_(*this);
        }

        typename proto::terminal<this_type>::type copy() const
        {
            typename proto::terminal<this_type>::type result = {*this};
            return result;
        }

        // bring in the operator() overloads
        rule const& get_parameterized_subject() const { return *this; }
        typedef rule parameterized_subject_type;
        #include <boost/spirit/home/karma/nonterminal/detail/fcall.hpp>

        std::string name_;
        function_type f;
    };

#if BOOST_WORKAROUND(BOOST_MSVC, < 1400)
    template <typename OutputIterator_, typename T1_, typename T2_
      , typename T3_, typename T4_, typename Expr>
    rule<OutputIterator_, T1_, T2_, T3_, T4_>& operator%=(
        rule<OutputIterator_, T1_, T2_, T3_, T4_>&r, Expr const& expr)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here, then
        // the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);

        typedef typename
            rule<OutputIterator_, T1_, T2_, T3_, T4_>::encoding_modifier_type
        encoding_modifier_type;

        r.f = detail::bind_generator<mpl::true_>(
            compile<karma::domain>(expr, encoding_modifier_type()));
        return r;
    }

    // non-const version needed to suppress proto's %= kicking in
    template <typename OutputIterator_, typename T1_, typename T2_
      , typename T3_, typename T4_, typename Expr>
    rule<OutputIterator_, T1_, T2_, T3_, T4_>& operator%=(
        rule<OutputIterator_, T1_, T2_, T3_, T4_>& r, Expr& expr)
    {
        return r %= static_cast<Expr const&>(expr);
    }
#endif
}}}

namespace boost { namespace spirit { namespace traits
{
    namespace detail
    {
        template <typename RuleAttribute, typename Attribute>
        struct nonterminal_handles_container
          : mpl::and_<
                traits::is_container<RuleAttribute>
              , is_convertible<Attribute, RuleAttribute> >
        {};
    }

    ///////////////////////////////////////////////////////////////////////////
    template <
        typename IteratorA, typename IteratorB, typename Attribute
      , typename Context, typename T1, typename T2, typename T3, typename T4>
    struct handles_container<
            karma::rule<IteratorA, T1, T2, T3, T4>, Attribute, Context
          , IteratorB>
      : detail::nonterminal_handles_container<
            typename attribute_of<
                karma::rule<IteratorA, T1, T2, T3, T4>
              , Context, IteratorB
            >::type, Attribute>
    {};
}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
