/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_LEXEME_MARCH_24_2007_0802AM)
#define BOOST_SPIRIT_X3_LEXEME_MARCH_24_2007_0802AM

#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename Subject>
    struct lexeme_directive : unary_parser<Subject, lexeme_directive<Subject>>
    {
        typedef unary_parser<Subject, lexeme_directive<Subject> > base_type;
        static bool const is_pass_through_unary = true;
        static bool const handles_container = Subject::handles_container;

        constexpr lexeme_directive(Subject const& subject)
          : base_type(subject) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        typename enable_if<has_skipper<Context>, bool>::type
        parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            x3::skip_over(first, last, context);
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
            //  no need to pre-skip if skipper is unused
            return this->subject.parse(
                first, last
              , context
              , rcontext
              , attr);
        }
    };

    struct lexeme_gen
    {
        template <typename Subject>
        constexpr lexeme_directive<typename extension::as_parser<Subject>::value_type>
        operator[](Subject const& subject) const
        {
            return { as_parser(subject) };
        }
    };

    constexpr auto lexeme = lexeme_gen{};
}}}

#endif
