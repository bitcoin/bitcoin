// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <type_traits>

#include <boost/swap.hpp>

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_UTILITIES_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_UTILITIES_HPP

namespace boost { namespace geometry { namespace index { namespace detail {

template<class T>
static inline void assign_cond(T & l, T const& r, std::true_type)
{
    l = r;
}

template<class T>
static inline void assign_cond(T &, T const&, std::false_type) {}

template<class T>
static inline void move_cond(T & l, T & r, std::true_type)
{
    l = ::boost::move(r);
}

template<class T>
static inline void move_cond(T &, T &, std::false_type) {}

template <typename T> inline
void swap_cond(T & l, T & r, std::true_type)
{
    ::boost::swap(l, r);
}

template <typename T> inline
void swap_cond(T &, T &, std::false_type) {}

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_UTILITIES_HPP
