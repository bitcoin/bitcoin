/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_POP_BACK_09172005_1038)
#define FUSION_POP_BACK_09172005_1038

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/view/iterator_range/iterator_range.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/iterator/iterator_adapter.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace fusion
{
    template <typename Iterator_, bool IsLast>
    struct pop_back_iterator
        : iterator_adapter<
            pop_back_iterator<Iterator_, IsLast>
          , Iterator_>
    {
        typedef iterator_adapter<
            pop_back_iterator<Iterator_, IsLast>
          , Iterator_>
        base_type;

        static bool const is_last = IsLast;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        pop_back_iterator(Iterator_ const& iterator_base)
            : base_type(iterator_base) {}

        template <typename BaseIterator>
        struct make
        {
            typedef pop_back_iterator<BaseIterator, is_last> type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(BaseIterator const& i)
            {
                return type(i);
            }
        };

        template <typename I, bool IsLast_>
        struct equal_to_helper
            : mpl::identity<typename I::iterator_base_type>
        {};

        template <typename I>
        struct equal_to_helper<I, true>
            : result_of::next<
                typename I::iterator_base_type>
        {};

        template <typename I1, typename I2>
        struct equal_to
            : result_of::equal_to<
                typename equal_to_helper<I1,
                    (I2::is_last && !I1::is_last)>::type
              , typename equal_to_helper<I2,
                    (I1::is_last && !I2::is_last)>::type
            >
        {};

        template <typename First, typename Last>
        struct distance
            : mpl::minus<
                typename result_of::distance<
                    typename First::iterator_base_type
                  , typename Last::iterator_base_type
                >::type
              , mpl::int_<(Last::is_last?1:0)>
            >::type
        {};


        template <typename Iterator, bool IsLast_>
        struct prior_impl
        {
            typedef typename Iterator::iterator_base_type base_type;

            typedef typename
                result_of::prior<base_type>::type
            base_prior;

            typedef pop_back_iterator<base_prior, false> type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(Iterator const& i)
            {
                return type(fusion::prior(i.iterator_base));
            }
        };

        template <typename Iterator>
        struct prior_impl<Iterator, true>
        {
            // If this is the last iterator, we'll have to double back
            typedef typename Iterator::iterator_base_type base_type;

            typedef typename
                result_of::prior<
                  typename result_of::prior<base_type>::type
                >::type
            base_prior;

            typedef pop_back_iterator<base_prior, false> type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(Iterator const& i)
            {
                return type(fusion::prior(
                    fusion::prior(i.iterator_base)));
            }
        };

        template <typename Iterator>
        struct prior : prior_impl<Iterator, Iterator::is_last>
        {};
    };

    namespace result_of
    {
        template <typename Sequence>
        struct pop_back
        {
            BOOST_MPL_ASSERT_NOT((result_of::empty<Sequence>));

            typedef pop_back_iterator<
                typename begin<Sequence>::type, false>
            begin_type;

            typedef pop_back_iterator<
                typename end<Sequence>::type, true>
            end_type;

            typedef
                iterator_range<begin_type, end_type>
            type;
        };
    }

    template <typename Sequence>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::pop_back<Sequence const>::type
    pop_back(Sequence const& seq)
    {
        typedef result_of::pop_back<Sequence const> comp;
        typedef typename comp::begin_type begin_type;
        typedef typename comp::end_type end_type;
        typedef typename comp::type result;

        return result(
            begin_type(fusion::begin(seq))
          , end_type(fusion::end(seq))
        );
    }
}}

#endif

