/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ANY_IF_NS_SO_DECEMBER_03_2017_0826PM)
#define BOOST_SPIRIT_ANY_IF_NS_SO_DECEMBER_03_2017_0826PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/algorithm/any_if.hpp>
#include <boost/spirit/home/support/algorithm/any_ns_so.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    //  This is a special version for a binary fusion::any. The predicate
    //  is used to decide whether to advance the second iterator or not.
    //  This is needed for sequences containing components with unused
    //  attributes. The second iterator is advanced only if the attribute
    //  of the corresponding component iterator is not unused.
    //
    //  This is a non-short circuiting (ns) strict order (so) version of the
    //  any_if algorithm.
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <
            typename Pred, typename First1, typename Last1, typename First2
          , typename Last2, typename F
        >
        inline bool
        any_if_ns_so(First1 const&, First2 const&, Last1 const&, Last2 const&
          , F const&, mpl::true_)
        {
            return false;
        }

        template <
            typename Pred, typename First1, typename Last1, typename First2
          , typename Last2, typename F
        >
        inline bool
        any_if_ns_so(First1 const& first1, First2 const& first2
          , Last1 const& last1, Last2 const& last2, F& f, mpl::false_)
        {
            typename result_of::attribute_value<First1, First2, Last2, Pred>::type
                attribute = spirit::detail::attribute_value<Pred, First1, Last2>(first2);

            bool head = f(*first1, attribute);
            bool tail =
                detail::any_if_ns_so<Pred>(
                    fusion::next(first1)
                  , attribute_next<Pred, First1, Last2>(first2)
                  , last1, last2
                  , f
                  , fusion::result_of::equal_to<
                        typename fusion::result_of::next<First1>::type, Last1>());
            return head || tail;
        }
    }

    template <typename Pred, typename Sequence1, typename Sequence2, typename F>
    inline bool
    any_if_ns_so(Sequence1 const& seq1, Sequence2& seq2, F f, Pred)
    {
        return detail::any_if_ns_so<Pred>(
                fusion::begin(seq1), fusion::begin(seq2)
              , fusion::end(seq1), fusion::end(seq2)
              , f
              , fusion::result_of::equal_to<
                    typename fusion::result_of::begin<Sequence1>::type
                  , typename fusion::result_of::end<Sequence1>::type>());
    }

    template <typename Pred, typename Sequence, typename F>
    inline bool
    any_if_ns_so(Sequence const& seq, unused_type const, F f, Pred)
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

