/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2013 Agustin Berge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_NO_SKIP_JAN_16_2010_0802PM)
#define BOOST_SPIRIT_X3_NO_SKIP_JAN_16_2010_0802PM

#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit { namespace x3
{
    // same as lexeme[], but does not pre-skip
    template <typename Subject>
    struct no_skip_directive : unary_parser<Subject, no_skip_directive<Subject>>
    {
        typedef unary_parser<Subject, no_skip_directive<Subject> > base_type;
        static bool const is_pass_through_unary = true;
        static bool const handles_container = Subject::handles_container;

        constexpr no_skip_directive(Subject const& subject)
          : base_type(subject) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        typename enable_if<has_skipper<Context>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            auto const& skipper = x3::get<skipper_tag>(context);

            typedef unused_skipper<
                typename remove_reference<decltype(skipper)>::type>
            unused_skipper_type;
            unused_skipper_type unused_skipper(skipper);

            return this->subject.parse(
                first, last
              , make_context<skipper_tag>(unused_skipper, context)
              , rcontext
              , attr);
        }
        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        typename disable_if<has_skipper<Context>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            return this->subject.parse(
                first, last
              , context
              , rcontext
              , attr);
        }
    };

    struct no_skip_gen
    {
        template <typename Subject>
        constexpr no_skip_directive<typename extension::as_parser<Subject>::value_type>
        operator[](Subject const& subject) const
        {
            return { as_parser(subject) };
        }
    };

    constexpr auto no_skip = no_skip_gen{};
}}}

#endif
