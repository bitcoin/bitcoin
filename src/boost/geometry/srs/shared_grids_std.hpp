// Boost.Geometry

// Copyright (c) 2018-2019, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_SHARED_GRIDS_STD_HPP
#define BOOST_GEOMETRY_SRS_SHARED_GRIDS_STD_HPP


#include <boost/config.hpp>

#ifdef BOOST_NO_CXX14_HDR_SHARED_MUTEX
#error "C++14 <shared_mutex> header required."
#endif

#include <boost/geometry/srs/projections/grids.hpp>

#include <mutex>
#include <shared_mutex>


namespace boost { namespace geometry
{
    
namespace srs
{

class shared_grids_std
{

// VS 2015 Update 2
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 190023918)
    typedef std::shared_mutex mutex_type;
// Other C++17
#elif !defined(BOOST_NO_CXX14_HDR_SHARED_MUTEX) && (__cplusplus > 201402L)
    typedef std::shared_mutex mutex_type;
#else
    typedef std::shared_timed_mutex mutex_type;
#endif

public:
    std::size_t size() const
    {
        std::shared_lock<mutex_type> lock(mutex);
        return gridinfo.size();
    }

    bool empty() const
    {
        std::shared_lock<mutex_type> lock(mutex);
        return gridinfo.empty();
    }

    typedef projections::detail::shared_grids_tag tag;

    struct read_locked
    {
        read_locked(shared_grids_std & g)
            : gridinfo(g.gridinfo)
            , lock(g.mutex)
        {}

        // should be const&
        projections::detail::pj_gridinfo & gridinfo;

    private:
        std::shared_lock<mutex_type> lock;
    };

    struct write_locked
    {
        write_locked(shared_grids_std & g)
            : gridinfo(g.gridinfo)
            , lock(g.mutex)
        {}

        projections::detail::pj_gridinfo & gridinfo;

    private:
        std::unique_lock<mutex_type> lock;
    };

private:
    projections::detail::pj_gridinfo gridinfo;
    mutable mutex_type mutex;
};


} // namespace srs


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_SHARED_GRIDS_STD_HPP
