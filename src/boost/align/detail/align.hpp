/*
Copyright 2014-2020 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_ALIGN_HPP
#define BOOST_ALIGN_DETAIL_ALIGN_HPP

#include <boost/align/detail/is_alignment.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace alignment {

inline void*
align(std::size_t alignment, std::size_t size, void*& ptr,
    std::size_t& space)
{
    BOOST_ASSERT(boost::alignment::detail::is_alignment(alignment));
    if (size <= space) {
        char* p = reinterpret_cast<char*>(~(alignment - 1) &
            (reinterpret_cast<std::size_t>(ptr) + alignment - 1));
        std::size_t n = p - static_cast<char*>(ptr);
        if (n <= space - size) {
            ptr = p;
            space -= n;
            return p;
        }
    }
    return 0;
}

} /* alignment */
} /* boost */

#endif
