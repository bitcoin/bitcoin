//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_MAKE_VECTOR_TYPE_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_MAKE_VECTOR_TYPE_HPP

#include <boost/preprocessor/cat.hpp>

#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// Meta-function which returns a vector type for \p Scalar with \p Size.
///
/// For example,
/// \code
/// make_vector_type<int, 2>::type == int2_
/// make_vector_type<float, 4>::type == float4_
/// \endcode
///
/// \see is_vector_type
template<class Scalar, size_t Size>
struct make_vector_type
{
};

/// \internal_
template<class Scalar>
struct make_vector_type<Scalar, 1>
{
    typedef Scalar type;
};

/// \internal_
#define BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTION(scalar, size) \
    template<> \
    struct make_vector_type<BOOST_PP_CAT(scalar, _), size> \
    { \
        typedef BOOST_PP_CAT(BOOST_PP_CAT(scalar, size), _) type; \
    };

/// \internal_
#define BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(scalar) \
    BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTION(scalar, 2) \
    BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTION(scalar, 4) \
    BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTION(scalar, 8) \
    BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTION(scalar, 16)

BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(char)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(uchar)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(short)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(ushort)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(int)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(uint)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(long)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(ulong)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(float)
BOOST_COMPUTE_DECLARE_MAKE_VECTOR_TYPE_FUNCTIONS(double)

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_TYPE_TRAITS_MAKE_VECTOR_TYPE_HPP
