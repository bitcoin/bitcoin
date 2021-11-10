
// NO INCLUDE GUARDS, THE HEADER IS INTENDED FOR MULTIPLE INCLUSION

#if !defined(BOOST_PP_IS_ITERATING)

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

#else

#include <boost/mpl/aux_/config/typeof.hpp>
#include <boost/mpl/aux_/config/ctps.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/cat.hpp>

#define i_ BOOST_PP_FRAME_ITERATION(1)

#   define AUX778076_MAP_TAIL(map, i_, P) \
    BOOST_PP_CAT(map,i_)< \
          BOOST_PP_ENUM_PARAMS(i_, P) \
        > \
    /**/


#if defined(BOOST_MPL_CFG_TYPEOF_BASED_SEQUENCES)

template<
      BOOST_PP_ENUM_PARAMS(i_, typename P)
    >
struct BOOST_PP_CAT(map,i_)
    : m_item<
          typename BOOST_PP_CAT(P,BOOST_PP_DEC(i_))::first
        , typename BOOST_PP_CAT(P,BOOST_PP_DEC(i_))::second
        , AUX778076_MAP_TAIL(map,BOOST_PP_DEC(i_),P)
        >
{
    typedef BOOST_PP_CAT(map,i_) type;
};

#else // "brute force" implementation

#   if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

template< typename Map>
struct m_at<Map,BOOST_PP_DEC(i_)>
{
    typedef typename Map::BOOST_PP_CAT(item,BOOST_PP_DEC(i_)) type;
};

template< typename Key, typename T, typename Base >
struct m_item<i_,Key,T,Base>
    : m_item_<Key,T,Base>
{
    typedef pair<Key,T> BOOST_PP_CAT(item,BOOST_PP_DEC(i_));
};

#   else

template<>
struct m_at_impl<BOOST_PP_DEC(i_)>
{
    template< typename Map > struct result_
    {
        typedef typename Map::BOOST_PP_CAT(item,BOOST_PP_DEC(i_)) type;
    };
};

template<>
struct m_item_impl<i_>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_<Key,T,Base>
    {
        typedef pair<Key,T> BOOST_PP_CAT(item,BOOST_PP_DEC(i_));
    };
};

#   endif

template<
      BOOST_PP_ENUM_PARAMS(i_, typename P)
    >
struct BOOST_PP_CAT(map,i_)
    : m_item<
          i_
        , typename BOOST_PP_CAT(P,BOOST_PP_DEC(i_))::first
        , typename BOOST_PP_CAT(P,BOOST_PP_DEC(i_))::second
        , AUX778076_MAP_TAIL(map,BOOST_PP_DEC(i_),P)
        >
{
    typedef BOOST_PP_CAT(map,i_) type;
};

#endif // BOOST_MPL_CFG_TYPEOF_BASED_SEQUENCES

#   undef AUX778076_MAP_TAIL

#undef i_

#endif // BOOST_PP_IS_ITERATING
