///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_COMPLEX128_HPP
#define BOOST_MP_COMPLEX128_HPP

#include <boost/multiprecision/float128.hpp>
#include <boost/multiprecision/complex_adaptor.hpp>

namespace boost {
namespace multiprecision {

using complex128 = number<complex_adaptor<float128_backend>, et_off>;

template <>
struct component_type<number<complex_adaptor<float128_backend> > >
{
   using type = float128;
};

}
} // namespace boost::multiprecision

#endif
