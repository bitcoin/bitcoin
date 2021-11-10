
#ifndef BOOST_MPL_COPY_IF_HPP_INCLUDED
#define BOOST_MPL_COPY_IF_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/fold.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/protect.hpp>
#include <boost/mpl/aux_/inserter_algorithm.hpp>
#include <boost/mpl/aux_/config/forwarding.hpp>

namespace boost { namespace mpl {

namespace aux {

template<
      typename Operation
    , typename Predicate
    >
struct copy_if_op
{
    template< typename Sequence, typename T > struct apply
#if !defined(BOOST_MPL_CFG_NO_NESTED_FORWARDING)
        : eval_if<
              typename apply1<Predicate,T>::type
            , apply2<Operation,Sequence,T>
            , identity<Sequence>
            >
    {
#else
    {
        typedef typename eval_if<
              typename apply1<Predicate,T>::type
            , apply2<Operation,Sequence,T>
            , identity<Sequence>
            >::type type;
#endif
    };
};

template<
      typename Sequence
    , typename Predicate
    , typename Inserter
    >
struct copy_if_impl
    : fold<
          Sequence
        , typename Inserter::state
        , protect< aux::copy_if_op<
              typename Inserter::operation
            , Predicate
            > >
        >
{
};

template<
      typename Sequence
    , typename Predicate
    , typename Inserter
    >
struct reverse_copy_if_impl
    : reverse_fold<
          Sequence
        , typename Inserter::state
        , protect< aux::copy_if_op<
              typename Inserter::operation
            , Predicate
            > >
        >
{
};

} // namespace aux

BOOST_MPL_AUX_INSERTER_ALGORITHM_DEF(3, copy_if)

}}

#endif // BOOST_MPL_COPY_IF_HPP_INCLUDED
