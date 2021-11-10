//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_UNIQUE_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_UNIQUE_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/detail/range_return.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function unique
///
/// range-based version of the unique std algorithm
///
/// \pre Rng meets the requirements for a Forward range
template< range_return_value re, class ForwardRange >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
unique( ForwardRange& rng )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return range_return<ForwardRange,re>::
        pack( std::unique( boost::begin(rng),
                           boost::end(rng)), rng );
}

/// \overload
template< range_return_value re, class ForwardRange >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
unique( const ForwardRange& rng )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return range_return<const ForwardRange,re>::
        pack( std::unique( boost::begin(rng),
                           boost::end(rng)), rng );
}
/// \overload
template< range_return_value re, class ForwardRange, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
unique( ForwardRange& rng, BinaryPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return range_return<ForwardRange,re>::
        pack(std::unique(boost::begin(rng), boost::end(rng), pred),
             rng);
}
/// \overload
template< range_return_value re, class ForwardRange, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
unique( const ForwardRange& rng, BinaryPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return range_return<const ForwardRange,re>::
        pack(std::unique(boost::begin(rng), boost::end(rng), pred),
             rng);
}

/// \overload
template< class ForwardRange >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange, return_begin_found>::type
unique( ForwardRange& rng )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return ::boost::range::unique<return_begin_found>(rng);
}
/// \overload
template< class ForwardRange >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange, return_begin_found>::type
unique( const ForwardRange& rng )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return ::boost::range::unique<return_begin_found>(rng);
}
/// \overload
template< class ForwardRange, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange, return_begin_found>::type
unique( ForwardRange& rng, BinaryPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return ::boost::range::unique<return_begin_found>(rng, pred);
}
/// \overload
template< class ForwardRange, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange, return_begin_found>::type
unique( const ForwardRange& rng, BinaryPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return ::boost::range::unique<return_begin_found>(rng, pred);
}

    } // namespace range
    using range::unique;
} // namespace boost

#endif // include guard
