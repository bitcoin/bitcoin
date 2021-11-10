///////////////////////////////////////////////////////////////////////////////
// transmogrify.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSMOGRIFY_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSMOGRIFY_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <cstring> // for std::strlen
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/matchers.hpp>
#include <boost/xpressive/detail/static/placeholders.hpp>
#include <boost/xpressive/detail/utility/dont_care.hpp>
#include <boost/xpressive/detail/utility/traits_utils.hpp>

namespace boost { namespace xpressive { namespace detail
{
    template<typename T, typename Char>
    struct is_char_literal
      : mpl::or_<is_same<T, Char>, is_same<T, char> >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // transmogrify
    //
    template<typename BidiIter, typename ICase, typename Traits, typename Matcher, typename EnableIf = void>
    struct default_transmogrify
    {
        typedef typename Traits::char_type char_type;
        typedef typename Traits::string_type string_type;

        typedef typename mpl::if_c
        <
            is_char_literal<Matcher, char_type>::value
          , literal_matcher<Traits, ICase, mpl::false_>
          , string_matcher<Traits, ICase>
        >::type type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2 const &m, Visitor &visitor)
        {
            return default_transmogrify::call_(m, visitor, is_char_literal<Matcher2, char_type>());
        }

        template<typename Matcher2, typename Visitor>
        static type call_(Matcher2 const &m, Visitor &visitor, mpl::true_)
        {
            char_type ch = char_cast<char_type>(m, visitor.traits());
            return type(ch, visitor.traits());
        }

        template<typename Matcher2, typename Visitor>
        static type call_(Matcher2 const &m, Visitor &visitor, mpl::false_)
        {
            string_type str = string_cast<string_type>(m, visitor.traits());
            return type(str, visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits, typename Matcher>
    struct default_transmogrify<BidiIter, ICase, Traits, Matcher, typename Matcher::is_boost_xpressive_xpression_>
    {
        typedef Matcher type;

        template<typename Matcher2>
        static Matcher2 const &call(Matcher2 const &m, dont_care)
        {
            return m;
        }
    };

    template<typename BidiIter, typename ICase, typename Traits, typename Matcher>
    struct transmogrify
      : default_transmogrify<BidiIter, ICase, Traits, Matcher>
    {};

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, assert_bol_placeholder >
    {
        typedef assert_bol_matcher<Traits> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2, Visitor &visitor)
        {
            return type(visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, assert_eol_placeholder >
    {
        typedef assert_eol_matcher<Traits> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2, Visitor &visitor)
        {
            return type(visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, logical_newline_placeholder >
    {
        typedef logical_newline_matcher<Traits> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2, Visitor &visitor)
        {
            return type(visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits, typename Char>
    struct transmogrify<BidiIter, ICase, Traits, range_placeholder<Char> >
    {
        // By design, we don't widen character ranges.
        typedef typename iterator_value<BidiIter>::type char_type;
        BOOST_MPL_ASSERT((is_same<Char, char_type>));
        typedef range_matcher<Traits, ICase> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2 const &m, Visitor &visitor)
        {
            return type(m.ch_min_, m.ch_max_, m.not_, visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, mark_placeholder >
    {
        typedef mark_matcher<Traits, ICase> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2 const &m, Visitor &visitor)
        {
            return type(m.mark_number_, visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, posix_charset_placeholder >
    {
        typedef posix_charset_matcher<Traits> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2 const &m, Visitor &visitor)
        {
            char const *name_end = m.name_ + std::strlen(m.name_);
            return type(visitor.traits().lookup_classname(m.name_, name_end, ICase::value), m.not_);
        }
    };

    template<typename BidiIter, typename Traits, typename Size>
    struct transmogrify<BidiIter, mpl::true_, Traits, set_matcher<Traits, Size> >
    {
        typedef set_matcher<Traits, Size> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2 m, Visitor &visitor)
        {
            m.nocase(visitor.traits());
            return m;
        }
    };

    template<typename BidiIter, typename ICase, typename Traits, typename Cond>
    struct transmogrify<BidiIter, ICase, Traits, assert_word_placeholder<Cond> >
    {
        typedef assert_word_matcher<Cond, Traits> type;

        template<typename Visitor>
        static type call(dont_care, Visitor &visitor)
        {
            return type(visitor.traits());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, reference_wrapper<basic_regex<BidiIter> > >
    {
        typedef regex_byref_matcher<BidiIter> type;

        template<typename Matcher2>
        static type call(Matcher2 const &m, dont_care)
        {
            return type(detail::core_access<BidiIter>::get_regex_impl(m.get()));
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, reference_wrapper<basic_regex<BidiIter> const> >
    {
        typedef regex_byref_matcher<BidiIter> type;

        template<typename Matcher2>
        static type call(Matcher2 const &m, dont_care)
        {
            return type(detail::core_access<BidiIter>::get_regex_impl(m.get()));
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, tracking_ptr<regex_impl<BidiIter> > >
    {
        typedef regex_matcher<BidiIter> type;

        template<typename Matcher2>
        static type call(Matcher2 const &m, dont_care)
        {
            return type(m.get());
        }
    };

    template<typename BidiIter, typename ICase, typename Traits>
    struct transmogrify<BidiIter, ICase, Traits, self_placeholder >
    {
        typedef regex_byref_matcher<BidiIter> type;

        template<typename Matcher2, typename Visitor>
        static type call(Matcher2, Visitor &visitor)
        {
            return type(visitor.self());
        }
    };

}}}

#endif
