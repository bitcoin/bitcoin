///////////////////////////////////////////////////////////////////////////////
// as_inverse.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_INVERSE_HPP_EAN_04_05_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_INVERSE_HPP_EAN_04_05_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/not.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/proto/core.hpp>

#define UNCV(x) typename remove_const<x>::type
#define UNREF(x) typename remove_reference<x>::type
#define UNCVREF(x) UNCV(UNREF(x))

namespace boost { namespace xpressive { namespace grammar_detail
{

    template<typename T>
    struct inverter
    {
        typedef T type;
        static T call(T t)
        {
            t.inverse();
            return t;
        }
    };

    template<typename Traits, typename ICase, typename Not>
    struct inverter<detail::literal_matcher<Traits, ICase, Not> >
    {
        typedef detail::literal_matcher<Traits, ICase, typename mpl::not_<Not>::type> type;
        static type call(detail::literal_matcher<Traits, ICase, Not> t)
        {
            return type(t.ch_);
        }
    };

    template<typename Traits>
    struct inverter<detail::logical_newline_matcher<Traits> >
    {
        // ~_ln matches any one character that is not in the "newline" character class
        typedef detail::posix_charset_matcher<Traits> type;
        static type call(detail::logical_newline_matcher<Traits> t)
        {
            return type(t.newline(), true);
        }
    };

    template<typename Traits>
    struct inverter<detail::assert_word_matcher<detail::word_boundary<mpl::true_>, Traits> >
    {
        typedef detail::assert_word_matcher<detail::word_boundary<mpl::false_>, Traits> type;
        static type call(detail::assert_word_matcher<detail::word_boundary<mpl::true_>, Traits> t)
        {
            return type(t.word());
        }
    };

    struct as_inverse : proto::callable
    {
        template<typename Sig>
        struct result;

        template<typename This, typename Matcher>
        struct result<This(Matcher)>
          : inverter<UNCVREF(Matcher)>
        {};

        template<typename Matcher>
        typename inverter<Matcher>::type operator ()(Matcher const &matcher) const
        {
            return inverter<Matcher>::call(matcher);
        }
    };

}}}

#undef UNCV
#undef UNREF
#undef UNCVREF

#endif
