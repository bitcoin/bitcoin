//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_RESULT_OF_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_RESULT_OF_HPP

#include <boost/utility/result_of.hpp>

namespace boost {
namespace compute {

/// Returns the result of \c Function when called with \c Args.
///
/// For example,
/// \code
/// // int + int = int
/// result_of<plus(int, int)>::type == int
/// \endcode
template<class Signature>
struct result_of
{
    // the default implementation uses the TR1-style result_of protocol. note
    // that we explicitly do *not* use the C++11 decltype operator as we want
    // the result type as it would be on an OpenCL device, not the actual C++
    // type resulting from "invoking" the function on the host.
    typedef typename ::boost::tr1_result_of<Signature>::type type;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_TYPE_TRAITS_RESULT_OF_HPP
