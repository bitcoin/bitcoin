/*
Copyright 2014 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_IS_ALIGNED_HPP
#define BOOST_ALIGN_DETAIL_IS_ALIGNED_HPP

#include <boost/align/detail/is_alignment.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace alignment {

inline bool
is_aligned(const volatile void* ptr, std::size_t alignment) BOOST_NOEXCEPT
{
    BOOST_ASSERT(detail::is_alignment(alignment));
    return (reinterpret_cast<std::size_t>(ptr) & (alignment - 1)) == 0;
}

inline bool
is_aligned(std::size_t alignment, const volatile void* ptr) BOOST_NOEXCEPT
{
    BOOST_ASSERT(detail::is_alignment(alignment));
    return (reinterpret_cast<std::size_t>(ptr) & (alignment - 1)) == 0;
}

} /* alignment */
} /* boost */

#endif
