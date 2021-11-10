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

#ifndef BOOST_GEOMETRY_ITERATORS_EVER_CIRCLING_ITERATOR_HPP
#define BOOST_GEOMETRY_ITERATORS_EVER_CIRCLING_ITERATOR_HPP


#include <type_traits>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/difference_type.hpp>
#include <boost/range/reference.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>

#include <boost/geometry/iterators/detail/iterator_base.hpp>


namespace boost { namespace geometry
{

/*!
    \brief Iterator which ever circles through a range
    \tparam Iterator iterator on which this class is based on
    \ingroup iterators
    \details If the iterator arrives at range.end() it restarts from the
     beginning. So it has to be stopped in another way.
    Don't call for(....; it++) because it will turn in an endless loop
    \note Name inspired on David Bowie's
    "Chant Of The Ever Circling Skeletal Family"
*/
template <typename Iterator>
struct ever_circling_iterator :
    public detail::iterators::iterator_base
    <
        ever_circling_iterator<Iterator>,
        Iterator
    >
{
    ever_circling_iterator() = default;

    explicit inline ever_circling_iterator(Iterator begin, Iterator end,
            bool skip_first = false)
      : m_begin(begin)
      , m_end(end)
      , m_skip_first(skip_first)
    {
        this->base_reference() = begin;
    }

    explicit inline ever_circling_iterator(Iterator begin, Iterator end, Iterator start,
            bool skip_first = false)
      : m_begin(begin)
      , m_end(end)
      , m_skip_first(skip_first)
    {
        this->base_reference() = start;
    }

    template
    <
        typename OtherIterator,
        std::enable_if_t<std::is_convertible<OtherIterator, Iterator>::value, int> = 0
    >
    inline ever_circling_iterator(ever_circling_iterator<OtherIterator> const& other)
        : m_begin(other.m_begin)
        , m_end(other.m_end)
        , m_skip_first(other.m_skip_first)
    {}

    /// Navigate to a certain position, should be in [start .. end], if at end
    /// it will circle again.
    inline void moveto(Iterator it)
    {
        this->base_reference() = it;
        check_end();
    }

private:
    template <typename OtherIterator> friend struct ever_circling_iterator;
    friend class boost::iterator_core_access;

    inline void increment(bool possibly_skip = true)
    {
        (this->base_reference())++;
        check_end(possibly_skip);
    }

    inline void check_end(bool possibly_skip = true)
    {
        if (this->base() == this->m_end)
        {
            this->base_reference() = this->m_begin;
            if (m_skip_first && possibly_skip)
            {
                increment(false);
            }
        }
    }

    Iterator m_begin;
    Iterator m_end;
    bool m_skip_first;
};

template <typename Range>
struct ever_circling_range_iterator
    : public boost::iterator_facade
    <
        ever_circling_range_iterator<Range>,
        typename boost::range_value<Range>::type const,
        boost::random_access_traversal_tag,
        typename boost::range_reference<Range const>::type,
        typename boost::range_difference<Range>::type
    >
{
private:
    typedef boost::iterator_facade
        <
            ever_circling_range_iterator<Range>,
            typename boost::range_value<Range>::type const,
            boost::random_access_traversal_tag,
            typename boost::range_reference<Range const>::type,
            typename boost::range_difference<Range>::type
        > base_type;

public:
    /// Constructor including the range it is based on
    explicit inline ever_circling_range_iterator(Range const& range)
        : m_begin(boost::begin(range))
        , m_iterator(boost::begin(range))
        , m_size(boost::size(range))
        , m_index(0)
    {}

    /// Default constructor
    explicit inline ever_circling_range_iterator()
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
    inline ever_circling_range_iterator(ever_circling_range_iterator<OtherRange> const& other)
        : m_begin(other.m_begin)
        , m_iterator(other.m_iterator)
        , m_size(other.m_size)
        , m_index(other.m_index)
    {}

    typedef typename base_type::reference reference;
    typedef typename base_type::difference_type difference_type;

private:
    template <typename OtherRange> friend struct ever_circling_range_iterator;
    friend class boost::iterator_core_access;

    inline reference dereference() const
    {
        return *m_iterator;
    }

    inline difference_type distance_to(ever_circling_range_iterator<Range> const& other) const
    {
        return other.m_index - this->m_index;
    }

    inline bool equal(ever_circling_range_iterator<Range> const& other) const
    {
        BOOST_GEOMETRY_ASSERT(m_begin == other.m_begin);
        return this->m_index == other.m_index;
    }

    inline void increment()
    {
        ++m_index;
        if (m_index >= 0 && m_index < m_size)
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
        --m_index;
        if (m_index >= 0 && m_index < m_size)
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
        if (m_index >= 0 && m_index < m_size
            && m_index + n >= 0 && m_index + n < m_size)
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
        while (m_index < 0)
        {
            m_index += m_size;
        }
        m_index = m_index % m_size;
        this->m_iterator = m_begin + m_index;
    }

    typename boost::range_iterator<Range const>::type m_begin;
    typename boost::range_iterator<Range const>::type m_iterator;
    difference_type m_size;
    difference_type m_index;
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ITERATORS_EVER_CIRCLING_ITERATOR_HPP
