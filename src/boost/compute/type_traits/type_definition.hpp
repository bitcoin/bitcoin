//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_TYPE_DEFINITION_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_TYPE_DEFINITION_HPP

#include <string>

namespace boost {
namespace compute {
namespace detail {

template<class T>
struct type_definition_trait
{
    static std::string value() { return std::string(); }
};

} // end detail namespace

/// Returns the OpenCL type definition for \c T.
///
/// \return a string containing the type definition for \c T
///
/// \see type_name<T>()
template<class T>
inline std::string type_definition()
{
    return detail::type_definition_trait<T>::value();
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_TYPE_TRAITS_TYPE_DEFINITION_HPP
