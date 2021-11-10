// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_DEBUG_HPP
#define BOOST_HISTOGRAM_DETAIL_DEBUG_HPP

#warning "debug.hpp included"

#include <boost/histogram/detail/type_name.hpp>
#include <iostream>

#define DEBUG(x)                                                                      \
  std::cout << __FILE__ << ":" << __LINE__ << " ["                                    \
            << boost::histogram::detail::type_name<decltype(x)>() << "] " #x "=" << x \
            << std::endl;

#endif
