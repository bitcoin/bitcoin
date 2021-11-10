//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_COMMON_TYPE_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_COMMON_TYPE_HPP

#include <boost/type_traits/common_type.hpp>

#include <boost/compute/types/fundamental.hpp>

namespace boost {

/// \internal_
#define BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPE(scalar, size) \
    template<> \
    struct common_type<BOOST_COMPUTE_MAKE_VECTOR_TYPE(scalar, size), \
                       BOOST_COMPUTE_MAKE_SCALAR_TYPE(scalar)> \
    { \
        typedef BOOST_COMPUTE_MAKE_VECTOR_TYPE(scalar, size) type; \
    }; \
    template<> \
    struct common_type<BOOST_COMPUTE_MAKE_SCALAR_TYPE(scalar), \
                       BOOST_COMPUTE_MAKE_VECTOR_TYPE(scalar, size)> \
    { \
        typedef BOOST_COMPUTE_MAKE_VECTOR_TYPE(scalar, size) type; \
    };

/// \internal_
#define BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(scalar) \
    BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPE(scalar, 2) \
    BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPE(scalar, 4) \
    BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPE(scalar, 8) \
    BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPE(scalar, 16) \

BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(char)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(uchar)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(short)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(ushort)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(int)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(uint)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(long)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(ulong)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(float)
BOOST_COMPUTE_DECLARE_SCALAR_VECTOR_COMMON_TYPES(double)

} // end boost namespace

#endif // BOOST_COMPUTE_TYPE_TRAITS_COMMON_TYPE_HPP
