// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020-2021.
// Modifications copyright (c) 2020-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_VIEWS_IDENTITY_VIEW_HPP
#define BOOST_GEOMETRY_VIEWS_IDENTITY_VIEW_HPP

#include <boost/range/iterator_range.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>


namespace boost { namespace geometry
{

// Silence warning C4512: assignment operator could not be generated
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4512)
#endif

/*!
\brief View on a range, not modifying anything
\tparam Range original range
\ingroup views
*/
template <typename Range>
struct identity_view
{
    using const_iterator = typename boost::range_iterator<Range const>::type;
    using iterator = typename boost::range_iterator<Range>::type;

    explicit inline identity_view(Range& r)
        : m_begin(boost::begin(r))
        , m_end(boost::end(r))
    {}

    inline const_iterator begin() const { return m_begin; }
    inline const_iterator end() const { return m_end; }

    inline iterator begin() { return m_begin; }
    inline iterator end() { return m_end; }

private:
    iterator m_begin;
    iterator m_end;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_VIEWS_IDENTITY_VIEW_HPP
