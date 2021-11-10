/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_ACTION_JANUARY_07_2007_1128AM)
#define BOOST_SPIRIT_X3_ACTION_JANUARY_07_2007_1128AM

#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/core/call.hpp>
#include <boost/spirit/home/x3/nonterminal/detail/transform_attribute.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace boost { namespace spirit { namespace x3
{
    struct raw_attribute_type;
    struct parse_pass_context_tag;

    template <typename Context>
    inline bool& _pass(Context const& context)
    {
        return x3::get<parse_pass_context_tag>(context);
    }

    template <typename Subject, typename Action>
    struct action : unary_parser<Subject, action<Subject, Action>>
    {
        typedef unary_parser<Subject, action<Subject, Action>> base_type;
        static bool const is_pass_through_unary = true;
        static bool const has_action = true;

        constexpr action(Subject const& subject, Action f)
          : base_type(subject), f(f) {}

        template <typename Iterator, typename Context, typename RuleContext, typename Attribute>
        bool call_action(
            Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, Attribute& attr) const
        {
            bool pass = true;
            auto action_context = make_context<parse_pass_context_tag>(pass, context);
            call(f, first, last, action_context, rcontext, attr);
            return pass;
        }

        template <typename Iterator, typename Context
          , typename RuleContext, typename Attribute>
        bool parse_main(Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, Attribute& attr) const
        {
            Iterator save = first;
            if (this->subject.parse(first, last, context, rcontext, attr))
            {
                if (call_action(first, last, context, rcontext, attr))
                    return true;

                // reset iterators if semantic action failed the match
                // retrospectively
                first = save;
            }
            return false;
        }
        
        // attr==raw_attribute_type, action wants iterator_range (see raw.hpp)
        template <typename Iterator, typename Context, typename RuleContext>
        bool parse_main(Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, raw_attribute_type&) const
        {
            boost::iterator_range<Iterator> rng;
            // synthesize the attribute since one is not supplied
            return parse_main(first, last, context, rcontext, rng);
        }

        // attr==unused, action wants attribute
        template <typename Iterator, typename Context, typename RuleContext>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, unused_type) const
        {
            typedef typename
                traits::attribute_of<action<Subject, Action>, Context>::type
            attribute_type;

            // synthesize the attribute since one is not supplied
            attribute_type attribute{};
            return parse_main(first, last, context, rcontext, attribute);
        }
        
        // main parse function
        template <typename Iterator, typename Context
            , typename RuleContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RuleContext& rcontext, Attribute& attr) const
        {
            return parse_main(first, last, context, rcontext, attr);
        }

        Action f;
    };

    template <typename P, typename Action>
    constexpr action<typename extension::as_parser<P>::value_type, Action>
    operator/(P const& p, Action f)
    {
        return { as_parser(p), f };
    }
}}}

#endif
