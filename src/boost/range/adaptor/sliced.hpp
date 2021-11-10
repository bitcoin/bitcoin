// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_SLICED_HPP
#define BOOST_RANGE_ADAPTOR_SLICED_HPP

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/size_type.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/concepts.hpp>
#include <boost/next_prior.hpp>

namespace boost
{
    namespace adaptors
    {
        struct sliced
        {
            sliced(std::size_t t_, std::size_t u_)
                : t(t_), u(u_) {}
            std::size_t t;
            std::size_t u;
        };

        template< class RandomAccessRange >
        class sliced_range : public boost::iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<RandomAccessRange>::type >
        {
            typedef boost::iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<RandomAccessRange>::type > base_t;
        public:
            template<typename Rng, typename T, typename U>
            sliced_range(Rng& rng, T t, U u)
                : base_t(boost::next(boost::begin(rng), t),
                         boost::next(boost::begin(rng), u))
            {
            }
        };

        template< class RandomAccessRange >
        inline sliced_range<RandomAccessRange>
        slice( RandomAccessRange& rng, std::size_t t, std::size_t u )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                RandomAccessRangeConcept<RandomAccessRange>));

            BOOST_ASSERT( t <= u && "error in slice indices" );
            BOOST_ASSERT( static_cast<std::size_t>(boost::size(rng)) >= u &&
                          "second slice index out of bounds" );

            return sliced_range<RandomAccessRange>(rng, t, u);
        }

        template< class RandomAccessRange >
        inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<const RandomAccessRange>::type >
        slice( const RandomAccessRange& rng, std::size_t t, std::size_t u )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                RandomAccessRangeConcept<const RandomAccessRange>));

            BOOST_ASSERT( t <= u && "error in slice indices" );
            BOOST_ASSERT( static_cast<std::size_t>(boost::size(rng)) >= u &&
                          "second slice index out of bounds" );

            return sliced_range<const RandomAccessRange>(rng, t, u);
        }

        template< class RandomAccessRange >
        inline sliced_range<RandomAccessRange>
        operator|( RandomAccessRange& r, const sliced& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                RandomAccessRangeConcept<RandomAccessRange>));

            return sliced_range<RandomAccessRange>( r, f.t, f.u );
        }

        template< class RandomAccessRange >
        inline sliced_range<const RandomAccessRange>
        operator|( const RandomAccessRange& r, const sliced& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                RandomAccessRangeConcept<const RandomAccessRange>));

            return sliced_range<const RandomAccessRange>( r, f.t, f.u );
        }

    } // namespace adaptors
    using adaptors::sliced_range;
} // namespace boost

#endif
