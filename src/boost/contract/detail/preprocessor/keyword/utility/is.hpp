
#ifndef BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS_HPP_
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/variadic/size.hpp>

/* PRIVATE */

#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS_1 0
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS_2 1

/* PUBLIC */

// Precondition: A macro named `cat_to_comma_prefix ## token-to-check` must be
//               #defined to expand to `,`.
// Precondition: tokens must start with a token concatenable to a macro name
//               (e.g., a literal or integral token).
#define BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS( \
        cat_to_comma_prefix, tokens) \
    BOOST_PP_CAT(BOOST_CONTRACT_DETAIL_PP_KEYWORD_UTILITY_IS_, \
            BOOST_PP_VARIADIC_SIZE(BOOST_PP_CAT(cat_to_comma_prefix, tokens)))

#endif // #include guard

