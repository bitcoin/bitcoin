/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2007 Dan Marsden
    Copyright (c) 2009 Christopher Schmidt
    Copyright (c) 2018 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_FIND_IF_05052005_1107)
#define FUSION_FIND_IF_05052005_1107

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/or.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/core/enable_if.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename Iterator, typename Pred>
    struct apply_filter
    {
        typedef typename mpl::apply1<
            Pred, Iterator>::type type;
        BOOST_STATIC_CONSTANT(int, value = type::value);
    };

    template <typename First, typename Last, typename Pred>
    struct main_find_if;

    template <typename First, typename Last, typename Pred>
    struct recursive_find_if
    {
        typedef typename
            main_find_if<
                typename result_of::next<First>::type, Last, Pred
            >::type
        type;
    };

    template <typename First, typename Last, typename Pred>
    struct main_find_if
    {
        typedef mpl::or_<
            result_of::equal_to<First, Last>
          , apply_filter<First, Pred> >
        filter;

        typedef typename
            mpl::eval_if<
                filter
              , mpl::identity<First>
              , recursive_find_if<First, Last, Pred>
            >::type
        type;
    };

    template<
        typename First, typename Last, 
        typename Pred, bool>
    struct choose_find_if;

    template<typename First, typename Last, typename Pred>
    struct choose_find_if<First, Last, Pred, false>
        : main_find_if<First, Last, Pred>
    {};

    template<typename Iter, typename Pred, int n, int unrolling>
    struct unroll_again;

    template <typename Iter, typename Pred, int offset>
    struct apply_offset_filter
    {
        typedef typename result_of::advance_c<Iter, offset>::type Shifted;
        typedef typename
            mpl::apply1<
                Pred
              , Shifted
            >::type
        type;
        BOOST_STATIC_CONSTANT(int, value = type::value);
    };

    template<typename Iter, typename Pred, int n>
    struct unrolled_find_if
    {
        typedef typename mpl::eval_if<
            apply_filter<Iter, Pred>,
            mpl::identity<Iter>,
            mpl::eval_if<
              apply_offset_filter<Iter, Pred, 1>,
              result_of::advance_c<Iter, 1>,
              mpl::eval_if<
                apply_offset_filter<Iter, Pred, 2>,
                result_of::advance_c<Iter, 2>,
                mpl::eval_if<
                  apply_offset_filter<Iter, Pred, 3>,
                  result_of::advance_c<Iter, 3>,
                  unroll_again<
                    Iter,
                    Pred,
                    n,
                    4> > > > >::type type;
    };

    template<typename Iter, typename Pred>
    struct unrolled_find_if<Iter, Pred, 3>
    {
        typedef typename mpl::eval_if<
            apply_filter<Iter, Pred>,
            mpl::identity<Iter>,
            mpl::eval_if<
              apply_offset_filter<Iter, Pred, 1>,
              result_of::advance_c<Iter, 1>,
              mpl::eval_if<
                apply_offset_filter<Iter, Pred, 2>,
                result_of::advance_c<Iter, 2>,
                result_of::advance_c<Iter, 3> > > >::type type;
    };

    template<typename Iter, typename Pred>
    struct unrolled_find_if<Iter, Pred, 2>
    {
        typedef typename mpl::eval_if<
            apply_filter<Iter, Pred>,
            mpl::identity<Iter>,
            mpl::eval_if<
              apply_offset_filter<Iter, Pred, 1>,
              result_of::advance_c<Iter, 1>,
              result_of::advance_c<Iter, 2> > >::type type;
    };

    template<typename Iter, typename Pred>
    struct unrolled_find_if<Iter, Pred, 1>
    {
        typedef typename mpl::eval_if<
            apply_filter<Iter, Pred>,
            mpl::identity<Iter>,
            result_of::advance_c<Iter, 1> >::type type;
    };

    template<typename Iter, typename Pred, int n, int unrolling>
    struct unroll_again
    {
        typedef typename unrolled_find_if<
            typename result_of::advance_c<Iter, unrolling>::type,
            Pred,
            n-unrolling>::type type;
    };

    template<typename Iter, typename Pred>
    struct unrolled_find_if<Iter, Pred, 0>
    {
        typedef Iter type;
    };

    template<typename First, typename Last, typename Pred>
    struct choose_find_if<First, Last, Pred, true>
    {
        typedef typename result_of::distance<First, Last>::type N;
        typedef typename unrolled_find_if<First, Pred, N::value>::type type;
    };

    template <typename First, typename Last, typename Pred>
    struct static_find_if
    {
        typedef typename
            choose_find_if<
                First
              , Last
              , Pred
              , traits::is_random_access<First>::value
            >::type
        type;

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type
        recursive_call(Iterator const& iter, mpl::true_)
        {
            return iter;
        }

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type
        recursive_call(Iterator const& iter, mpl::false_)
        {
            return recursive_call(fusion::next(iter));
        }

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type
        recursive_call(Iterator const& iter)
        {
            typedef result_of::equal_to<Iterator, type> found;
            return recursive_call(iter, found());
        }

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename boost::disable_if<traits::is_random_access<Iterator>, type>::type
        iter_call(Iterator const& iter)
        {
            return recursive_call(iter);
        }

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename boost::enable_if<traits::is_random_access<Iterator>, type>::type
        iter_call(Iterator const& iter)
        {
            typedef typename result_of::distance<Iterator, type>::type N;
            return fusion::advance<N>(iter);
        }

        template <typename Sequence>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type
        call(Sequence& seq)
        {
            return iter_call(fusion::begin(seq));
        }
    };

    template <typename Sequence, typename Pred>
    struct result_of_find_if
    {
        typedef
            static_find_if<
                typename result_of::begin<Sequence>::type
              , typename result_of::end<Sequence>::type
              , Pred
            >
        filter;

        typedef typename filter::type type;
    };
}}}

#endif
