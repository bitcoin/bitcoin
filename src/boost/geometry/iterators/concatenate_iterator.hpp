// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ITERATORS_CONCATENATE_ITERATOR_HPP
#define BOOST_GEOMETRY_ITERATORS_CONCATENATE_ITERATOR_HPP


#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_categories.hpp>


namespace boost { namespace geometry
{



template
<
    typename Iterator1,
    typename Iterator2,
    typename Value,
    typename Reference = Value&
>
class concatenate_iterator
    : public boost::iterator_facade
        <
            concatenate_iterator<Iterator1, Iterator2, Value, Reference>,
            Value,
            boost::bidirectional_traversal_tag,
            Reference
        >
{
private:
    Iterator1 m_it1, m_end1;
    Iterator2 m_begin2, m_it2;

public:
    typedef Iterator1 first_iterator_type;
    typedef Iterator2 second_iterator_type;

    // default constructor
    concatenate_iterator() = default;

    // for begin
    concatenate_iterator(Iterator1 it1, Iterator1 end1,
                         Iterator2 begin2, Iterator2 it2)
        : m_it1(it1), m_end1(end1), m_begin2(begin2), m_it2(it2)
    {}

    // for end
    concatenate_iterator(Iterator1 end1, Iterator2 begin2, Iterator2 end2)
        : m_it1(end1), m_end1(end1), m_begin2(begin2), m_it2(end2)
    {}

    template
    <
        typename OtherIt1,
        typename OtherIt2,
        typename OtherValue,
        typename OtherReference,
        std::enable_if_t
            <
                std::is_convertible<OtherIt1, Iterator1>::value
                && std::is_convertible<OtherIt2, Iterator2>::value,
                int
            > = 0
    >
    concatenate_iterator(concatenate_iterator
                         <
                             OtherIt1,
                             OtherIt2,
                             OtherValue,
                             OtherReference
                         > const& other)
        : m_it1(other.m_it1)
        , m_end1(other.m_end1)
        , m_begin2(other.m_begin2)
        , m_it2(other.m_it2)
    {}

    concatenate_iterator(concatenate_iterator const& other) = default;

    concatenate_iterator& operator=(concatenate_iterator const& other) = default;

private:
    friend class boost::iterator_core_access;

    template <typename It1, typename It2, typename V, typename R>
    friend class concatenate_iterator;

    inline Reference dereference() const
    {
        if ( m_it1 == m_end1 )
        {
            return *m_it2;
        }
        return *m_it1;
    }

    template
    <
        typename OtherIt1,
        typename OtherIt2,
        typename OtherValue,
        typename OtherReference
    >
    inline bool equal(concatenate_iterator
                      <
                          OtherIt1,
                          OtherIt2,
                          OtherValue,
                          OtherReference
                      > const& other) const
    {
        return m_it1 == other.m_it1 && m_it2 == other.m_it2;
    }

    inline void increment()
    {
        if ( m_it1 == m_end1 )
        {
            ++m_it2;
        }
        else
        {
            ++m_it1;
        }
    }

    inline void decrement()
    {
        if ( m_it2 == m_begin2 )
        {
            --m_it1;
        }
        else
        {
            --m_it2;
        }
    }
};



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ITERATORS_CONCATENATE_ITERATOR_HPP
