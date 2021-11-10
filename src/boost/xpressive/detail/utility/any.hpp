///////////////////////////////////////////////////////////////////////////////
// any.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_ANY_HPP_EAN_11_19_2005
#define BOOST_XPRESSIVE_DETAIL_UTILITY_ANY_HPP_EAN_11_19_2005

#include <boost/version.hpp>

#if BOOST_VERSION >= 103300

// In Boost 1.33+, we have a cons list in Fusion, so just include it.

# if BOOST_VERSION >= 103500
#  include <boost/fusion/include/any.hpp> // Boost 1.35+ has Fusion2
# else
#  include <boost/spirit/fusion/algorithm/any.hpp> // Fusion1
# endif

#else

# include <boost/spirit/fusion/sequence/begin.hpp>
# include <boost/spirit/fusion/sequence/end.hpp>
# include <boost/spirit/fusion/iterator/equal_to.hpp>
# include <boost/mpl/bool.hpp>
# include <boost/spirit/fusion/iterator/equal_to.hpp>
# include <boost/spirit/fusion/iterator/next.hpp>
# include <boost/spirit/fusion/iterator/deref.hpp>

namespace boost { namespace fusion
{

    namespace detail
    {
        template <typename First, typename Last, typename F>
        inline bool
        any(First const&, Last const&, F const&, mpl::true_)
        {
            return false;
        }

        template <typename First, typename Last, typename F>
        inline bool
        any(First const& first, Last const& last, F const& f, mpl::false_)
        {
            if(f(*first))
                return true;
            return detail::any(fusion::next(first), last, f
              , meta::equal_to<BOOST_DEDUCED_TYPENAME meta::next<First>::type, Last>());
        }
    }

    namespace meta
    {
        template <typename Sequence, typename F>
        struct any
        {
            typedef bool type;
        };
    }

    namespace function
    {
        struct any
        {
            template <typename Sequence, typename F>
            struct apply
            {
                typedef bool type;
            };

            template <typename Sequence, typename F>
            inline bool
            operator()(Sequence const& seq, F const& f) const
            {
                return detail::any(
                        fusion::begin(seq)
                      , fusion::end(seq)
                      , f
                      , meta::equal_to<
                            BOOST_DEDUCED_TYPENAME meta::begin<Sequence>::type
                          , BOOST_DEDUCED_TYPENAME meta::end<Sequence>::type>());
            }

            template <typename Sequence, typename F>
            inline bool
            operator()(Sequence& seq, F const& f) const
            {
                return detail::any(
                        fusion::begin(seq)
                      , fusion::end(seq)
                      , f
                      , meta::equal_to<
                            BOOST_DEDUCED_TYPENAME meta::begin<Sequence>::type
                          , BOOST_DEDUCED_TYPENAME meta::end<Sequence>::type>());
            }
        };
    }

    function::any const any = function::any();
}}

#endif

#endif
