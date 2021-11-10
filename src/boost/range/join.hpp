// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_JOIN_HPP_INCLUDED
#define BOOST_RANGE_JOIN_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/range/detail/join_iterator.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost
{
    namespace range_detail
    {

template<class SinglePassRange1, class SinglePassRange2>
class joined_type
{
public:
    typedef iterator_range<
        range_detail::join_iterator<
            BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
            BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type,
            BOOST_DEDUCED_TYPENAME range_value<SinglePassRange1>::type
        >
    > type;
};

    } // namespace range_detail

namespace range
{

template<class SinglePassRange1, class SinglePassRange2>
class joined_range
    : public range_detail::joined_type<SinglePassRange1, SinglePassRange2>::type
{
    typedef range_detail::join_iterator<
        BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
        BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type,
        BOOST_DEDUCED_TYPENAME range_value<SinglePassRange1>::type
        > iterator_t;

    typedef BOOST_DEDUCED_TYPENAME range_detail::joined_type<
                    SinglePassRange1, SinglePassRange2>::type base_t;
public:
    joined_range(SinglePassRange1& rng1, SinglePassRange2& rng2)
        : base_t(
            iterator_t(rng1, rng2, range_detail::join_iterator_begin_tag()),
            iterator_t(rng1, rng2, range_detail::join_iterator_end_tag())
        )
    {
    }
};

template<class SinglePassRange1, class SinglePassRange2>
joined_range<const SinglePassRange1, const SinglePassRange2>
join(const SinglePassRange1& r1, const SinglePassRange2& r2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    return joined_range<const SinglePassRange1, const SinglePassRange2>(r1, r2);
}

template<class SinglePassRange1, class SinglePassRange2>
joined_range<SinglePassRange1, SinglePassRange2>
join(SinglePassRange1& r1, SinglePassRange2& r2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    return joined_range<SinglePassRange1, SinglePassRange2>(r1, r2);
}

} // namespace range

using ::boost::range::joined_range;
using ::boost::range::join;

} // namespace boost

#endif // include guard
