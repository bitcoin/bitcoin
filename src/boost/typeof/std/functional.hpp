// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_functional_hpp_INCLUDED
#define BOOST_TYPEOF_STD_functional_hpp_INCLUDED

#include <functional>
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

#ifndef BOOST_NO_CXX98_FUNCTION_BASE
BOOST_TYPEOF_REGISTER_TEMPLATE(std::unary_function, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::binary_function, 3)
#endif//BOOST_NO_CXX98_FUNCTION_BASE
BOOST_TYPEOF_REGISTER_TEMPLATE(std::plus, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::minus, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multiplies, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::divides, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::modulus, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::negate, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::equal_to, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::not_equal_to, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::greater, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::less, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::greater_equal, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::less_equal, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::logical_and, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::logical_or, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::logical_not, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::unary_negate, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::binary_negate, 1)

#ifndef BOOST_NO_CXX98_BINDERS

#if defined(__MWERKS__) && defined(_MSL_EXTENDED_BINDERS)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::binder1st, 2)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::binder2nd, 2)
#else
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::binder1st, 1)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::binder2nd, 1)
#endif

BOOST_TYPEOF_REGISTER_TEMPLATE(std::pointer_to_unary_function, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::pointer_to_binary_function, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::mem_fun_t, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::mem_fun1_t, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::mem_fun_ref_t, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::mem_fun1_ref_t, 3)

#if !BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::const_mem_fun_t, 2)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::const_mem_fun1_t, 3)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::const_mem_fun_ref_t, 2)
    BOOST_TYPEOF_REGISTER_TEMPLATE(std::const_mem_fun1_ref_t, 3)
#endif//BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)

#endif//BOOST_NO_CXX98_BINDERS

#endif//BOOST_TYPEOF_STD_functional_hpp_INCLUDED
