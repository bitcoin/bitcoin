
#ifndef BOOST_MPL_AUX_COUNT_IMPL_HPP_INCLUDED
#define BOOST_MPL_AUX_COUNT_IMPL_HPP_INCLUDED

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

#include <boost/mpl/count_fwd.hpp>
#include <boost/mpl/count_if.hpp>
#include <boost/mpl/same_as.hpp>
#include <boost/mpl/aux_/config/static_constant.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>
#include <boost/mpl/aux_/traits_lambda_spec.hpp>

namespace boost { namespace mpl {

template< typename Tag > struct count_impl
{
    template< typename Sequence, typename T > struct apply
#if BOOST_WORKAROUND(BOOST_BORLANDC,BOOST_TESTED_AT(0x561))
    {
        typedef typename count_if< Sequence,same_as<T> >::type type;
        BOOST_STATIC_CONSTANT(int, value = BOOST_MPL_AUX_VALUE_WKND(type)::value);
#else
        : count_if< Sequence,same_as<T> >
    {
#endif
    };
};

BOOST_MPL_ALGORITM_TRAITS_LAMBDA_SPEC(2,count_impl)

}}

#endif // BOOST_MPL_AUX_COUNT_IMPL_HPP_INCLUDED
