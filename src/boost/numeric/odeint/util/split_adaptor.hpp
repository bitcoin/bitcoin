/*
 [auto_generated]
 boost/numeric/odeint/util/split_adaptor.hpp

 [begin_description]
 A range adaptor which returns even-sized slices.
 [end_description]

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky
 Copyright 2013 Pascal Germroth

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_UTIL_SPLIT_ADAPTOR_INCLUDED
#define BOOST_NUMERIC_ODEINT_UTIL_SPLIT_ADAPTOR_INCLUDED

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/size_type.hpp>
#include <boost/range/iterator_range.hpp>
#include <algorithm>

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

/** \brief Returns the begin and end offset for a sub-range */
inline std::pair<std::size_t, std::size_t>
split_offsets( std::size_t total_length, std::size_t index, std::size_t parts )
{
    BOOST_ASSERT( parts > 0 );
    BOOST_ASSERT( index < parts );
    const std::size_t
        slice = total_length / parts,
        partial = total_length % parts,
        lo = (std::min)(index, partial),
        hi = (std::max<std::ptrdiff_t>)(0, index - partial),
        begin_offset = lo * (slice + 1) + hi * slice,
        length = slice + (index < partial ? 1 : 0),
        end_offset = begin_offset + length;
    return std::make_pair( begin_offset, end_offset );
}

/** \brief Return the sub-range `index` from a range which is split into `parts`.
 *
 * For example, splitting a range into three about equal-sized sub-ranges:
 * \code
 * sub0 = make_split_range(rng, 0, 3);
 * sub1 = rng | split(1, 3);
 * sub2 = rng | split(2, 3);
 * \endcode
 */
template< class RandomAccessRange >
inline iterator_range< typename range_iterator<RandomAccessRange>::type >
make_split_range( RandomAccessRange& rng, std::size_t index, std::size_t parts )
{
    const std::pair<std::size_t, std::size_t> off = split_offsets(boost::size(rng), index, parts);
    return make_iterator_range( boost::begin(rng) + off.first, boost::begin(rng) + off.second );
}

template< class RandomAccessRange >
inline iterator_range< typename range_iterator<const RandomAccessRange>::type >
make_split_range( const RandomAccessRange& rng, std::size_t index, std::size_t parts )
{
    const std::pair<std::size_t, std::size_t> off = split_offsets(boost::size(rng), index, parts);
    return make_iterator_range( boost::begin(rng) + off.first, boost::begin(rng) + off.second );
}


struct split
{
    split(std::size_t index, std::size_t parts)
        : index(index), parts(parts) {}
    std::size_t index, parts;
};

template< class RandomAccessRange >
inline iterator_range< typename range_iterator<RandomAccessRange>::type >
operator|( RandomAccessRange& rng, const split& f )
{
    return make_split_range( rng, f.index, f.parts );
}

template< class RandomAccessRange >
inline iterator_range< typename range_iterator<const RandomAccessRange>::type >
operator|( const RandomAccessRange& rng, const split& f )
{
    return make_split_range( rng, f.index, f.parts );
}


}
}
}
}

#endif
