//  declval.hpp  -------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_TYPE_TRAITS_DECLVAL_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_DECLVAL_HPP_INCLUDED

#include <boost/config.hpp>

//----------------------------------------------------------------------------//

#include <boost/type_traits/add_rvalue_reference.hpp>

//----------------------------------------------------------------------------//
//                                                                            //
//                           C++03 implementation of                          //
//                   20.2.4 Function template declval [declval]               //
//                          Written by Vicente J. Botet Escriba               //
//                                                                            //
// 1 The library provides the function template declval to simplify the
// definition of expressions which occur as unevaluated operands.
// 2 Remarks: If this function is used, the program is ill-formed.
// 3 Remarks: The template parameter T of declval may be an incomplete type.
// [ Example:
//
// template <class To, class From>
// decltype(static_cast<To>(declval<From>())) convert(From&&);
//
// declares a function template convert which only participates in overloading
// if the type From can be explicitly converted to type To. For another example
// see class template common_type (20.9.7.6). -end example ]
//----------------------------------------------------------------------------//

namespace boost {

    template <typename T>
    typename add_rvalue_reference<T>::type declval() BOOST_NOEXCEPT; // as unevaluated operand

}  // namespace boost

#endif  // BOOST_TYPE_TRAITS_DECLVAL_HPP_INCLUDED
