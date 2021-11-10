
#ifndef BOOST_MPL_VECTOR_C_HPP_INCLUDED
#define BOOST_MPL_VECTOR_C_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2008
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#if !defined(BOOST_MPL_PREPROCESSING_MODE)
#   include <boost/mpl/limits/vector.hpp>
#   include <boost/mpl/aux_/nttp_decl.hpp>
#   include <boost/mpl/aux_/config/preprocessor.hpp>

#   include <boost/preprocessor/inc.hpp>
#   include <boost/preprocessor/cat.hpp>
#   include <boost/preprocessor/stringize.hpp>

#if !defined(BOOST_NEEDS_TOKEN_PASTING_OP_FOR_TOKENS_JUXTAPOSING)
#   define AUX778076_VECTOR_C_HEADER \
    BOOST_PP_CAT(BOOST_PP_CAT(vector,BOOST_MPL_LIMIT_VECTOR_SIZE),_c).hpp \
    /**/
#else
#   define AUX778076_VECTOR_C_HEADER \
    BOOST_PP_CAT(BOOST_PP_CAT(vector,BOOST_MPL_LIMIT_VECTOR_SIZE),_c)##.hpp \
    /**/
#endif

#   include BOOST_PP_STRINGIZE(boost/mpl/vector/AUX778076_VECTOR_C_HEADER)
#   undef AUX778076_VECTOR_C_HEADER
#   include <climits>
#endif

#include <boost/mpl/aux_/config/use_preprocessed.hpp>

#if !defined(BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS) \
    && !defined(BOOST_MPL_PREPROCESSING_MODE)

#   define BOOST_MPL_PREPROCESSED_HEADER vector_c.hpp
#   include <boost/mpl/aux_/include_preprocessed.hpp>

#else

#   include <boost/mpl/limits/vector.hpp>

#   define AUX778076_SEQUENCE_NAME vector_c
#   define AUX778076_SEQUENCE_LIMIT BOOST_MPL_LIMIT_VECTOR_SIZE
#   define AUX778076_SEQUENCE_NAME_N(n) BOOST_PP_CAT(BOOST_PP_CAT(vector,n),_c)
#   define AUX778076_SEQUENCE_CONVERT_CN_TO(z,n,TARGET) TARGET(BOOST_PP_CAT(C,n))
#   define AUX778076_SEQUENCE_INTEGRAL_WRAPPER
#   include <boost/mpl/aux_/sequence_wrapper.hpp>

#endif // BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#endif // BOOST_MPL_VECTOR_C_HPP_INCLUDED
