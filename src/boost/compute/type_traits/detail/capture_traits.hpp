//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_DETAIL_CAPTURE_TRAITS_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_DETAIL_CAPTURE_TRAITS_HPP

#include <boost/compute/type_traits/type_name.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T>
struct capture_traits
{
    static std::string type_name()
    {
        return ::boost::compute::type_name<T>();
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_TYPE_TRAITS_DETAIL_CAPTURE_TRAITS_HPP
