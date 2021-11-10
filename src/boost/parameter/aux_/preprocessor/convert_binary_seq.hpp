// Copyright Cromwell D. Enage 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_CONVERT_BINARY_SEQ_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_CONVERT_BINARY_SEQ_HPP

#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/push_back.hpp>

#define BOOST_PARAMETER_AUX_PP_AUGMENT_BINARY_SEQ_INDEX_FOLD_OP(s, seq, idx) \
    BOOST_PP_SEQ_PUSH_BACK(seq, (idx, BOOST_PP_SEQ_SIZE(seq)))
/**/

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#define BOOST_PARAMETER_AUX_PP_CONVERT_BINARY_SEQ_FOLD_OP(s, seq, elem) \
    ( \
        BOOST_PP_SEQ_PUSH_BACK( \
            BOOST_PP_SEQ_ELEM(0, seq) \
          , BOOST_PP_IIF( \
                BOOST_PP_TUPLE_ELEM(2, 0, elem) \
              , BOOST_PP_SEQ_ELEM(2, seq) \
              , BOOST_PP_SEQ_ELEM(1, seq) \
            )(BOOST_PP_TUPLE_ELEM(2, 1, elem), BOOST_PP_SEQ_ELEM(3, seq)) \
        ) \
    )(BOOST_PP_SEQ_ELEM(1, seq))(BOOST_PP_SEQ_ELEM(2, seq)) \
    (BOOST_PP_SEQ_ELEM(3, seq))
/**/

#include <boost/parameter/aux_/preprocessor/seq_enum.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>

#define BOOST_PARAMETER_AUX_PP_CONVERT_BINARY_SEQ(seq, macro0, macro1, data) \
    BOOST_PARAMETER_SEQ_ENUM( \
        BOOST_PP_SEQ_ELEM( \
            0 \
          , BOOST_PP_SEQ_FOLD_LEFT( \
                BOOST_PARAMETER_AUX_PP_CONVERT_BINARY_SEQ_FOLD_OP \
              , (BOOST_PP_SEQ_NIL)(macro0)(macro1)(data) \
              , BOOST_PP_SEQ_FOLD_LEFT( \
                    BOOST_PARAMETER_AUX_PP_AUGMENT_BINARY_SEQ_INDEX_FOLD_OP \
                  , BOOST_PP_EMPTY() \
                  , seq \
                ) \
            ) \
        ) \
    )
/**/

#endif  // include guard

