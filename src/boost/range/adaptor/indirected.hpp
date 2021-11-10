// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_INDIRECTED_HPP
#define BOOST_RANGE_ADAPTOR_INDIRECTED_HPP

#include <boost/range/iterator_range.hpp>
#include <boost/range/concepts.hpp>
#include <boost/iterator/indirect_iterator.hpp>

namespace boost
{
    namespace range_detail
    {
        template< class R >
        struct indirected_range :
            public boost::iterator_range<
                        boost::indirect_iterator<
                            BOOST_DEDUCED_TYPENAME range_iterator<R>::type
                        >
                    >
        {
        private:
            typedef boost::iterator_range<
                        boost::indirect_iterator<
                            BOOST_DEDUCED_TYPENAME range_iterator<R>::type
                        >
                    >
                base;

        public:
            explicit indirected_range( R& r )
                : base( r )
            { }
        };

        struct indirect_forwarder {};

        template< class SinglePassRange >
        inline indirected_range<SinglePassRange>
        operator|( SinglePassRange& r, indirect_forwarder )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return indirected_range<SinglePassRange>( r );
        }

        template< class SinglePassRange >
        inline indirected_range<const SinglePassRange>
        operator|( const SinglePassRange& r, indirect_forwarder )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return indirected_range<const SinglePassRange>( r );
        }

    } // 'range_detail'

    using range_detail::indirected_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::indirect_forwarder indirected =
                                            range_detail::indirect_forwarder();
        }

        template<class SinglePassRange>
        inline indirected_range<SinglePassRange>
        indirect(SinglePassRange& rng)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));
            return indirected_range<SinglePassRange>(rng);
        }

        template<class SinglePassRange>
        inline indirected_range<const SinglePassRange>
        indirect(const SinglePassRange& rng)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return indirected_range<const SinglePassRange>(rng);
        }
    } // 'adaptors'

}

#endif
