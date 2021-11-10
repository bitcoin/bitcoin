// Copyright (C) 2005 Arkadiy Vertleyb
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_ENCODE_DECODE_PARAMS_HPP_INCLUDED
#define BOOST_TYPEOF_ENCODE_DECODE_PARAMS_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

// Assumes iter0 contains initial iterator

#define BOOST_TYPEOF_DECODE_PARAM(z, n, text)   \
    typedef boost::type_of::decode_type<iter##n> decode##n;     \
    typedef typename decode##n::type p##n;      \
    typedef typename decode##n::iter BOOST_PP_CAT(iter, BOOST_PP_INC(n));

#define BOOST_TYPEOF_DECODE_PARAMS(n)\
    BOOST_PP_REPEAT(n, BOOST_TYPEOF_DECODE_PARAM, ~)

// The P0, P1, ... PN are encoded and added to V 

#define BOOST_TYPEOF_ENCODE_PARAMS_BEGIN(z, n, text)\
    typename boost::type_of::encode_type<

#define BOOST_TYPEOF_ENCODE_PARAMS_END(z, n, text)\
    , BOOST_PP_CAT(P, n)>::type

#define BOOST_TYPEOF_ENCODE_PARAMS(n, ID)                   \
    BOOST_PP_REPEAT(n, BOOST_TYPEOF_ENCODE_PARAMS_BEGIN, ~) \
    typename boost::type_of::push_back<V, boost::type_of::constant<std::size_t,ID> >::type      \
    BOOST_PP_REPEAT(n, BOOST_TYPEOF_ENCODE_PARAMS_END, ~)

#endif//BOOST_TYPEOF_ENCODE_DECODE_PARAMS_HPP_INCLUDED
