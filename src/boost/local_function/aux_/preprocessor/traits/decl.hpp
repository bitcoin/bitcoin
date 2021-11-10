
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_HPP_

#include <boost/local_function/aux_/preprocessor/traits/decl_sign_/sign.hpp>
#include <boost/local_function/aux_/preprocessor/traits/decl_/nil.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/list/adt.hpp>

// PUBLIC //

// Expand: decl_traits (see DECL_TRAITS macros to inspect these traits).
#define BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS(declarations) \
    BOOST_PP_IIF(BOOST_PP_LIST_IS_NIL(declarations), \
        BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_NIL \
        BOOST_PP_TUPLE_EAT(1) \
    , \
        BOOST_LOCAL_FUNCTION_AUX_PP_DECL_TRAITS_SIGN \
    )(declarations)

#endif // #include guard

