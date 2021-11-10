//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_GENERATE_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_GENERATE_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {
/// \brief template function generate
///
/// range-based version of the generate std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
/// \pre Generator is a model of the UnaryFunctionConcept
template< class ForwardRange, class Generator >
inline ForwardRange& generate( ForwardRange& rng, Generator gen )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    std::generate(boost::begin(rng), boost::end(rng), gen);
    return rng;
}

/// \overload
template< class ForwardRange, class Generator >
inline const ForwardRange& generate( const ForwardRange& rng, Generator gen )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    std::generate(boost::begin(rng), boost::end(rng), gen);
    return rng;
}

    } // namespace range
    using range::generate;
} // namespace boost

#endif // include guard
