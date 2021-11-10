
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_SYMBOL_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_SYMBOL_HPP_

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/transform.hpp>

// PRIVATE //

// NOTE: INFIX is used to separate symbols concatenated together. Some of these
// symbols are user-defined so they can be anything. Because they can contain
// underscore `_` and/or start with capital letters, it is NOT safe for the
// INFIX to be the underscore `_` character because that could lead to library
// defined symbols containing double underscores `__` or a leading underscore
// (followed or not by a capital letter) in the global namespace. All these
// symbols are reserved by the C++ standard: (1) "each name that contains a
// double underscore (_ _) or begins with an underscore followed by an
// uppercase letter is reserved to the implementation" and (2) "each name that
// begins with an underscore is reserved to the implementation for use as a
// name in the global namespace".
#define BOOST_LOCAL_FUNCTION_AUX_SYMBOL_INFIX_ X // `X` used as separator.
            
#define BOOST_LOCAL_FUNCTION_AUX_SYMBOL_PREFIX_ boost_local_function_aux

#define BOOST_LOCAL_FUNCTION_AUX_SYMBOL_POSTFIX_(s, unused, tokens) \
    BOOST_PP_CAT(tokens, BOOST_LOCAL_FUNCTION_AUX_SYMBOL_INFIX_)

// PUBLIC //

// Prefixes this library reserved symbol.
#define BOOST_LOCAL_FUNCTION_AUX_SYMBOL(seq) \
    BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TRANSFORM( \
            BOOST_LOCAL_FUNCTION_AUX_SYMBOL_POSTFIX_, \
            ~, (BOOST_LOCAL_FUNCTION_AUX_SYMBOL_PREFIX_) seq ))

// Postfixes this library reserved symbol.
#define BOOST_LOCAL_FUNCTION_AUX_SYMBOL_POSTFIX(seq) \
    BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TRANSFORM( \
            BOOST_LOCAL_FUNCTION_AUX_SYMBOL_POSTFIX_, \
            ~, seq (BOOST_LOCAL_FUNCTION_AUX_SYMBOL_PREFIX_) ))

#endif // #include guard

