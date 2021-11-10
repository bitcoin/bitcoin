//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_GETENV_HPP
#define BOOST_COMPUTE_DETAIL_GETENV_HPP

#include <cstdlib>

namespace boost {
namespace compute {
namespace detail {

inline const char* getenv(const char *env_var)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4996)
#endif
    return std::getenv(env_var);
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_GETENV_HPP
