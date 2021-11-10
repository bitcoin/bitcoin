
#ifndef BOOST_CONTRACT_DETAIL_PP_KEYWORD_PRIVATE_HPP_
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_PRIVATE_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/contract/detail/preprocessor/keyword/utility/is.hpp>
#include <boost/preprocessor/cat.hpp>

/* PRIVATE */

// Must expand to a single comma `,` (not local macros, do not #undefine).
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_PRIVATE_CAT_TO_COMMAprivate ,

// Must expand to empty `` (not local macros, do not #undefine).
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_PRIVATE_CAT_TO_EMPTYprivate

/* PUBLIC */

// Precondition: tokens must start with a token concatenable to a macro name
//               (e.g., a literal or integral token).
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PRIVATE(tokens) \
    BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS( \
            BOOST_CONTRACT_DETAIL_PP_KEYWORD_PRIVATE_CAT_TO_COMMA, tokens)

// Precondition: tokens must start with `private` (this can be
//               checked with `..._IS_PRIVATE` macro above).
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_REMOVE_PRIVATE(tokens) \
    BOOST_PP_CAT(BOOST_CONTRACT_DETAIL_PP_KEYWORD_PRIVATE_CAT_TO_EMPTY, tokens)

#endif // #include guard

