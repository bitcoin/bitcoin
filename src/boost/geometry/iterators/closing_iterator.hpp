// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Copyright (c) 2020-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ITERATORS_CLOSING_ITERATOR_HPP
#define BOOST_GEOMETRY_ITERATORS_CLOSING_ITERATOR_HPP


#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/difference_type.hpp>
#include <boost/range/reference.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>


namespace boost { namespace geometry
{

/*!
\brief Iterator which iterates through a range, but adds first element at end of the range
\tparam Range range on which this class is based on
\ingroup iterators
\note It's const iterator treating the Range as one containing non-mutable elements.
        For both "closing_iterator<Range> and "closing_iterator<Range const>
        const reference is always returned when dereferenced.
\note This class is normally used from "closeable_view" if Close==true
*/
template <typename Range>
struct closing_iterator
    : public boost::iterator_facade
    <
        closing_iterator<Range>,
        typename boost::range_value<Range>::type const,
        boost::random_access_traversal_tag,
        typename boost::range_reference<Range const>::type,
        typename boost::range_difference<Range>::type
    >
{
private:
    typedef boost::iterator_facade
        <
            closing_iterator<Range>,
            typename boost::range_value<Range>::type const,
            boost::random_access_traversal_tag,
            typename boost::range_reference<Range const>::type,
            typename boost::range_difference<Range>::type
        > base_type;

public:
    typedef typename base_type::reference reference;
    typedef typename base_type::difference_type difference_type;

    /// Constructor including the range it is based on
    explicit inline closing_iterator(Range const& range)
        : m_iterator(boost::begin(range))
        , m_begin(boost::begin(range))
        , m_end(boost::end(range))
        , m_size(m_end - m_begin)
        , m_index(0)
    {}

    /// Constructor to indicate the end of a range
    explicit inline closing_iterator(Range const& range, bool)
        : m_iterator(boost::end(range))
        , m_begin(boost::begin(range))
        , m_end(boost::end(range))
        , m_size(m_end - m_begin)
        , m_index((m_size == 0) ? 0 : m_size + 1)
    {}

    /// Default constructor
    inline closing_iterator()
        : m_size(0)
        , m_index(0)
    {}

    template
    <
        typename OtherRange,
        std::enable_if_t
            <
                std::is_convertible
                    <
                        typename boost::range_iterator<OtherRange const>::type,
                        typename boost::range_iterator<Range const>::type
                    >::value,
                int
            > = 0
    >
    inline closing_iterator(closing_iterator<OtherRange> const& other)
        : m_iterator(other.m_iterator)
        , m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_size(other.m_size)
        , m_index(other.m_index)
    {}

private:
    template <typename OtherRange> friend struct closing_iterator;
    friend class boost::iterator_core_access;

    inline reference dereference() const
    {
        return *m_iterator;
    }

    inline difference_type distance_to(closing_iterator<Range> const& other) const
    {
        return other.m_index - this->m_index;
    }

    inline bool equal(closing_iterator<Range> const& other) const
    {
        BOOST_GEOMETRY_ASSERT(m_begin == other.m_begin && m_end == other.m_end);
        return this->m_index == other.m_index;
    }

    inline void increment()
    {
        if (++m_index < m_size)
        {
            ++m_iterator;
        }
        else
        {
            update_iterator();
        }
    }

    inline void decrement()
    {
        if (m_index-- < m_size)
        {
            --m_iterator;
        }
        else
        {
            update_iterator();
        }
    }

    inline void advance(difference_type n)
    {
        if (m_index < m_size && m_index + n < m_size)
        {
            m_index += n;
            m_iterator += n;
        }
        else
        {
            m_index += n;
            update_iterator();
        }
    }

    inline void update_iterator()
    {
        this->m_iterator = m_index <= m_size
            ? m_begin + (m_index % m_size)
            : m_end
            ;
    }

    typename boost::range_iterator<Range const>::type m_iterator;
    typename boost::range_iterator<Range const>::type m_begin;
    typename boost::range_iterator<Range const>::type m_end;
    difference_type m_size;
    difference_type m_index;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ITERATORS_CLOSING_ITERATOR_HPP
