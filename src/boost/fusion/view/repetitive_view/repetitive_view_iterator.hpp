/*=============================================================================
    Copyright (c) 2007 Tobias Schwinger

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_REPETITIVE_VIEW_ITERATOR_HPP_INCLUDED
#define BOOST_FUSION_REPETITIVE_VIEW_ITERATOR_HPP_INCLUDED

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/iterator_base.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/iterator/mpl/convert_iterator.hpp>
#include <boost/fusion/adapted/mpl/mpl_iterator.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/view/repetitive_view/detail/deref_impl.hpp>
#include <boost/fusion/view/repetitive_view/detail/next_impl.hpp>
#include <boost/fusion/view/repetitive_view/detail/value_of_impl.hpp>

namespace boost { namespace fusion
{
    struct repetitive_view_iterator_tag;

    template<typename Sequence, typename Pos =
        typename result_of::begin<Sequence>::type>
    struct repetitive_view_iterator
        : iterator_base< repetitive_view_iterator<Sequence,Pos> >
    {
        typedef repetitive_view_iterator_tag fusion_tag;

        typedef Sequence sequence_type;
        typedef typename convert_iterator<Pos>::type pos_type;
        typedef typename convert_iterator<typename result_of::begin<Sequence>::type>::type first_type;
        typedef typename convert_iterator<typename result_of::end<Sequence>::type>::type end_type;
        typedef single_pass_traversal_tag category;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        explicit repetitive_view_iterator(Sequence& in_seq)
            : seq(in_seq), pos(begin(in_seq)) {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        repetitive_view_iterator(Sequence& in_seq, pos_type const& in_pos)
            : seq(in_seq), pos(in_pos) {}

        Sequence& seq;
        pos_type pos;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(repetitive_view_iterator& operator= (repetitive_view_iterator const&))
    };
}}

#ifdef BOOST_FUSION_WORKAROUND_FOR_LWG_2408
namespace std
{
    template <typename Sequence, typename Pos>
    struct iterator_traits< ::boost::fusion::repetitive_view_iterator<Sequence, Pos> >
    { };
}
#endif

#endif

