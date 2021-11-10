/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_COUNT_IF_09162005_0141)
#define BOOST_FUSION_COUNT_IF_09162005_0141

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/fusion/iterator/advance.hpp>

namespace boost { namespace fusion { 
    struct random_access_traversal_tag;
namespace detail
{
    template <typename First, typename Last, typename F>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline int
    linear_count_if(First const&, Last const&, F const&, mpl::true_)
    {
        return 0;
    }

    template <typename First, typename Last, typename F>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline int
    linear_count_if(First const& first, Last const& last, F& f, mpl::false_)
    {
        int n =
            detail::linear_count_if(
                fusion::next(first)
              , last
              , f
              , result_of::equal_to<typename result_of::next<First>::type, Last>());
        if (f(*first))
            ++n;
        return n;
    }

    template <typename Sequence, typename F, typename Tag>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline int
    count_if(Sequence const& seq, F f, Tag)
    {
        return detail::linear_count_if(
                fusion::begin(seq)
              , fusion::end(seq)
              , f
              , result_of::equal_to<
                    typename result_of::begin<Sequence>::type
                  , typename result_of::end<Sequence>::type>());
    }

    template<int n>
    struct unrolled_count_if
    {
        template<typename I0, typename F>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static int call(I0 const& i0, F f)
        {
            int ct = unrolled_count_if<n-4>::
                call(fusion::advance_c<4>(i0), f);
            if(f(*i0))
                ++ct;

            typedef typename result_of::next<I0>::type I1;
            I1 i1(fusion::next(i0));
            if(f(*i1))
                ++ct;

            typedef typename result_of::next<I1>::type I2;
            I2 i2(fusion::next(i1));
            if(f(*i2))
                ++ct;

            typedef typename result_of::next<I2>::type I3;
            I3 i3(fusion::next(i2));
            if(f(*i3))
                ++ct;

            return ct;
        }
    };

    template<>
    struct unrolled_count_if<3>
    {
        template<typename I0, typename F>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static int call(I0 const& i0, F f)
        {
            int ct = 0;
            if(f(*i0))
                ++ct;

            typedef typename result_of::next<I0>::type I1;
            I1 i1(fusion::next(i0));
            if(f(*i1))
                ++ct;

            typedef typename result_of::next<I1>::type I2;
            I2 i2(fusion::next(i1));
            if(f(*i2))
                ++ct;

            return ct;
        }
    };

    template<>
    struct unrolled_count_if<2>
    {
        template<typename I0, typename F>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static int call(I0 const& i0, F f)
        {
            int ct = 0;

            if(f(*i0))
                ++ct;

            typedef typename result_of::next<I0>::type I1;
            I1 i1(fusion::next(i0));
            if(f(*i1))
                ++ct;

            return ct;
        }
    };

    template<>
    struct unrolled_count_if<1>
    {
        template<typename I0, typename F>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static int call(I0 const& i0, F f)
        {
            int ct = 0;
            if(f(*i0))
                ++ct;
            return ct;
        }
    };


    template<>
    struct unrolled_count_if<0>
    {
        template<typename I0, typename F>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static int call(I0 const&, F)
        {
            return 0;
        }
    };

    template <typename Sequence, typename F>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline int
    count_if(Sequence const& seq, F f, random_access_traversal_tag)
    {
        typedef typename result_of::begin<Sequence>::type begin;
        typedef typename result_of::end<Sequence>::type end;
        return detail::unrolled_count_if<result_of::distance<begin, end>::type::value>::
            call(fusion::begin(seq), f);
    }
}}}

#endif

