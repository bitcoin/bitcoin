// Boost.Geometry Index
//
// R-tree OpenGL drawing visitor implementation
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

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_GL_DRAW_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_GL_DRAW_HPP

#include <boost/geometry/core/static_assert.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

namespace utilities {

namespace dispatch {

template <typename Point, size_t Dimension>
struct gl_draw_point
{};

template <typename Point>
struct gl_draw_point<Point, 2>
{
    static inline void apply(Point const& p, typename coordinate_type<Point>::type z)
    {
        typename coordinate_type<Point>::type const& x = geometry::get<0>(p);
        typename coordinate_type<Point>::type const& y = geometry::get<1>(p);
        /*glBegin(GL_POINT);
        glVertex3f(x, y, z);
        glEnd();*/
        glBegin(GL_QUADS);
        glVertex3f(x+1, y, z);
        glVertex3f(x, y+1, z);
        glVertex3f(x-1, y, z);
        glVertex3f(x, y-1, z);
        glEnd();
    }
};

template <typename Box, size_t Dimension>
struct gl_draw_box
{};

template <typename Box>
struct gl_draw_box<Box, 2>
{
    static inline void apply(Box const& b, typename coordinate_type<Box>::type z)
    {
        glBegin(GL_LINE_LOOP);
        glVertex3f(geometry::get<min_corner, 0>(b), geometry::get<min_corner, 1>(b), z);
        glVertex3f(geometry::get<max_corner, 0>(b), geometry::get<min_corner, 1>(b), z);
        glVertex3f(geometry::get<max_corner, 0>(b), geometry::get<max_corner, 1>(b), z);
        glVertex3f(geometry::get<min_corner, 0>(b), geometry::get<max_corner, 1>(b), z);
        glEnd();
    }
};

template <typename Segment, size_t Dimension>
struct gl_draw_segment
{};

template <typename Segment>
struct gl_draw_segment<Segment, 2>
{
    static inline void apply(Segment const& s, typename coordinate_type<Segment>::type z)
    {
        glBegin(GL_LINES);
        glVertex3f(geometry::get<0, 0>(s), geometry::get<0, 1>(s), z);
        glVertex3f(geometry::get<1, 0>(s), geometry::get<1, 1>(s), z);
        glEnd();
    }
};

template <typename Indexable, typename Tag>
struct gl_draw_indexable
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Indexable type.",
        Indexable, Tag);
};

template <typename Box>
struct gl_draw_indexable<Box, box_tag>
    : gl_draw_box<Box, geometry::dimension<Box>::value>
{};

template <typename Point>
struct gl_draw_indexable<Point, point_tag>
    : gl_draw_point<Point, geometry::dimension<Point>::value>
{};

template <typename Segment>
struct gl_draw_indexable<Segment, segment_tag>
    : gl_draw_segment<Segment, geometry::dimension<Segment>::value>
{};

} // namespace dispatch

template <typename Indexable> inline
void gl_draw_indexable(Indexable const& i, typename coordinate_type<Indexable>::type z)
{
    dispatch::gl_draw_indexable<
        Indexable,
        typename tag<Indexable>::type
    >::apply(i, z);
}

} // namespace utilities

namespace rtree { namespace utilities {

namespace visitors {

template <typename MembersHolder>
struct gl_draw
    : public MembersHolder::visitor_const
{
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::translator_type translator_type;

    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    inline gl_draw(translator_type const& t,
                   size_t level_first = 0,
                   size_t level_last = (std::numeric_limits<size_t>::max)(),
                   typename coordinate_type<box_type>::type z_coord_level_multiplier = 1
    )
        : tr(t)
        , level_f(level_first)
        , level_l(level_last)
        , z_mul(z_coord_level_multiplier)
        , level(0)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        if ( level_f <= level )
        {
            size_t level_rel = level - level_f;

            if ( level_rel == 0 )
                glColor3f(0.75f, 0.0f, 0.0f);
            else if ( level_rel == 1 )
                glColor3f(0.0f, 0.75f, 0.0f);
            else if ( level_rel == 2 )
                glColor3f(0.0f, 0.0f, 0.75f);
            else if ( level_rel == 3 )
                glColor3f(0.75f, 0.75f, 0.0f);
            else if ( level_rel == 4 )
                glColor3f(0.75f, 0.0f, 0.75f);
            else if ( level_rel == 5 )
                glColor3f(0.0f, 0.75f, 0.75f);
            else
                glColor3f(0.5f, 0.5f, 0.5f);

            for (typename elements_type::const_iterator it = elements.begin();
                it != elements.end(); ++it)
            {
                detail::utilities::gl_draw_indexable(it->first, level_rel * z_mul);
            }
        }
        
        size_t level_backup = level;
        ++level;

        if ( level < level_l )
        {
            for (typename elements_type::const_iterator it = elements.begin();
                it != elements.end(); ++it)
            {
                rtree::apply_visitor(*this, *it->second);
            }
        }

        level = level_backup;
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        if ( level_f <= level )
        {
            size_t level_rel = level - level_f;

            glColor3f(0.25f, 0.25f, 0.25f);

            for (typename elements_type::const_iterator it = elements.begin();
                it != elements.end(); ++it)
            {
                detail::utilities::gl_draw_indexable(tr(*it), level_rel * z_mul);
            }
        }
    }

    translator_type const& tr;
    size_t level_f;
    size_t level_l;
    typename coordinate_type<box_type>::type z_mul;

    size_t level;
};

} // namespace visitors

template <typename Rtree> inline
void gl_draw(Rtree const& tree,
             size_t level_first = 0,
             size_t level_last = (std::numeric_limits<size_t>::max)(),
             typename coordinate_type<
                    typename Rtree::bounds_type
                >::type z_coord_level_multiplier = 1
             )
{
    typedef utilities::view<Rtree> RTV;
    RTV rtv(tree);

    if ( !tree.empty() )
    {
        glColor3f(0.75f, 0.75f, 0.75f);
        detail::utilities::gl_draw_indexable(tree.bounds(), 0);
    }

    visitors::gl_draw<
        typename RTV::members_holder
    > gl_draw_v(rtv.translator(), level_first, level_last, z_coord_level_multiplier);

    rtv.apply_visitor(gl_draw_v);
}

}} // namespace rtree::utilities

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_UTILITIES_GL_DRAW_HPP
