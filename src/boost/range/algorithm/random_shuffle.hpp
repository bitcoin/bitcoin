//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_RANDOM_SHUFFLE_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_RANDOM_SHUFFLE_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>
#ifdef BOOST_NO_CXX98_RANDOM_SHUFFLE
#include <cstdlib>
#endif

namespace boost
{
    namespace range
    {

        namespace detail
        {
#ifdef BOOST_NO_CXX98_RANDOM_SHUFFLE

// wrap std::rand as UniformRandomBitGenerator
struct wrap_rand
{
    typedef unsigned int result_type;

    static BOOST_CONSTEXPR result_type (min)()
    {
        return 0;
    }

    static BOOST_CONSTEXPR result_type (max)()
    {
        return RAND_MAX;
    }

    result_type operator()()
    {
        return std::rand();
    }
};

template< class RandomIt >
inline void random_shuffle(RandomIt first, RandomIt last)
{
    std::shuffle(first, last, wrap_rand());
}

// wrap Generator as UniformRandomBitGenerator
template< class Generator >
struct wrap_generator
{
    typedef unsigned int result_type;
    static const int max_arg = ((0u - 1u) >> 2) + 1;
    Generator& g;

    wrap_generator(Generator& gen) : g(gen) {}

    static BOOST_CONSTEXPR result_type (min)()
    {
        return 0;
    }

    static BOOST_CONSTEXPR result_type (max)()
    {
        return max_arg - 1;
    }

    result_type operator()()
    {
        return static_cast<result_type>(g(max_arg));
    }
};

template< class RandomIt, class Generator >
inline void random_shuffle(RandomIt first, RandomIt last, Generator& gen)
{
    std::shuffle(first, last, wrap_generator< Generator >(gen));
}

#else
    
using std::random_shuffle;

#endif  
        } // namespace detail

/// \brief template function random_shuffle
///
/// range-based version of the random_shuffle std algorithm
///
/// \pre RandomAccessRange is a model of the RandomAccessRangeConcept
/// \pre Generator is a model of the UnaryFunctionConcept
template<class RandomAccessRange>
inline RandomAccessRange& random_shuffle(RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    detail::random_shuffle(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange>
inline const RandomAccessRange& random_shuffle(const RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    detail::random_shuffle(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange, class Generator>
inline RandomAccessRange& random_shuffle(RandomAccessRange& rng, Generator& gen)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    detail::random_shuffle(boost::begin(rng), boost::end(rng), gen);
    return rng;
}

/// \overload
template<class RandomAccessRange, class Generator>
inline const RandomAccessRange& random_shuffle(const RandomAccessRange& rng, Generator& gen)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    detail::random_shuffle(boost::begin(rng), boost::end(rng), gen);
    return rng;
}

    } // namespace range
    using range::random_shuffle;
} // namespace boost

#endif // include guard
