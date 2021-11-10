/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_INSERT_07222005_0730)
#define FUSION_INSERT_07222005_0730

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/detail/as_fusion_element.hpp>
#include <boost/fusion/iterator/mpl/convert_iterator.hpp>
#include <boost/fusion/view/joint_view/joint_view.hpp>
#include <boost/fusion/view/single_view/single_view.hpp>
#include <boost/fusion/view/iterator_range/iterator_range.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/adapted/mpl/mpl_iterator.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence, typename Position, typename T>
        struct insert
        {
            typedef typename detail::as_fusion_element<T>::type element_type;
            typedef typename convert_iterator<Position>::type pos_type;
            typedef typename result_of::begin<Sequence>::type first_type;
            typedef typename result_of::end<Sequence>::type last_type;

            typedef iterator_range<first_type, pos_type> left_type;
            typedef iterator_range<pos_type, last_type> right_type;
            typedef fusion::single_view<element_type> single_view;
            typedef joint_view<left_type, single_view const> left_insert_type;
            typedef joint_view<left_insert_type, right_type> type;
        };
    }

    template <typename Sequence, typename Position, typename T>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename
        lazy_enable_if<
            traits::is_sequence<Sequence>
          , result_of::insert<Sequence const, Position, T>
        >::type
    insert(Sequence const& seq, Position const& pos, T const& x)
    {
        typedef result_of::insert<
            Sequence const, Position, T>
        result_of;
        typedef typename result_of::left_type left_type;
        typedef typename result_of::right_type right_type;
        typedef typename result_of::single_view single_view;
        typedef typename result_of::left_insert_type left_insert_type;
        typedef typename result_of::type result;

        left_type left(fusion::begin(seq), convert_iterator<Position>::call(pos));
        right_type right(convert_iterator<Position>::call(pos), fusion::end(seq));
        single_view insert(x);
        left_insert_type left_insert(left, insert);
        return result(left_insert, right);
    }
}}

#endif

