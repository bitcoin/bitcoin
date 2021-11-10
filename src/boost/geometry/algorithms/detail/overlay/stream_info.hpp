// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_STREAM_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_STREAM_INFO_HPP


#include <string>

#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


    static inline std::string dir(int d)
    {
        return d == 0 ? "-" : (d == 1 ? "L" : d == -1 ? "R" : "#");
    }
    static inline std::string how_str(int h)
    {
        return h == 0 ? "-" : (h == 1 ? "A" : "D");
    }

    template <typename P, typename SR, typename O, typename C>
    std::ostream& operator<<(std::ostream &os, turn_info<P, SR, O, C> const& info)
    {
        os  << "\t"
            << " src " << info.seg_id.source_index
            << " seg " << info.seg_id.segment_index
            << " (// " << info.other_id.source_index
                << "." << info.other_id.segment_index << ")"
            << " how " << info.how
                << "[" << how_str(info.arrival)
                << " " << dir(info.direction)
                << (info.opposite ? " o" : "")
                << "]"
            << " sd "
                << dir(info.sides.template get<0,0>())
                << dir(info.sides.template get<0,1>())
                << dir(info.sides.template get<1,0>())
                << dir(info.sides.template get<1,1>())
            << " nxt seg " << info.travels_to_vertex_index
            << " , ip " << info.travels_to_ip_index
            << " , or " << info.next_ip_index
            << " frac " << info.fraction
            << info.visit_state;
        if (info.flagged)
        {
            os << " FLAGGED";
        }
        return os;
    }



}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_STREAM_INFO_HPP
