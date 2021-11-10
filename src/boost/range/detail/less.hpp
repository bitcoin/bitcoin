/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_RANGE_DETAIL_LESS
#define BOOST_RANGE_DETAIL_LESS

namespace boost {
namespace range {
namespace detail {

struct less {
    template<class T, class U>
    bool operator()(const T& lhs, const U& rhs) const {
        return lhs < rhs;
    }
};

} /* detail */
} /* range */
} /* boost */

#endif
