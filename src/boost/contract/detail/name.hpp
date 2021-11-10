
#ifndef BOOST_CONTRACT_DETAIL_NAME_HPP_
#define BOOST_CONTRACT_DETAIL_NAME_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/contract/detail/config.hpp>
#include <boost/preprocessor/cat.hpp>

/* PUBLIC */

// NOTE: Explicitly list number of names to concatenate using ..._NAME-n
// (instead of using ..._SEQ_CAT or similar) for optimal speed and reentrancy.

#define BOOST_CONTRACT_DETAIL_NAME1(name1) \
    BOOST_PP_CAT(BOOST_CONTRACT_DETAIL_NAME_PREFIX, name1)

#define BOOST_CONTRACT_DETAIL_NAME2(name1, name2) \
    BOOST_PP_CAT(BOOST_CONTRACT_DETAIL_NAME_PREFIX, BOOST_PP_CAT(name1, \
            BOOST_PP_CAT(BOOST_CONTRACT_DETAIL_NAME_INFIX, name2)))
       
#endif // #include guard

