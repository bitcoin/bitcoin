// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICHMENT_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICHMENT_INFO_HPP

#include <boost/geometry/algorithms/detail/signed_size_type.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


/*!
\brief Keeps info to enrich intersection info (per source)
\details Class to keep information necessary for traversal phase (a phase
    of the overlay process). The information is gathered during the
    enrichment phase
 */
template<typename Point>
struct enrichment_info
{
    inline enrichment_info()
        : travels_to_vertex_index(-1)
        , travels_to_ip_index(-1)
        , next_ip_index(-1)
        , startable(true)
        , prefer_start(true)
        , count_left(0)
        , count_right(0)
        , rank(-1)
        , zone(-1)
        , region_id(-1)
        , isolated(false)
    {}

    inline signed_size_type get_next_turn_index() const
    {
        return next_ip_index == -1 ? travels_to_ip_index : next_ip_index;
    }

    // vertex to which is free travel after this IP,
    // so from "segment_index+1" to "travels_to_vertex_index", without IP-s,
    // can be -1
    signed_size_type travels_to_vertex_index;

    // same but now IP index, so "next IP index" but not on THIS segment
    signed_size_type travels_to_ip_index;

    // index of next IP on this segment, -1 if there is no one
    signed_size_type next_ip_index;

    bool startable; // Can be used to start in traverse
    bool prefer_start; // Is preferred as starting point (if true)

    // Counts if polygons left/right of this operation
    std::size_t count_left;
    std::size_t count_right;
    signed_size_type rank; // in cluster
    signed_size_type zone; // open zone, in cluster
    signed_size_type region_id;
    bool isolated;
};


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICHMENT_INFO_HPP
