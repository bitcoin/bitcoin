// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.


#ifndef BOOST_MULTI_ARRAY_TYPES_HPP
#define BOOST_MULTI_ARRAY_TYPES_HPP

//
// types.hpp - supply types that are needed by several headers
//
#include "boost/config.hpp"
#include <cstddef>

namespace boost {
namespace detail {
namespace multi_array{

// needed typedefs
typedef std::size_t size_type;
typedef std::ptrdiff_t index;

} // namespace multi_array
} // namespace detail
} // namespace boost
  



#endif
