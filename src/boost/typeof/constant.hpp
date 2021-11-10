/*
Copyright 2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_TYPEOF_CONSTANT_HPP
#define BOOST_TYPEOF_CONSTANT_HPP

#include <boost/config.hpp>

namespace boost {
namespace type_of {

template<class T, T N>
struct constant {
    typedef constant<T, N> type;
    typedef constant<T, N + 1> next;
    BOOST_STATIC_CONSTANT(T, value=N);
};

} /* type_of */
} /* boost */

#endif
