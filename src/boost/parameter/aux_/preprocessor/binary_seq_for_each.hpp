// Copyright Cromwell D. Enage 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_BINARY_SEQ_FOR_EACH_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_BINARY_SEQ_FOR_EACH_HPP

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_0(macro, data)
/**/

#include <boost/parameter/aux_/preprocessor/for_each_pred.hpp>
#include <boost/parameter/aux_/preprocessor/binary_seq_for_each_inc.hpp>
#include <boost/preprocessor/repetition/for.hpp>

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_1(macro, data) \
    BOOST_PP_FOR( \
        (data)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_2 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_2(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_3 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_3(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_4 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_4(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_5 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_5(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_6 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_6(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_7 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_7(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_8 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_1_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_8(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_9 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_2_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_9 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_2_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_9(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_10 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_3_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_10 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_3_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_10 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_3_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_10 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_3_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_10(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_11 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_4_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_11(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_12 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_5_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_12(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_13 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_6_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_13(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_14 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_7_N \
      , macro \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_14(macro, data) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(0)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(0)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(0)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(0)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(0)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(0)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(0)(1)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    ) \
    BOOST_PP_FOR( \
        (data)(1)(1)(1)(1)(1)(1)(1)(0)(0)(0)(0)(0)(0)(0) \
      , BOOST_PARAMETER_AUX_PP_FOR_EACH_PRED_15 \
      , BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_INC_8_N \
      , macro \
    )
/**/

#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/cat.hpp>

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH(n, seq) \
    BOOST_PP_CAT(BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_, n)( \
        BOOST_PP_SEQ_ELEM(0, seq), BOOST_PP_SEQ_ELEM(1, seq) \
    )
/**/

#define BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH_Z(z, n, seq) \
    BOOST_PARAMETER_AUX_PP_BINARY_SEQ_FOR_EACH(n, seq)
/**/

#endif  // include guard

