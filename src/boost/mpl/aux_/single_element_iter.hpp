
#ifndef BOOST_MPL_AUX_SINGLE_ELEMENT_ITER_HPP_INCLUDED
#define BOOST_MPL_AUX_SINGLE_ELEMENT_ITER_HPP_INCLUDED

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

#include <boost/mpl/iterator_tags.hpp>
#include <boost/mpl/advance_fwd.hpp>
#include <boost/mpl/distance_fwd.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/aux_/nttp_decl.hpp>
#include <boost/mpl/aux_/value_wknd.hpp>
#include <boost/mpl/aux_/config/ctps.hpp>

namespace boost { namespace mpl { 

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

namespace aux {

template< typename T, BOOST_MPL_AUX_NTTP_DECL(int, is_last_) >
struct sel_iter;

template< typename T >
struct sel_iter<T,0>
{
    typedef random_access_iterator_tag category;
    typedef sel_iter<T,1> next;
    typedef T type;
};

template< typename T >
struct sel_iter<T,1>
{
    typedef random_access_iterator_tag category;
    typedef sel_iter<T,0> prior;
};

} // namespace aux

template< typename T, BOOST_MPL_AUX_NTTP_DECL(int, is_last_), typename Distance >
struct advance< aux::sel_iter<T,is_last_>,Distance>
{
    typedef aux::sel_iter<
          T
        , ( is_last_ + BOOST_MPL_AUX_NESTED_VALUE_WKND(int, Distance) )
        > type;
};

template< 
      typename T
    , BOOST_MPL_AUX_NTTP_DECL(int, l1)
    , BOOST_MPL_AUX_NTTP_DECL(int, l2) 
    >
struct distance< aux::sel_iter<T,l1>, aux::sel_iter<T,l2> >
    : int_<( l2 - l1 )>
{
};

#else

namespace aux {

struct sel_iter_tag;

template< typename T, BOOST_MPL_AUX_NTTP_DECL(int, is_last_) >
struct sel_iter
{
    enum { pos_ = is_last_ };
    typedef aux::sel_iter_tag tag;
    typedef random_access_iterator_tag category;

    typedef sel_iter<T,(is_last_ + 1)> next;
    typedef sel_iter<T,(is_last_ - 1)> prior;
    typedef T type;
};

} // namespace aux

template<> struct advance_impl<aux::sel_iter_tag>
{
    template< typename Iterator, typename N > struct apply
    {
        enum { pos_ = Iterator::pos_, n_ = N::value };
        typedef aux::sel_iter<
              typename Iterator::type
            , (pos_ + n_)
            > type;
    };
};

template<> struct distance_impl<aux::sel_iter_tag>
{
    template< typename Iter1, typename Iter2 > struct apply
    {
        enum { pos1_ = Iter1::pos_, pos2_ = Iter2::pos_ };
        typedef int_<( pos2_ - pos1_ )> type;
        BOOST_STATIC_CONSTANT(int, value = ( pos2_ - pos1_ ));
    };
};

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

}}

#endif // BOOST_MPL_AUX_SINGLE_ELEMENT_ITER_HPP_INCLUDED
