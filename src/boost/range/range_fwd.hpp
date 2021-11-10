// Boost.Range library
//
//  Copyright Neil Groves 2003-2004.
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_RANGE_FWD_HPP_INCLUDED
#define BOOST_RANGE_RANGE_FWD_HPP_INCLUDED

namespace boost
{

// Extension points
    template<typename C, typename Enabler>
    struct range_iterator;

    template<typename C, typename Enabler>
    struct range_mutable_iterator;

    template<typename C, typename Enabler>
    struct range_const_iterator;

// Core classes
    template<typename IteratorT>
    class iterator_range;

    template<typename ForwardRange>
    class sub_range;

// Meta-functions
    template<typename T>
    struct range_category;

    template<typename T>
    struct range_difference;

    template<typename T>
    struct range_pointer;

    template<typename T>
    struct range_reference;

    template<typename T>
    struct range_reverse_iterator;

    template<typename T>
    struct range_size;

    template<typename T>
    struct range_value;

    template<typename T>
    struct has_range_iterator;

    template<typename T>
    struct has_range_const_iterator;

} // namespace boost

#endif // include guard
