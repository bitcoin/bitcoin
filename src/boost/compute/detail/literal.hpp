//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_LITERAL_HPP
#define BOOST_COMPUTE_DETAIL_LITERAL_HPP

#include <iomanip>
#include <limits>
#include <sstream>

#include <boost/type_traits/is_same.hpp>

#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T>
std::string make_literal(T x)
{
    std::stringstream s;
    s << std::setprecision(
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
            std::numeric_limits<T>::max_digits10
#else
            // We don't have max_digits10, so add 3 other digits (this is what is required for
            // float, and is one more than required for double).
            3 + std::numeric_limits<T>::digits10
#endif
            )
      << std::scientific
      << x;

    if(boost::is_same<T, float>::value || boost::is_same<T, float_>::value){
        s << "f";
    }

    return s.str();
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_LITERAL_HPP
