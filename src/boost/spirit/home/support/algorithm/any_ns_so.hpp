/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ANY_NS_SO_DECEMBER_03_2017_0826PM)
#define BOOST_SPIRIT_ANY_NS_SO_DECEMBER_03_2017_0826PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/fusion/include/equal_to.hpp>
#include <boost/fusion/include/next.hpp>
#include <boost/fusion/include/deref.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/end.hpp>
#include <boost/fusion/include/any.hpp>
#include <boost/spirit/home/support/unused.hpp>

namespace boost { namespace spirit
{
    // A non-short circuiting (ns) strict order (so) version of the any
    // algorithm

    namespace detail
    {
        template <typename First1, typename Last, typename First2, typename F>
        inline bool
        any_ns_so(First1 const&, First2 const&, Last const&, F const&, mpl::true_)
        {
            return false;
        }

        template <typename First1, typename Last, typename First2, typename F>
        inline bool
        any_ns_so(First1 const& first1, First2 const& first2, Last const& last, F& f, mpl::false_)
        {
            bool head = f(*first1, *first2);
            bool tail =
                detail::any_ns_so(
                    fusion::next(first1)
                  , fusion::next(first2)
                  , last
                  , f
                  , fusion::result_of::equal_to<
                        typename fusion::result_of::next<First1>::type, Last>());
            return head || tail;
        }

        template <typename First, typename Last, typename F>
        inline bool
        any_ns_so(First const&, Last const&, F const&, mpl::true_)
        {
            return false;
        }

        template <typename First, typename Last, typename F>
        inline bool
        any_ns_so(First const& first, Last const& last, F& f, mpl::false_)
        {
            bool head = f(*first);
            bool tail =
                detail::any_ns_so(
                    fusion::next(first)
                  , last
                  , f
                  , fusion::result_of::equal_to<
                        typename fusion::result_of::next<First>::type, Last>());
            return head || tail;
        }
    }

    template <typename Sequence1, typename Sequence2, typename F>
    inline bool
    any_ns_so(Sequence1 const& seq1, Sequence2& seq2, F f)
    {
        return detail::any_ns_so(
                fusion::begin(seq1)
              , fusion::begin(seq2)
              , fusion::end(seq1)
              , f
              , fusion::result_of::equal_to<
                    typename fusion::result_of::begin<Sequence1>::type
                  , typename fusion::result_of::end<Sequence1>::type>());
    }

    template <typename Sequence, typename F>
    inline bool
    any_ns_so(Sequence const& seq, unused_type, F f)
    {
        return detail::any_ns_so(
                fusion::begin(seq)
              , fusion::end(seq)
              , f
              , fusion::result_of::equal_to<
                    typename fusion::result_of::begin<Sequence>::type
                  , typename fusion::result_of::end<Sequence>::type>());
    }

}}

#endif

