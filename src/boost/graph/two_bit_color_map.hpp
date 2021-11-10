// Copyright (C) 2005-2006 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Douglas Gregor
//           Andrew Lumsdaine

// Two bit per color property map

#ifndef BOOST_TWO_BIT_COLOR_MAP_HPP
#define BOOST_TWO_BIT_COLOR_MAP_HPP

#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/detail/mpi_include.hpp>
#include <boost/shared_array.hpp>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <algorithm>
#include <limits>

namespace boost
{

enum two_bit_color_type
{
    two_bit_white = 0,
    two_bit_gray = 1,
    two_bit_green = 2,
    two_bit_black = 3
};

template <> struct color_traits< two_bit_color_type >
{
    static two_bit_color_type white() { return two_bit_white; }
    static two_bit_color_type gray() { return two_bit_gray; }
    static two_bit_color_type green() { return two_bit_green; }
    static two_bit_color_type black() { return two_bit_black; }
};

template < typename IndexMap = identity_property_map > struct two_bit_color_map
{
    std::size_t n;
    IndexMap index;
    shared_array< unsigned char > data;

    BOOST_STATIC_CONSTANT(
        int, bits_per_char = std::numeric_limits< unsigned char >::digits);
    BOOST_STATIC_CONSTANT(int, elements_per_char = bits_per_char / 2);
    typedef typename property_traits< IndexMap >::key_type key_type;
    typedef two_bit_color_type value_type;
    typedef void reference;
    typedef read_write_property_map_tag category;

    explicit two_bit_color_map(
        std::size_t n, const IndexMap& index = IndexMap())
    : n(n)
    , index(index)
    , data(new unsigned char[(n + elements_per_char - 1) / elements_per_char]())
    {
    }
};

template < typename IndexMap >
inline two_bit_color_type get(const two_bit_color_map< IndexMap >& pm,
    typename property_traits< IndexMap >::key_type key)
{
    BOOST_STATIC_CONSTANT(int,
        elements_per_char = two_bit_color_map< IndexMap >::elements_per_char);
    typename property_traits< IndexMap >::value_type i = get(pm.index, key);
    BOOST_ASSERT((std::size_t)i < pm.n);
    std::size_t byte_num = i / elements_per_char;
    std::size_t bit_position = ((i % elements_per_char) * 2);
    return two_bit_color_type((pm.data.get()[byte_num] >> bit_position) & 3);
}

template < typename IndexMap >
inline void put(const two_bit_color_map< IndexMap >& pm,
    typename property_traits< IndexMap >::key_type key,
    two_bit_color_type value)
{
    BOOST_STATIC_CONSTANT(int,
        elements_per_char = two_bit_color_map< IndexMap >::elements_per_char);
    typename property_traits< IndexMap >::value_type i = get(pm.index, key);
    BOOST_ASSERT((std::size_t)i < pm.n);
    BOOST_ASSERT(value >= 0 && value < 4);
    std::size_t byte_num = i / elements_per_char;
    std::size_t bit_position = ((i % elements_per_char) * 2);
    pm.data.get()[byte_num]
        = (unsigned char)((pm.data.get()[byte_num] & ~(3 << bit_position))
            | (value << bit_position));
}

template < typename IndexMap >
inline two_bit_color_map< IndexMap > make_two_bit_color_map(
    std::size_t n, const IndexMap& index_map)
{
    return two_bit_color_map< IndexMap >(n, index_map);
}

} // end namespace boost

#include BOOST_GRAPH_MPI_INCLUDE(<boost/graph/distributed/two_bit_color_map.hpp>)

#endif // BOOST_TWO_BIT_COLOR_MAP_HPP
