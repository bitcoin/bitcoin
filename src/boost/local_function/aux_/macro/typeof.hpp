
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_TYPEOF_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_TYPEOF_HPP_

#include <boost/local_function/aux_/symbol.hpp>

// PUBLIC //

// Actual type-name for specified symbol name.
#define BOOST_LOCAL_FUNCTION_AUX_TYPEOF_TYPE(name) \
    /* cannot prefix in front of name because it could start with non */ \
    /* alphanumeric symbols (e.g., & for binding by reference) */ \
    BOOST_LOCAL_FUNCTION_AUX_SYMBOL_POSTFIX( (name)(typeof_type) )

#endif // #include guard

