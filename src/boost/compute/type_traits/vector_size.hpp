//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_VECTOR_SIZE_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_VECTOR_SIZE_HPP

#include <boost/preprocessor/cat.hpp>

#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// Meta-function returning the size (number of components) of a vector type
/// \p T. For scalar types this function returns \c 1.
///
/// For example,
/// \code
/// vector_size<float>::value == 1
/// vector_size<float4_>::value == 4
/// \endcode
template<class T>
struct vector_size
{
    /// \internal_
    BOOST_STATIC_CONSTANT(size_t, value = 1);
};

/// \internal_
#define BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTION(scalar, size) \
    template<> \
    struct vector_size<BOOST_PP_CAT(BOOST_PP_CAT(scalar, size), _)> \
    { \
        BOOST_STATIC_CONSTANT(size_t, value = size); \
    };

/// \internal_
#define BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(scalar) \
    BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTION(scalar, 2) \
    BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTION(scalar, 4) \
    BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTION(scalar, 8) \
    BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTION(scalar, 16)

BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(char)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(uchar)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(short)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(ushort)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(int)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(uint)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(long)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(ulong)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(float)
BOOST_COMPUTE_DECLARE_VECTOR_SIZE_FUNCTIONS(double)

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_TYPE_TRAITS_VECTOR_SIZE_HPP
