//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_REMOVE_COPY_IF_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_REMOVE_COPY_IF_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    /// \brief template function remove_copy_if
    ///
    /// range-based version of the remove_copy_if std algorithm
    ///
    /// \pre SinglePassRange is a model of the SinglePassRangeConcept
    /// \pre OutputIterator is a model of the OutputIteratorConcept
    /// \pre Predicate is a model of the PredicateConcept
    /// \pre InputIterator's value type is convertible to Predicate's argument type
    /// \pre out_it is not an iterator in the range rng
    template< class SinglePassRange, class OutputIterator, class Predicate >
    inline OutputIterator
    remove_copy_if(const SinglePassRange& rng, OutputIterator out_it, Predicate pred)
    {
        BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange> ));
        return std::remove_copy_if(boost::begin(rng), boost::end(rng), out_it, pred);
    }
}

#endif // include guard
