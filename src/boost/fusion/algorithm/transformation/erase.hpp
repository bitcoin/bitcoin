/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_ERASE_07232005_0534)
#define FUSION_ERASE_07232005_0534

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/mpl/convert_iterator.hpp>
#include <boost/fusion/view/joint_view/joint_view.hpp>
#include <boost/fusion/view/iterator_range/iterator_range.hpp>
#include <boost/fusion/support/detail/as_fusion_element.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/adapted/mpl/mpl_iterator.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/if.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence, typename First>
        struct compute_erase_last // put this in detail!!!
        {
            typedef typename result_of::end<Sequence>::type seq_last_type;
            typedef typename convert_iterator<First>::type first_type;
            typedef typename
                mpl::if_<
                    result_of::equal_to<first_type, seq_last_type>
                  , first_type
                  , typename result_of::next<first_type>::type
                >::type
            type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(First const& first, mpl::false_)
            {
                return fusion::next(convert_iterator<First>::call(first));
            }

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(First const& first, mpl::true_)
            {
                return convert_iterator<First>::call(first);
            }

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(First const& first)
            {
                return call(first, result_of::equal_to<first_type, seq_last_type>());
            }
        };

        struct use_default;

        template <class T, class Default>
        struct fusion_default_help
          : mpl::if_<
                is_same<T, use_default>
              , Default
              , T
            >
        {
        };

        template <
            typename Sequence
          , typename First
          , typename Last = use_default>
        struct erase
        {
            typedef typename result_of::begin<Sequence>::type seq_first_type;
            typedef typename result_of::end<Sequence>::type seq_last_type;
            BOOST_STATIC_ASSERT((!result_of::equal_to<seq_first_type, seq_last_type>::value));

            typedef First FirstType;
            typedef typename 
                fusion_default_help<
                    Last 
                  , typename compute_erase_last<Sequence, First>::type
                >::type
            LastType;

            typedef typename convert_iterator<FirstType>::type first_type;
            typedef typename convert_iterator<LastType>::type last_type;
            typedef iterator_range<seq_first_type, first_type> left_type;
            typedef iterator_range<last_type, seq_last_type> right_type;
            typedef joint_view<left_type, right_type> type;
        };
    }

    template <typename Sequence, typename First>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename
        lazy_enable_if<
            traits::is_sequence<Sequence>
          , typename result_of::erase<Sequence const, First> 
        >::type
    erase(Sequence const& seq, First const& first)
    {
        typedef result_of::erase<Sequence const, First> result_of;
        typedef typename result_of::left_type left_type;
        typedef typename result_of::right_type right_type;
        typedef typename result_of::type result_type;

        left_type left(
            fusion::begin(seq)
          , convert_iterator<First>::call(first));
        right_type right(
            fusion::result_of::compute_erase_last<Sequence const, First>::call(first)
          , fusion::end(seq));
        return result_type(left, right);
    }

    template <typename Sequence, typename First, typename Last>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::erase<Sequence const, First, Last>::type
    erase(Sequence const& seq, First const& first, Last const& last)
    {
        typedef result_of::erase<Sequence const, First, Last> result_of;
        typedef typename result_of::left_type left_type;
        typedef typename result_of::right_type right_type;
        typedef typename result_of::type result_type;

        left_type left(fusion::begin(seq), first);
        right_type right(last, fusion::end(seq));
        return result_type(left, right);
    }
}}

#endif

