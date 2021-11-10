// Boost.Geometry

// Copyright (c) 2018-2019, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_SHARED_GRIDS_BOOST_HPP
#define BOOST_GEOMETRY_SRS_SHARED_GRIDS_BOOST_HPP


#include <boost/geometry/srs/projections/grids.hpp>


#include <boost/thread.hpp>


namespace boost { namespace geometry
{
    
namespace srs
{

class shared_grids_boost
{
public:
    std::size_t size() const
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex);
        return gridinfo.size();
    }

    bool empty() const
    {
        boost::shared_lock<boost::shared_mutex> lock(mutex);
        return gridinfo.empty();
    }

    typedef projections::detail::shared_grids_tag tag;

    struct read_locked
    {
        read_locked(shared_grids_boost & g)
            : gridinfo(g.gridinfo)
            , lock(g.mutex)
        {}

        // should be const&
        projections::detail::pj_gridinfo & gridinfo;

    private:
        boost::shared_lock<boost::shared_mutex> lock;
    };

    struct write_locked
    {
        write_locked(shared_grids_boost & g)
            : gridinfo(g.gridinfo)
            , lock(g.mutex)
        {}

        projections::detail::pj_gridinfo & gridinfo;

    private:
        boost::unique_lock<boost::shared_mutex> lock;
    };

private:
    projections::detail::pj_gridinfo gridinfo;
    mutable boost::shared_mutex mutex;
};


} // namespace srs


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_SHARED_GRIDS_BOOST_HPP
