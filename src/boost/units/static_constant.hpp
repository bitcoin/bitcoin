// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_STATIC_CONSTANT_HPP
#define BOOST_UNITS_STATIC_CONSTANT_HPP

#include <boost/units/config.hpp>

#if defined(BOOST_NO_CXX11_CONSTEXPR) || defined(BOOST_UNITS_DOXYGEN)
/// A convenience macro that allows definition of static
/// constants in headers in an ODR-safe way.
# define BOOST_UNITS_STATIC_CONSTANT(name, type)            \
template<bool b>                                            \
struct name##_instance_t                                    \
{                                                           \
    static const type instance;                             \
};                                                          \
                                                            \
namespace                                                   \
{                                                           \
    static const type& name = name##_instance_t<true>::instance;   \
}                                                           \
                                                            \
template<bool b>                                            \
const type name##_instance_t<b>::instance
#else
# define BOOST_UNITS_STATIC_CONSTANT(name, type)            \
BOOST_STATIC_CONSTEXPR type name
#endif

/// A convenience macro for static constants with auto 
/// type deduction. 
#if BOOST_UNITS_HAS_TYPEOF

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#define BOOST_UNITS_AUTO_STATIC_CONSTANT(name, value)               \
BOOST_TYPEOF_NESTED_TYPEDEF(name##_nested_t, value)                 \
BOOST_UNITS_STATIC_CONSTANT(name, name##_nested_t::type) = (value)

#elif BOOST_UNITS_HAS_MWERKS_TYPEOF

#define BOOST_UNITS_AUTO_STATIC_CONSTANT(name, value)               \
BOOST_UNITS_STATIC_CONSTANT(name, __typeof__(value)) = (value)

#elif BOOST_UNITS_HAS_GNU_TYPEOF

#define BOOST_UNITS_AUTO_STATIC_CONSTANT(name, value)               \
BOOST_UNITS_STATIC_CONSTANT(name, typeof(value)) = (value)

#endif // BOOST_UNITS_HAS_BOOST_TYPEOF

#endif // BOOST_UNITS_HAS_TYPEOF

#endif // BOOST_UNITS_STATIC_CONSTANT_HPP
