
#ifndef BOOST_MPL_UNIQUE_HPP_INCLUDED
#define BOOST_MPL_UNIQUE_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
// Copyright John R. Bandela 2000-2002
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
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/aux_/inserter_algorithm.hpp>
#include <boost/mpl/aux_/na.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/lambda_spec.hpp>

namespace boost { namespace mpl {

namespace aux {

template< typename Predicate, typename Operation >
struct unique_op
{
    template< typename Pair, typename T > struct apply
    {
        typedef typename Pair::first seq_;
        typedef typename Pair::second prior_;
        typedef typename eval_if<
              and_< is_not_na<prior_>, apply2<Predicate,prior_,T> >
            , identity<seq_>
            , apply2<Operation,seq_,T>
            >::type new_seq_;

        typedef pair<new_seq_,T> type;
    };
};

template<
      typename Sequence
    , typename Predicate
    , typename Inserter
    >
struct unique_impl
    : first< typename fold<
          Sequence
        , pair< typename Inserter::state,na >
        , protect< aux::unique_op<Predicate,typename Inserter::operation> >
        >::type >
{
};

template<
      typename Sequence
    , typename Predicate
    , typename Inserter
    >
struct reverse_unique_impl
    : first< typename reverse_fold<
          Sequence
        , pair< typename Inserter::state,na >
        , protect< aux::unique_op<Predicate,typename Inserter::operation> >
        >::type >
{
};

} // namespace aux

BOOST_MPL_AUX_INSERTER_ALGORITHM_DEF(3, unique)

}}

#endif // BOOST_MPL_UNIQUE_HPP_INCLUDED
