/*
Copyright 2017-2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_DETAIL_DETECTOR_HPP_INCLUDED
#define BOOST_TT_DETAIL_DETECTOR_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/make_void.hpp>

namespace boost {
namespace detail {

template<class T>
using detector_t = typename boost::make_void<T>::type;

template<class Default, class, template<class...> class, class...>
struct detector {
    using value_t = boost::false_type;
    using type = Default;
};

template<class Default, template<class...> class Op, class... Args>
struct detector<Default, detector_t<Op<Args...> >, Op, Args...> {
    using value_t = boost::true_type;
    using type = Op<Args...>;
};

} /* detail */
} /* boost */

#endif
