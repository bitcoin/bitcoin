
#ifndef BOOST_MPL_AUX_JOINT_ITER_HPP_INCLUDED
#define BOOST_MPL_AUX_JOINT_ITER_HPP_INCLUDED

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

#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/iterator_tags.hpp>
#include <boost/mpl/aux_/lambda_spec.hpp>
#include <boost/mpl/aux_/config/ctps.hpp>

#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
#   include <boost/type_traits/is_same.hpp>
#endif

namespace boost { namespace mpl {

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

template<
      typename Iterator1
    , typename LastIterator1
    , typename Iterator2
    >
struct joint_iter
{
    typedef Iterator1 base;
    typedef forward_iterator_tag category;
};

template<
      typename LastIterator1
    , typename Iterator2
    >
struct joint_iter<LastIterator1,LastIterator1,Iterator2>
{
    typedef Iterator2 base;
    typedef forward_iterator_tag category;
};


template< typename I1, typename L1, typename I2 >
struct deref< joint_iter<I1,L1,I2> >
{
    typedef typename joint_iter<I1,L1,I2>::base base_;
    typedef typename deref<base_>::type type;
};

template< typename I1, typename L1, typename I2 >
struct next< joint_iter<I1,L1,I2> >
{
    typedef joint_iter< typename mpl::next<I1>::type,L1,I2 > type;
};

template< typename L1, typename I2 >
struct next< joint_iter<L1,L1,I2> >
{
    typedef joint_iter< L1,L1,typename mpl::next<I2>::type > type;
};

#else // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

template<
      typename Iterator1
    , typename LastIterator1
    , typename Iterator2
    >
struct joint_iter;

template< bool > struct joint_iter_impl
{
    template< typename I1, typename L1, typename I2 > struct result_
    {
        typedef I1 base;
        typedef forward_iterator_tag category;
        typedef joint_iter< typename mpl::next<I1>::type,L1,I2 > next;
        typedef typename deref<I1>::type type;
    };
};

template<> struct joint_iter_impl<true>
{
    template< typename I1, typename L1, typename I2 > struct result_
    {
        typedef I2 base;
        typedef forward_iterator_tag category;
        typedef joint_iter< L1,L1,typename mpl::next<I2>::type > next;
        typedef typename deref<I2>::type type;
    };
};

template<
      typename Iterator1
    , typename LastIterator1
    , typename Iterator2
    >
struct joint_iter
    : joint_iter_impl< is_same<Iterator1,LastIterator1>::value >
        ::template result_<Iterator1,LastIterator1,Iterator2>
{
};

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

BOOST_MPL_AUX_PASS_THROUGH_LAMBDA_SPEC(3, joint_iter)

}}

#endif // BOOST_MPL_AUX_JOINT_ITER_HPP_INCLUDED
