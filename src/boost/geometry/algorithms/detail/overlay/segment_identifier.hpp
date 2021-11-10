// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SEGMENT_IDENTIFIER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SEGMENT_IDENTIFIER_HPP


#if defined(BOOST_GEOMETRY_DEBUG_OVERLAY)
#  define BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER
#endif

#if defined(BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER)
#include <iostream>
#endif


#include <boost/geometry/algorithms/detail/signed_size_type.hpp>
#include <boost/geometry/algorithms/detail/ring_identifier.hpp>


namespace boost { namespace geometry
{


// Internal struct to uniquely identify a segment
// on a linestring,ring
// or polygon (needs ring_index)
// or multi-geometry (needs multi_index)
// It is always used for clockwise indication (even if the original is anticlockwise)
struct segment_identifier
{
    inline segment_identifier()
        : source_index(-1)
        , multi_index(-1)
        , ring_index(-1)
        , segment_index(-1)
        , piece_index(-1)
    {}

    inline segment_identifier(signed_size_type src,
                              signed_size_type mul,
                              signed_size_type rin,
                              signed_size_type seg)
        : source_index(src)
        , multi_index(mul)
        , ring_index(rin)
        , segment_index(seg)
        , piece_index(-1)
    {}

    inline bool operator<(segment_identifier const& other) const
    {
        return source_index != other.source_index ? source_index < other.source_index
            : multi_index !=other.multi_index ? multi_index < other.multi_index
            : ring_index != other.ring_index ? ring_index < other.ring_index
            : piece_index != other.piece_index ? piece_index < other.piece_index
            : segment_index < other.segment_index
            ;
    }

    inline bool operator==(segment_identifier const& other) const
    {
        return source_index == other.source_index
            && segment_index == other.segment_index
            && ring_index == other.ring_index
            && piece_index == other.piece_index
            && multi_index == other.multi_index
            ;
    }

#if defined(BOOST_GEOMETRY_DEBUG_SEGMENT_IDENTIFIER)
    friend std::ostream& operator<<(std::ostream &os, segment_identifier const& seg_id)
    {
        os
            << "s:" << seg_id.source_index
            << ", v:" << seg_id.segment_index // v:vertex because s is used for source
            ;
        if (seg_id.ring_index >= 0) os << ", r:" << seg_id.ring_index;
        if (seg_id.multi_index >= 0) os << ", m:" << seg_id.multi_index;
        if (seg_id.piece_index >= 0) os << ", p:" << seg_id.piece_index;
        return os;
    }
#endif

    signed_size_type source_index;
    signed_size_type multi_index;
    signed_size_type ring_index;
    signed_size_type segment_index;

    // For buffer - todo: move this to buffer-only
    signed_size_type piece_index;
};

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

// Create a ring identifier from a segment identifier
inline ring_identifier ring_id_by_seg_id(segment_identifier const& seg_id)
{
    return ring_identifier(seg_id.source_index, seg_id.multi_index, seg_id.ring_index);
}

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_SEGMENT_IDENTIFIER_HPP
