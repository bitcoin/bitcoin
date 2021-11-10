// Boost.Geometry Index
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_ASSERT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_ASSERT_HPP

#include <boost/geometry/core/assert.hpp>

#undef BOOST_GEOMETRY_INDEX_ASSERT

#define BOOST_GEOMETRY_INDEX_ASSERT(CONDITION, TEXT_MSG) \
    BOOST_GEOMETRY_ASSERT_MSG(CONDITION, TEXT_MSG)

#endif // BOOST_GEOMETRY_INDEX_DETAIL_ASSERT_HPP
