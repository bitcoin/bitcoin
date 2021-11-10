/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2014 Thomas Bernard

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_X3_DIRECTIVE_REPEAT_HPP
#define BOOST_SPIRIT_X3_DIRECTIVE_REPEAT_HPP

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/operator/kleene.hpp>

namespace boost { namespace spirit { namespace x3 { namespace detail
{
    template <typename T>
    struct exact_count // handles repeat(exact)[p]
    {
        typedef T type;
        bool got_max(T i) const { return i >= exact_value; }
        bool got_min(T i) const { return i >= exact_value; }

        T const exact_value;
    };

    template <typename T>
    struct finite_count // handles repeat(min, max)[p]
    {
        typedef T type;
        bool got_max(T i) const { return i >= max_value; }
        bool got_min(T i) const { return i >= min_value; }

        T const min_value;
        T const max_value;
    };

    template <typename T>
    struct infinite_count // handles repeat(min, inf)[p]
    {
        typedef T type;
        bool got_max(T /*i*/) const { return false; }
        bool got_min(T i) const { return i >= min_value; }

        T const min_value;
    };
}}}}

namespace boost { namespace spirit { namespace x3
{
    template<typename Subject, typename RepeatCountLimit>
    struct repeat_directive : unary_parser<Subject, repeat_directive<Subject,RepeatCountLimit>>
    {
        typedef unary_parser<Subject, repeat_directive<Subject,RepeatCountLimit>> base_type;
        static bool const is_pass_through_unary = true;
        static bool const handles_container = true;

        constexpr repeat_directive(Subject const& subject, RepeatCountLimit const& repeat_limit_)
          : base_type(subject)
          , repeat_limit(repeat_limit_)
        {}

        template<typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(
            Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            Iterator local_iterator = first;
            typename RepeatCountLimit::type i{};
            for (/**/; !repeat_limit.got_min(i); ++i)
            {
                if (!detail::parse_into_container(
                      this->subject, local_iterator, last, context, rcontext, attr))
                    return false;
            }

            first = local_iterator;
            // parse some more up to the maximum specified
            for (/**/; !repeat_limit.got_max(i); ++i)
            {
                if (!detail::parse_into_container(
                      this->subject, first, last, context, rcontext, attr))
                    break;
            }
            return true;
        }

        RepeatCountLimit repeat_limit;
    };

    // Infinite loop tag type
    struct inf_type {};
    constexpr inf_type inf = inf_type();

    struct repeat_gen
    {
        template<typename Subject>
        constexpr auto operator[](Subject const& subject) const
        {
            return *as_parser(subject);
        }

        template <typename T>
        struct repeat_gen_lvl1
        {
            constexpr repeat_gen_lvl1(T&& repeat_limit_)
              : repeat_limit(repeat_limit_)
            {}

            template<typename Subject>
            constexpr repeat_directive< typename extension::as_parser<Subject>::value_type, T>
            operator[](Subject const& subject) const
            {
                return { as_parser(subject),repeat_limit };
            }

            T repeat_limit;
        };

        template <typename T>
        constexpr repeat_gen_lvl1<detail::exact_count<T>>
        operator()(T const exact) const
        {
            return { detail::exact_count<T>{exact} };
        }

        template <typename T>
        constexpr repeat_gen_lvl1<detail::finite_count<T>>
        operator()(T const min_val, T const max_val) const
        {
            return { detail::finite_count<T>{min_val,max_val} };
        }

        template <typename T>
        constexpr repeat_gen_lvl1<detail::infinite_count<T>>
        operator()(T const min_val, inf_type const &) const
        {
            return { detail::infinite_count<T>{min_val} };
        }
    };

    constexpr auto repeat = repeat_gen{};
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Subject, typename RepeatCountLimit, typename Context>
    struct attribute_of<x3::repeat_directive<Subject,RepeatCountLimit>, Context>
      : build_container<typename attribute_of<Subject, Context>::type> {};
}}}}


#endif
