// Boost.Geometry

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013-2021.
// Modifications copyright (c) 2013-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_RANGE_HPP
#define BOOST_GEOMETRY_UTIL_RANGE_HPP

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <boost/concept_check.hpp>
#include <boost/config.hpp>
#include <boost/core/addressof.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/difference_type.hpp>
#include <boost/range/has_range_iterator.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/reference.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/mutable_range.hpp>

namespace boost { namespace geometry { namespace range
{

namespace detail
{

BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator_category)

template <typename T>
struct is_iterator
    : std::integral_constant
        <
            bool,
            has_iterator_category
                <
                    std::iterator_traits<T>
                >::value
        >
{};


template <typename T, bool HasIterator = boost::has_range_iterator<T>::value>
struct is_range_impl
    : is_iterator
        <
            typename boost::range_iterator<T>::type
        >
{};
template <typename T>
struct is_range_impl<T, false>
    : std::false_type
{};

template <typename T>
struct is_range
    : is_range_impl<T>
{};

template <typename Range, typename T = void>
using enable_if_mutable_t = std::enable_if_t
    <
        (! std::is_const<std::remove_reference_t<Range>>::value),
        T
    >;


} // namespace detail


/*!
\brief Short utility to conveniently return an iterator of a RandomAccessRange.
\ingroup utility
*/
template <typename RandomAccessRange>
inline typename boost::range_iterator<RandomAccessRange>::type
pos(RandomAccessRange && rng,
    typename boost::range_size<RandomAccessRange>::type i)
{
    BOOST_RANGE_CONCEPT_ASSERT((boost::RandomAccessRangeConcept<RandomAccessRange>));
    BOOST_GEOMETRY_ASSERT(i <= boost::size(rng));
    return boost::begin(rng)
         + static_cast<typename boost::range_difference<RandomAccessRange>::type>(i);
}

/*!
\brief Short utility to conveniently return an element of a RandomAccessRange.
\ingroup utility
*/
template <typename RandomAccessRange>
inline typename boost::range_reference<RandomAccessRange>::type
at(RandomAccessRange && rng,
   typename boost::range_size<RandomAccessRange>::type i)
{
    return *pos(rng, i);
}

/*!
\brief Short utility to conveniently return the front element of a Range.
\ingroup utility
*/
template <typename Range>
inline typename boost::range_reference<Range>::type
front(Range && rng)
{
    BOOST_GEOMETRY_ASSERT(!boost::empty(rng));
    return *boost::begin(rng);
}

/*!
\brief Short utility to conveniently return the back element of a BidirectionalRange.
\ingroup utility
*/
template <typename BidirectionalRange>
inline typename boost::range_reference<BidirectionalRange>::type
back(BidirectionalRange && rng)
{
    BOOST_RANGE_CONCEPT_ASSERT((boost::BidirectionalRangeConcept<BidirectionalRange>));
    BOOST_GEOMETRY_ASSERT(!boost::empty(rng));
    auto it = boost::end(rng);
    return *(--it);
}


/*!
\brief Short utility to conveniently clear a mutable range.
       It uses traits::clear<>.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline void clear(Range && rng)
{
    geometry::traits::clear
        <
            std::remove_reference_t<Range>
        >::apply(rng);
}

/*!
\brief Short utility to conveniently insert a new element at the end of a mutable range.
       It uses boost::geometry::traits::push_back<>.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline void push_back(Range && rng,
                      typename boost::range_value<Range>::type const& value)
{
    geometry::traits::push_back
        <
            std::remove_reference_t<Range>
        >::apply(rng, value);
}

/*!
\brief Short utility to conveniently insert a new element at the end of a mutable range.
       It uses boost::geometry::traits::push_back<>.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline void push_back(Range && rng,
                      typename boost::range_value<Range>::type && value)
{
    geometry::traits::push_back
        <
            std::remove_reference_t<Range>
        >::apply(rng, std::move(value));
}

/*!
\brief Short utility to conveniently insert a new element at the end of a mutable range.
       It uses boost::geometry::traits::emplace_back<>.
\ingroup utility
*/
template
<
    typename Range,
    typename ...Args,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline void emplace_back(Range && rng, Args &&... args)
{
    geometry::traits::emplace_back
        <
            std::remove_reference_t<Range>
        >::apply(rng, std::forward<Args>(args)...);
}

/*!
\brief Short utility to conveniently resize a mutable range.
       It uses boost::geometry::traits::resize<>.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline void resize(Range && rng,
                   typename boost::range_size<Range>::type new_size)
{
    geometry::traits::resize
        <
            std::remove_reference_t<Range>
        >::apply(rng, new_size);
}

/*!
\brief Short utility to conveniently remove an element from the back of a mutable range.
       It uses resize().
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline void pop_back(Range && rng)
{
    BOOST_GEOMETRY_ASSERT(!boost::empty(rng));
    range::resize(rng, boost::size(rng) - 1);
}

/*!
\brief Short utility to conveniently remove an element from a mutable range.
       It uses std::move() and resize(). Version taking mutable iterators.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline typename boost::range_iterator<Range>::type
erase(Range && rng,
      typename boost::range_iterator<Range>::type it)
{
    BOOST_GEOMETRY_ASSERT(!boost::empty(rng));
    BOOST_GEOMETRY_ASSERT(it != boost::end(rng));

    typename boost::range_difference<Range>::type const
        d = std::distance(boost::begin(rng), it);

    typename boost::range_iterator<Range>::type
        next = it;
    ++next;

    std::move(next, boost::end(rng), it);
    range::resize(rng, boost::size(rng) - 1);

    // NOTE: In general this should be sufficient:
    //    return it;
    // But in MSVC using the returned iterator causes
    // assertion failures when iterator debugging is enabled
    // Furthermore the code below should work in the case if resize()
    // invalidates iterators when the container is resized down.
    return boost::begin(rng) + d;
}

/*!
\brief Short utility to conveniently remove an element from a mutable range.
       It uses std::move() and resize(). Version taking non-mutable iterators.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline typename boost::range_iterator<Range>::type
erase(Range && rng,
      typename boost::range_iterator<std::remove_reference_t<Range> const>::type cit)
{
    BOOST_RANGE_CONCEPT_ASSERT(( boost::RandomAccessRangeConcept<Range> ));

    typename boost::range_iterator<Range>::type
        it = boost::begin(rng)
                + std::distance(boost::const_begin(rng), cit);

    return range::erase(rng, it);
}

/*!
\brief Short utility to conveniently remove a range of elements from a mutable range.
       It uses std::move() and resize(). Version taking mutable iterators.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline typename boost::range_iterator<Range>::type
erase(Range && rng,
      typename boost::range_iterator<Range>::type first,
      typename boost::range_iterator<Range>::type last)
{
    typename boost::range_difference<Range>::type const
        diff = std::distance(first, last);
    BOOST_GEOMETRY_ASSERT(diff >= 0);

    std::size_t const count = static_cast<std::size_t>(diff);
    BOOST_GEOMETRY_ASSERT(count <= boost::size(rng));
    
    if ( count > 0 )
    {
        typename boost::range_difference<Range>::type const
            d = std::distance(boost::begin(rng), first);

        std::move(last, boost::end(rng), first);
        range::resize(rng, boost::size(rng) - count);

        // NOTE: In general this should be sufficient:
        //    return first;
        // But in MSVC using the returned iterator causes
        // assertion failures when iterator debugging is enabled
        // Furthermore the code below should work in the case if resize()
        // invalidates iterators when the container is resized down.
        return boost::begin(rng) + d;
    }

    return first;
}

/*!
\brief Short utility to conveniently remove a range of elements from a mutable range.
       It uses std::move() and resize(). Version taking non-mutable iterators.
\ingroup utility
*/
template
<
    typename Range,
    detail::enable_if_mutable_t<Range, int> = 0
>
inline typename boost::range_iterator<Range>::type
erase(Range && rng,
      typename boost::range_iterator<std::remove_reference_t<Range> const>::type cfirst,
      typename boost::range_iterator<std::remove_reference_t<Range> const>::type clast)
{
    BOOST_RANGE_CONCEPT_ASSERT(( boost::RandomAccessRangeConcept<Range> ));

    typename boost::range_iterator<Range>::type
        first = boost::begin(rng)
                    + std::distance(boost::const_begin(rng), cfirst);
    typename boost::range_iterator<Range>::type
        last = boost::begin(rng)
                    + std::distance(boost::const_begin(rng), clast);

    return range::erase(rng, first, last);
}

// back_inserter

template <class Container>
class back_insert_iterator
{
public:
    typedef std::output_iterator_tag iterator_category;
    typedef void value_type;
    typedef void difference_type;
    typedef void pointer;
    typedef void reference;

    typedef Container container_type;

    explicit back_insert_iterator(Container & c)
        : container(boost::addressof(c))
    {}

    back_insert_iterator & operator=(typename Container::value_type const& value)
    {
        range::push_back(*container, value);
        return *this;
    }

    back_insert_iterator & operator=(typename Container::value_type && value)
    {
        range::push_back(*container, std::move(value));
        return *this;
    }

    back_insert_iterator & operator* ()
    {
        return *this;
    }

    back_insert_iterator & operator++ ()
    {
        return *this;
    }

    back_insert_iterator operator++(int)
    {
        return *this;
    }

private:
    Container * container;
};

template <typename Range>
inline back_insert_iterator<Range> back_inserter(Range & rng)
{
    return back_insert_iterator<Range>(rng);
}

}}} // namespace boost::geometry::range

#endif // BOOST_GEOMETRY_UTIL_RANGE_HPP
