// Boost.Geometry Index
//
// R-tree queries range adaptors
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_ADAPTORS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_ADAPTORS_HPP

#include <deque>
#include <boost/static_assert.hpp>

#include <boost/geometry/index/adaptors/query.hpp>

namespace boost { namespace geometry { namespace index {

template <typename Value, typename Options, typename IndexableGetter, typename EqualTo, typename Allocator>
class rtree;

namespace adaptors { namespace detail {

template <typename Value, typename Options, typename IndexableGetter, typename EqualTo, typename Allocator>
class query_range< index::rtree<Value, Options, IndexableGetter, EqualTo, Allocator> >
{
public:
    typedef std::vector<Value> result_type;
    typedef typename result_type::iterator iterator;
    typedef typename result_type::const_iterator const_iterator;

    template <typename Predicates> inline
    query_range(index::rtree<Value, Options, IndexableGetter, EqualTo, Allocator> const& rtree,
                Predicates const& pred)
    {
        rtree.query(pred, std::back_inserter(m_result));
    }

    inline iterator begin() { return m_result.begin(); }
    inline iterator end() { return m_result.end(); }
    inline const_iterator begin() const { return m_result.begin(); }
    inline const_iterator end() const { return m_result.end(); }

private:
    result_type m_result;
};

}} // namespace adaptors::detail

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_ADAPTORS_HPP
