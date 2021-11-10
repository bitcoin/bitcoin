//  abs.hpp
//
//  (C) Copyright 2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//

#ifndef BOOST_RATIO_MPL_RATIONAL_C_TAG_HPP
#define BOOST_RATIO_MPL_RATIONAL_C_TAG_HPP

#ifdef BOOST_RATIO_EXTENSIONS

#include <boost/mpl/int.hpp>

namespace boost {
namespace mpl {

struct rational_c_tag : int_<10> {};

}
}

#endif // BOOST_RATIO_EXTENSIONS
#endif  // BOOST_RATIO_MPL_RATIONAL_C_TAG_HPP
