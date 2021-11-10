/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_X3_DIRECTIVE_WITH_HPP
#define BOOST_SPIRIT_X3_DIRECTIVE_WITH_HPP

#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    // with directive injects a value into the context prior to parsing.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Derived, typename T>
    struct with_value_holder
      : unary_parser<Subject, Derived>
    {
        typedef unary_parser<Subject, Derived> base_type;
        mutable T val;
        constexpr with_value_holder(Subject const& subject, T&& val)
          : base_type(subject)
          , val(std::forward<T>(val)) {}
    };
    
    template <typename Subject, typename Derived, typename T>
    struct with_value_holder<Subject, Derived, T&>
      : unary_parser<Subject, Derived>
    {
        typedef unary_parser<Subject, Derived> base_type;
        T& val;
        constexpr with_value_holder(Subject const& subject, T& val)
          : base_type(subject)
          , val(val) {}
    };

    template <typename Subject, typename ID, typename T>
    struct with_directive
      : with_value_holder<Subject, with_directive<Subject, ID, T>, T>
    {
        typedef with_value_holder<Subject, with_directive<Subject, ID, T>, T> base_type;
        static bool const is_pass_through_unary = true;
        static bool const handles_container = Subject::handles_container;

        typedef Subject subject_type;

        constexpr with_directive(Subject const& subject, T&& val)
          : base_type(subject, std::forward<T>(val)) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            return this->subject.parse(
                first, last
              , make_context<ID>(this->val, context)
              , rcontext
              , attr);
        }
    };
   
    template <typename ID, typename T>
    struct with_gen
    {
        T&& val;

        template <typename Subject>
        constexpr with_directive<typename extension::as_parser<Subject>::value_type, ID, T>
        operator[](Subject const& subject) const
        {
            return { as_parser(subject), std::forward<T>(val) };
        }
    };

    template <typename ID, typename T>
    constexpr with_gen<ID, T> with(T&& val)
    {
        return { std::forward<T>(val) };
    }
}}}

#endif
