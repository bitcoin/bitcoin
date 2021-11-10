///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_IS_COMPLEX_HPP
#define BOOST_MP_IS_COMPLEX_HPP

#include <type_traits>
#include <complex>

namespace boost { namespace multiprecision { namespace detail {

template <class T> struct is_complex : public std::integral_constant<bool, false> {};

template <class T> struct is_complex<std::complex<T> > : public std::integral_constant<bool, true> {};

}
}
} // namespace boost::multiprecision::detail

#endif // BOOST_MP_IS_BACKEND_HPP
