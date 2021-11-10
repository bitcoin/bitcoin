
#ifndef BOOST_MPL_AUX_INSERT_RANGE_IMPL_HPP_INCLUDED
#define BOOST_MPL_AUX_INSERT_RANGE_IMPL_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/clear.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/iterator_range.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/iter_push_front.hpp>
#include <boost/mpl/aux_/traits_lambda_spec.hpp>
#include <boost/mpl/aux_/config/forwarding.hpp>

#include <boost/type_traits/same_traits.hpp>

namespace boost { namespace mpl {

// default implementation; conrete sequences might override it by 
// specializing either the 'insert_range_impl' or the primary 
// 'insert_range' template


template< typename Tag >
struct insert_range_impl
{
    template<
          typename Sequence
        , typename Pos
        , typename Range
        >
    struct apply
#if !defined(BOOST_MPL_CFG_NO_NESTED_FORWARDING)
        : reverse_fold<
              joint_view<
                  iterator_range<typename begin<Sequence>::type,Pos>
                , joint_view<
                      Range
                    , iterator_range<Pos,typename end<Sequence>::type>
                    >
                >
            , typename clear<Sequence>::type
            , insert<_1, begin<_1>, _2>
            >
    {
#else
    {
        typedef typename reverse_fold<
                joint_view<
                    iterator_range<typename begin<Sequence>::type,Pos>
                  , joint_view<
                        Range
                      , iterator_range<Pos,typename end<Sequence>::type>
                      >
                  >
              , typename clear<Sequence>::type
              , insert<_1, begin<_1>, _2>
              >::type type;
#endif
    };
};

BOOST_MPL_ALGORITM_TRAITS_LAMBDA_SPEC(3,insert_range_impl)

}}

#endif // BOOST_MPL_AUX_INSERT_RANGE_IMPL_HPP_INCLUDED
