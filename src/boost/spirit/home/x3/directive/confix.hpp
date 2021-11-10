/*=============================================================================
    Copyright (c) 2009 Chris Hoeppler
    Copyright (c) 2014 Lee Clagett

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_SPIRIT_X3_CONFIX_MAY_30_2014_1819PM)
#define BOOST_SPIRIT_X3_CONFIX_MAY_30_2014_1819PM

#include <boost/spirit/home/x3/core/parser.hpp>

namespace boost { namespace spirit { namespace x3
{
    template<typename Prefix, typename Subject, typename Postfix>
    struct confix_directive :
        unary_parser<Subject, confix_directive<Prefix, Subject, Postfix>>
    {
        typedef unary_parser<
            Subject, confix_directive<Prefix, Subject, Postfix>> base_type;
        static bool const is_pass_through_unary = true;
        static bool const handles_container = Subject::handles_container;

        constexpr confix_directive(Prefix const& prefix
                         , Subject const& subject
                         , Postfix const& postfix) :
            base_type(subject),
            prefix(prefix),
            postfix(postfix)
        {
        }

        template<typename Iterator, typename Context
                 , typename RContext, typename Attribute>
        bool parse(
            Iterator& first, Iterator const& last
            , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            Iterator save = first;

            if (!(prefix.parse(first, last, context, rcontext, unused) &&
                  this->subject.parse(first, last, context, rcontext, attr) &&
                  postfix.parse(first, last, context, rcontext, unused)))
            {
                first = save;
                return false;
            }

            return true;
        }

        Prefix prefix;
        Postfix postfix;
    };

    template<typename Prefix, typename Postfix>
    struct confix_gen
    {
        template<typename Subject>
        constexpr confix_directive<
            Prefix, typename extension::as_parser<Subject>::value_type, Postfix>
        operator[](Subject const& subject) const
        {
            return { prefix, as_parser(subject), postfix };
        }

        Prefix prefix;
        Postfix postfix;
    };


    template<typename Prefix, typename Postfix>
    constexpr confix_gen<typename extension::as_parser<Prefix>::value_type,
               typename extension::as_parser<Postfix>::value_type>
    confix(Prefix const& prefix, Postfix const& postfix)
    {
        return { as_parser(prefix), as_parser(postfix) };
    }

}}}

#endif
