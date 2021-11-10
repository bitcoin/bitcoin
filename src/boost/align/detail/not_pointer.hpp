/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_NOT_POINTER_HPP
#define BOOST_ALIGN_DETAIL_NOT_POINTER_HPP

namespace boost {
namespace alignment {
namespace detail {

template<class T, class U>
struct not_pointer {
    typedef U type;
};

template<class T, class U>
struct not_pointer<T*, U> { };

} /* detail */
} /* alignment */
} /* boost */

#endif
