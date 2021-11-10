
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_HPP_
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_HPP_

// Boost.Preprocessor author P. Mensodines confirmed on an Boost email thread
// (subject ``check if a token is a keyword (was "BOOST_PP_IS_UNARY()")'')
// that it is OK to used `PP_IS_UNARY()` to check if tokens match predefined
// "keyword" as it is done by the macros below (even if `PP_IS_UNARY()` is
// technically only part of Boost.Preprocessor private API).
#include <boost/preprocessor/detail/is_unary.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

// PRIVATE //

#define BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_(a, b) \
    BOOST_PP_IS_UNARY(BOOST_PP_CAT(a, b))

// PUBLIC //

// `checking_prefix ## tokens` expand to unary (e.g., `(1)`) iff `tokens` start
// with keyword to check.
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_FRONT( \
        tokens, checking_prefix) \
    BOOST_PP_IIF(BOOST_PP_IS_UNARY(tokens), \
        /* on MSVC this check works even if tokens already unary but on */ \
        /* C++03 (including GCC) this check on non-unary tokens gives */ \
        /* a concatenation error -- so return false is tokens is not unary */ \
        0 BOOST_PP_TUPLE_EAT(2) \
    , \
        BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_ \
    )(checking_prefix, tokens)

// `token ## checking_postfix` expand to unary (e.g., `(1)`) iff `token` is the
// keyword to check. This check only works if `token` is a single token, it
// will always expand to 0 if token is multiple tokens (e.g., `const *this`).
// This check will expand to 0 with no error if `token` starts with a
// non-alphanumeric symbol (e.g., `*this`).
#define BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_BACK( \
        token, checking_postfix) \
    BOOST_LOCAL_FUNCTION_DETAIL_PP_KEYWORD_FACILITY_IS_(token, checking_postfix)

#endif // #include guard

