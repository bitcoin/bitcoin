/*=============================================================================
    Copyright (c) 2012-2014 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_BUILD_CONS_10172012_0130)
#define BOOST_FUSION_BUILD_CONS_10172012_0130

#include <boost/tuple/tuple.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/value_of.hpp>
#include <boost/fusion/iterator/deref.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <
        typename First
      , typename Last
      , bool is_empty = result_of::equal_to<First, Last>::value>
    struct build_tuple_cons;

    template <typename First, typename Last>
    struct build_tuple_cons<First, Last, true>
    {
        typedef boost::tuples::null_type type;

        BOOST_FUSION_GPU_ENABLED
        static type
        call(First const&, Last const&)
        {
            return type();
        }
    };

    template <typename First, typename Last>
    struct build_tuple_cons<First, Last, false>
    {
        typedef
            build_tuple_cons<typename result_of::next<First>::type, Last>
        next_build_tuple_cons;

        typedef boost::tuples::cons<
            typename result_of::value_of<First>::type
          , typename next_build_tuple_cons::type>
        type;

        BOOST_FUSION_GPU_ENABLED
        static type
        call(First const& f, Last const& l)
        {
            typename result_of::value_of<First>::type v = *f;
            return type(v, next_build_tuple_cons::call(fusion::next(f), l));
        }
    };
}}}

#endif
