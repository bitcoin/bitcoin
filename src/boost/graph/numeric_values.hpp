// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_NUMERIC_VALUES_HPP
#define BOOST_GRAPH_NUMERIC_VALUES_HPP

#include <limits>

namespace boost
{

#define BOOST_GRAPH_SPECIALIZE_NUMERIC_FLOAT(type)          \
    template <> struct numeric_values< type >               \
    {                                                       \
        typedef type value_type;                            \
        static type zero() { return 0.0; }                  \
        static type infinity()                              \
        {                                                   \
            return std::numeric_limits< type >::infinity(); \
        }                                                   \
    };

/**
 * This generic type reports various numeric values for some type. In the
 * general case, numeric values simply treat their maximum value as infinity
 * and the default-constructed value as 0.
 *
 * Specializations of this template can redefine the notions of zero and
 * infinity for various types. For example, the class is specialized for
 * floating point types to use the built in notion of infinity.
 */
template < typename T > struct numeric_values
{
    typedef T value_type;

    static T zero() { return T(); }

    static T infinity() { return (std::numeric_limits< T >::max)(); }
};

// Specializations for floating point types refer to 0.0 and their infinity
// value defined by numeric_limits.
BOOST_GRAPH_SPECIALIZE_NUMERIC_FLOAT(float)
BOOST_GRAPH_SPECIALIZE_NUMERIC_FLOAT(double)
BOOST_GRAPH_SPECIALIZE_NUMERIC_FLOAT(long double)

#undef BOOST_GRAPH_SPECIALIZE_NUMERIC_VALUE
}

#endif
