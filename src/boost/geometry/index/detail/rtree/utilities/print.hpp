// Boost.Geometry Index
//
// R-tree ostreaming visitor implementation
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_PRINT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_PRINT_HPP

#include <iostream>

#include <boost/geometry/core/static_assert.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {
    
namespace utilities {

namespace dispatch {

template <typename Point, size_t Dimension>
struct print_point
{
    BOOST_STATIC_ASSERT(0 < Dimension);

    static inline void apply(std::ostream & os, Point const& p)
    {
        print_point<Point, Dimension - 1>::apply(os, p);

        os << ", " << geometry::get<Dimension - 1>(p);
    }
};

template <typename Point>
struct print_point<Point, 1>
{
    static inline void apply(std::ostream & os, Point const& p)
    {
        os << geometry::get<0>(p);
    }
};

template <typename Box, size_t Corner, size_t Dimension>
struct print_corner
{
    BOOST_STATIC_ASSERT(0 < Dimension);

    static inline void apply(std::ostream & os, Box const& b)
    {
        print_corner<Box, Corner, Dimension - 1>::apply(os, b);

        os << ", " << geometry::get<Corner, Dimension - 1>(b);
    }
};

template <typename Box, size_t Corner>
struct print_corner<Box, Corner, 1>
{
    static inline void apply(std::ostream & os, Box const& b)
    {
        os << geometry::get<Corner, 0>(b);
    }
};

template <typename Indexable, typename Tag>
struct print_indexable
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Indexable type.",
        Indexable, Tag);
};

template <typename Indexable>
struct print_indexable<Indexable, box_tag>
{
    static const size_t dimension = geometry::dimension<Indexable>::value;

    static inline void apply(std::ostream &os, Indexable const& i)
    {
        os << '(';
        print_corner<Indexable, min_corner, dimension>::apply(os, i);
        os << ")x(";
        print_corner<Indexable, max_corner, dimension>::apply(os, i);
        os << ')';
    }
};

template <typename Indexable>
struct print_indexable<Indexable, point_tag>
{
    static const size_t dimension = geometry::dimension<Indexable>::value;

    static inline void apply(std::ostream &os, Indexable const& i)
    {
        os << '(';
        print_point<Indexable, dimension>::apply(os, i);
        os << ')';
    }
};

template <typename Indexable>
struct print_indexable<Indexable, segment_tag>
{
    static const size_t dimension = geometry::dimension<Indexable>::value;

    static inline void apply(std::ostream &os, Indexable const& i)
    {
        os << '(';
        print_corner<Indexable, 0, dimension>::apply(os, i);
        os << ")-(";
        print_corner<Indexable, 1, dimension>::apply(os, i);
        os << ')';
    }
};

} // namespace dispatch

template <typename Indexable> inline
void print_indexable(std::ostream & os, Indexable const& i)
{
    dispatch::print_indexable<
        Indexable,
        typename tag<Indexable>::type
    >::apply(os, i);
}

} // namespace utilities

namespace rtree { namespace utilities {

namespace visitors {

template <typename MembersHolder>
struct print
    : public MembersHolder::visitor_const
{
    typedef typename MembersHolder::translator_type translator_type;

    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    inline print(std::ostream & o, translator_type const& t)
        : os(o), tr(t), level(0)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        spaces(level) << "INTERNAL NODE - L:" << level << " Ch:" << elements.size() << " @:" << &n << '\n';
        
        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            spaces(level);
            detail::utilities::print_indexable(os, it->first);
            os << " ->" << it->second << '\n';
        }

        size_t level_backup = level;
        ++level;

        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            rtree::apply_visitor(*this, *it->second);
        }

        level = level_backup;
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        spaces(level) << "LEAF - L:" << level << " V:" << elements.size() << " @:" << &n << '\n';
        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            spaces(level);
            detail::utilities::print_indexable(os, tr(*it));
            os << '\n';
        }
    }

    inline std::ostream & spaces(size_t level)
    {
        for ( size_t i = 0 ; i < 2 * level ; ++i )
            os << ' ';
        return os;
    }

    std::ostream & os;
    translator_type const& tr;

    size_t level;
};

} // namespace visitors

template <typename Rtree> inline
void print(std::ostream & os, Rtree const& tree)
{
    typedef utilities::view<Rtree> RTV;
    RTV rtv(tree);

    visitors::print<
        typename RTV::members_holder
    > print_v(os, rtv.translator());
    rtv.apply_visitor(print_v);
}

}} // namespace rtree::utilities

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_PRINT_HPP
