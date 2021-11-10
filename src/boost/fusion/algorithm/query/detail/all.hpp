/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_ALL_05052005_1237)
#define FUSION_ALL_05052005_1237

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/distance.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename First, typename Last, typename F>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline bool
    linear_all(First const&, Last const&, F const&, mpl::true_)
    {
        return true;
    }

    template <typename First, typename Last, typename F>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline bool
    linear_all(First const& first, Last const& last, F& f, mpl::false_)
    {
        typename result_of::deref<First>::type x = *first;
        return f(x) &&
            detail::linear_all(
                fusion::next(first)
              , last
              , f
              , result_of::equal_to<typename result_of::next<First>::type, Last>());
    }

    template <typename Sequence, typename F, typename Tag>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline bool
    all(Sequence const& seq, F f, Tag)
    {
        return detail::linear_all(
                fusion::begin(seq)
              , fusion::end(seq)
              , f
              , result_of::equal_to<
                    typename result_of::begin<Sequence>::type
                  , typename result_of::end<Sequence>::type>());
    }

    template<int N>
    struct unrolled_all
    {
        template <typename It, typename F>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static bool call(It const& it, F f)
        {
            return
                f(*it) &&
                f(*fusion::advance_c<1>(it))&&
                f(*fusion::advance_c<2>(it)) &&
                f(*fusion::advance_c<3>(it)) &&
                detail::unrolled_all<N-4>::call(fusion::advance_c<4>(it), f);
        }
    };

    template<>
    struct unrolled_all<3>
    {
        template <typename It, typename F>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static bool call(It const& it, F f)
        {
            return
                f(*it) &&
                f(*fusion::advance_c<1>(it)) &&
                f(*fusion::advance_c<2>(it));
        }
    };

    template<>
    struct unrolled_all<2>
    {
        template <typename It, typename F>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static bool call(It const& it, F f)
        {
            return
                f(*it) &&
                f(*fusion::advance_c<1>(it));
        }
    };

    template<>
    struct unrolled_all<1>
    {
        template <typename It, typename F>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static bool call(It const& it, F f)
        {
            return f(*it);
        }
    };

    template<>
    struct unrolled_all<0>
    {
        template <typename It, typename F>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static bool call(It const& /*it*/, F /*f*/)
        {
            return true;
        }
    };

    template <typename Sequence, typename F>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline bool
    all(Sequence const& seq, F f, random_access_traversal_tag)
    {
        typedef typename result_of::begin<Sequence>::type begin;
        typedef typename result_of::end<Sequence>::type end;
        return detail::unrolled_all<result_of::distance<begin, end>::type::value>::call(
            fusion::begin(seq), f);
    }
}}}

#endif

