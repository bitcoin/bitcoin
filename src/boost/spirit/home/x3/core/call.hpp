/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_CALL_CONTEXT_MAY_26_2014_0234PM)
#define BOOST_SPIRIT_X3_CALL_CONTEXT_MAY_26_2014_0234PM

#include <type_traits>

#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/utility/is_callable.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace boost { namespace spirit { namespace x3
{
    ////////////////////////////////////////////////////////////////////////////
    struct rule_val_context_tag;

    template <typename Context>
    inline decltype(auto) _val(Context const& context)
    {
        return x3::get<rule_val_context_tag>(context);
    }

    ////////////////////////////////////////////////////////////////////////////
    struct where_context_tag;

    template <typename Context>
    inline decltype(auto) _where(Context const& context)
    {
        return x3::get<where_context_tag>(context);
    }

    ////////////////////////////////////////////////////////////////////////////
    struct attr_context_tag;

    template <typename Context>
    inline decltype(auto) _attr(Context const& context)
    {
        return x3::get<attr_context_tag>(context);
    }

    ////////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename F, typename Context>
        auto call(F f, Context const& context, mpl::true_)
        {
            return f(context);
        }

        template <typename F, typename Context>
        auto call(F f, Context const& /* context */, mpl::false_)
        {
            return f();
        }
    }

    template <
        typename F, typename Iterator
      , typename Context, typename RuleContext, typename Attribute>
    auto call(
        F f, Iterator& first, Iterator const& last
      , Context const& context, RuleContext& rcontext, Attribute& attr)
    {
        boost::iterator_range<Iterator> rng(first, last);
        auto val_context = make_context<rule_val_context_tag>(rcontext, context);
        auto where_context = make_context<where_context_tag>(rng, val_context);
        auto attr_context = make_context<attr_context_tag>(attr, where_context);
        return detail::call(f, attr_context, is_callable<F(decltype(attr_context) const&)>());
    }
}}}

#endif
