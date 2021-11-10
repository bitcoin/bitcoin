/*
Copyright 2014 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_MAX_OBJECTS_HPP
#define BOOST_ALIGN_DETAIL_MAX_OBJECTS_HPP

#include <boost/align/detail/integral_constant.hpp>
#include <cstddef>

namespace boost {
namespace alignment {
namespace detail {

template<class T>
struct max_objects
    : integral_constant<std::size_t,
        ~static_cast<std::size_t>(0) / sizeof(T)> { };

} /* detail */
} /* alignment */
} /* boost */

#endif
