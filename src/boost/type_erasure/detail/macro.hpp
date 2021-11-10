// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_MACRO_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_MACRO_HPP_INCLUDED

#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/pop_back.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_OPEN_NAMESPACE_F(z, data, x) \
    namespace x {

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_OPEN_NAMEPACE_I(seq)\
    BOOST_PP_SEQ_FOR_EACH(BOOST_TYPE_ERASURE_OPEN_NAMESPACE_F, ~, BOOST_PP_SEQ_POP_BACK(seq))

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_OPEN_NAMESPACE(seq)\
    BOOST_PP_IF(BOOST_PP_NOT_EQUAL(BOOST_PP_SEQ_SIZE(seq), 1), BOOST_TYPE_ERASURE_OPEN_NAMEPACE_I, BOOST_PP_TUPLE_EAT(1))(seq)

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_CLOSE_NAMESPACE(seq) \
    BOOST_PP_REPEAT(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(seq)), } BOOST_PP_TUPLE_EAT(3), ~)

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_QUALIFIED_NAME_F(z, data, x)\
    ::x

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_QUALIFIED_NAME(seq) \
    BOOST_PP_SEQ_FOR_EACH(BOOST_TYPE_ERASURE_QUALIFIED_NAME_F, ~, seq)

#endif
