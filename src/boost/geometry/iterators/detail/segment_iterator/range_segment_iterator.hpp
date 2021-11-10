// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ITERATORS_DETAIL_SEGMENT_ITERATOR_RANGE_SEGMENT_ITERATOR_HPP
#define BOOST_GEOMETRY_ITERATORS_DETAIL_SEGMENT_ITERATOR_RANGE_SEGMENT_ITERATOR_HPP


#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/iterators/closing_iterator.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace segment_iterator
{


template <typename Range, closure_selector Closure = closure<Range>::value>
struct range_iterator_type
{
    typedef typename boost::range_iterator<Range>::type type;
};

template <typename Range>
struct range_iterator_type<Range, open>
{
    typedef closing_iterator<Range> type;
};



template <typename Range, closure_selector Closure = closure<Range>::value>
struct range_iterator_begin
{
    static inline typename range_iterator_type<Range, Closure>::type
    apply(Range& range)
    {
        return boost::begin(range);
    }
};

template <typename Range>
struct range_iterator_begin<Range, open>
{
    static inline typename range_iterator_type<Range, open>::type
    apply(Range& range)
    {
        return closing_iterator<Range>(range);
    }
};



template <typename Range, closure_selector Closure = closure<Range>::value>
struct range_iterator_end
{
    static inline typename range_iterator_type<Range, Closure>::type
    apply(Range& range)
    {
        return boost::end(range);
    }
};

template <typename Range>
struct range_iterator_end<Range, open>
{
    static inline typename range_iterator_type<Range, open>::type
    apply(Range& range)
    {
        return closing_iterator<Range>(range, true);
    }
};






template <typename Range, typename Value, typename Reference = Value>
class range_segment_iterator
    : public boost::iterator_facade
        <
            range_segment_iterator<Range, Value, Reference>,
            Value,
            boost::bidirectional_traversal_tag,
            Reference
        >
{
    static inline bool has_less_than_two_elements(Range const& r)
    {
        return boost::size(r) < ((closure<Range>::value == open) ? 1u : 2u);
    }

public:
    typedef typename range_iterator_type<Range>::type iterator_type;

    // default constructor
    range_segment_iterator()
        : m_it(), m_has_less_than_two_elements(false)
    {}

    // for begin
    range_segment_iterator(Range& r)
        : m_it(range_iterator_begin<Range>::apply(r))
        , m_has_less_than_two_elements(has_less_than_two_elements(r))
    {}

    // for end
    range_segment_iterator(Range& r, bool)
        : m_it(range_iterator_end<Range>::apply(r))
        , m_has_less_than_two_elements(has_less_than_two_elements(r))
    {
        if (! m_has_less_than_two_elements)
        {
            // the range consists of at least two items
            --m_it;
        }
    }

    template
    <
        typename OtherRange,
        typename OtherValue,
        typename OtherReference
    >
    range_segment_iterator(range_segment_iterator
                           <
                               OtherRange,
                               OtherValue,
                               OtherReference
                           > const& other)
        : m_it(other.m_it)
    {
        typedef typename range_segment_iterator
            <
                OtherRange, OtherValue, OtherReference
            >::iterator_type other_iterator_type;

        static const bool are_conv
            = std::is_convertible<other_iterator_type, iterator_type>::value;

        BOOST_GEOMETRY_STATIC_ASSERT((are_conv),
            "Other iterator has to be convertible to member iterator.",
            OtherRange);
    }

private:
    friend class boost::iterator_core_access;

    template <typename Rng, typename V, typename R>
    friend class range_segment_iterator;

    inline Reference dereference() const
    {
        if (m_has_less_than_two_elements)
        {
            return Reference(*m_it, *m_it);
        }

        iterator_type next(m_it);
        ++next;
        return Reference(*m_it, *next);
    }

    template
    <
        typename OtherRange,
        typename OtherValue,
        typename OtherReference
    >
    inline bool equal(range_segment_iterator
                      <
                          OtherRange,
                          OtherValue,
                          OtherReference
                      > const& other) const
    {
        return m_it == other.m_it;
    }

    inline void increment()
    {
        ++m_it;
    }

    inline void decrement()
    {
        --m_it;
    }

private:
    iterator_type m_it;
    bool m_has_less_than_two_elements;
};


}} // namespace detail::segment_iterator
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ITERATORS_DETAIL_SEGMENT_ITERATOR_RANGE_SEGMENT_ITERATOR_HPP
