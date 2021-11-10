// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_EXT_ERASE_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_EXT_ERASE_HPP_INCLUDED

#include <boost/range/config.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/difference_type.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/assert.hpp>

namespace boost
{
    namespace range
    {

template< class Container >
inline Container& erase( Container& on,
      iterator_range<BOOST_DEDUCED_TYPENAME Container::iterator> to_erase )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<Container> ));
    on.erase( boost::begin(to_erase), boost::end(to_erase) );
    return on;
}

template< class Container, class T >
inline Container& remove_erase( Container& on, const T& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<Container> ));
    on.erase(
        std::remove(boost::begin(on), boost::end(on), val),
        boost::end(on));
    return on;
}

template< class Container, class Pred >
inline Container& remove_erase_if( Container& on, Pred pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<Container> ));
    on.erase(
        std::remove_if(boost::begin(on), boost::end(on), pred),
        boost::end(on));
    return on;
}

template< class Container >
inline Container& unique_erase( Container& on)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<Container> ));
    on.erase(
        std::unique(boost::begin(on), boost::end(on)),
        boost::end(on));
    return on;
}

template< class Container, class Pred >
inline Container& unique_erase( Container& on, Pred pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<Container> ));
    on.erase(
        std::unique(boost::begin(on), boost::end(on), pred),
        boost::end(on));
    return on;
}

    } // namespace range
    using range::erase;
    using range::remove_erase;
    using range::remove_erase_if;
    using range::unique_erase;
} // namespace boost

#endif // include guard
