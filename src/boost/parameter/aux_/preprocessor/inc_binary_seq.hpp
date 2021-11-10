// Copyright Cromwell D. Enage 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_INC_BINARY_SEQ_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_INC_BINARY_SEQ_HPP

#include <boost/preprocessor/seq/push_back.hpp>

// This macro keeps the rest of the sequence if carry == 0.
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_0(seq, element) \
    (BOOST_PP_SEQ_PUSH_BACK(seq, element), 0)
/**/

#include <boost/preprocessor/control/iif.hpp>

// This macro updates the rest of the sequence if carry == 1.
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_1(seq, element) \
    (BOOST_PP_SEQ_PUSH_BACK(seq, BOOST_PP_IIF(element, 0, 1)), element)
/**/

#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/cat.hpp>

// This macro maintains a tuple (seq, carry), where seq is the intermediate
// result and carry is a flag that will unset upon finding an element == 0.
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_OP(s, result_tuple, element) \
    BOOST_PP_CAT( \
        BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_ \
      , BOOST_PP_TUPLE_ELEM(2, 1, result_tuple) \
    )(BOOST_PP_TUPLE_ELEM(2, 0, result_tuple), element)
/**/

// This macro keeps the sequence at its original length if carry == 0.
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_IMPL_0(seq) seq
/**/

// This macro appends a zero to seq if carry == 1.
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_IMPL_1(seq) \
    BOOST_PP_SEQ_PUSH_BACK(seq, 0)
/**/

// This macro takes in the tuple (seq, carry), with carry indicating whether
// or not seq originally contained all 1s.  If so, then seq now contains all
// 0s, and this macro pushes an extra 0 before expanding to the new sequence.
// Otherwise, this macro expands to seq as is.
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_IMPL(seq_and_carry) \
    BOOST_PP_CAT( \
        BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_IMPL_ \
      , BOOST_PP_TUPLE_ELEM(2, 1, seq_and_carry) \
    )(BOOST_PP_TUPLE_ELEM(2, 0, seq_and_carry))
/**/

#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>

// This macro treats the specified sequence of 1s and 0s like a binary number
// in reverse and expands to a sequence representing the next value up.
// However, if the input sequence contains all 1s, then the output sequence
// will contain one more element but all 0s.
//
// Examples:
// seq = (1)(0)(1)(0) --> return (0)(1)(1)(0)
// seq = (1)(1)(1)(0) --> return (0)(0)(0)(1)
// seq = (1)(1)(1)(1) --> return (0)(0)(0)(0)(0)
#define BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ(seq) \
    BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_IMPL( \
        BOOST_PP_SEQ_FOLD_LEFT( \
            BOOST_PARAMETER_AUX_PP_INC_BINARY_SEQ_OP \
          , (BOOST_PP_SEQ_NIL, 1) \
          , seq \
        ) \
    )
/**/

#endif  // include guard

