//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPE_TRAITS_TYPE_NAME_HPP
#define BOOST_COMPUTE_TYPE_TRAITS_TYPE_NAME_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T>
struct type_name_trait;

/// \internal_
#define BOOST_COMPUTE_DEFINE_SCALAR_TYPE_NAME_FUNCTION(type) \
    template<> \
    struct type_name_trait<BOOST_PP_CAT(type, _)> \
    { \
        static const char* value() \
        { \
            return BOOST_PP_STRINGIZE(type); \
        } \
    };

/// \internal_
#define BOOST_COMPUTE_DEFINE_VECTOR_TYPE_NAME_FUNCTION(scalar, n) \
    template<> \
    struct type_name_trait<BOOST_PP_CAT(BOOST_PP_CAT(scalar, n), _)> \
    { \
        static const char* value() \
        { \
            return BOOST_PP_STRINGIZE(BOOST_PP_CAT(scalar, n)); \
        } \
    };

/// \internal_
#define BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(scalar) \
    BOOST_COMPUTE_DEFINE_SCALAR_TYPE_NAME_FUNCTION(scalar) \
    BOOST_COMPUTE_DEFINE_VECTOR_TYPE_NAME_FUNCTION(scalar, 2) \
    BOOST_COMPUTE_DEFINE_VECTOR_TYPE_NAME_FUNCTION(scalar, 4) \
    BOOST_COMPUTE_DEFINE_VECTOR_TYPE_NAME_FUNCTION(scalar, 8) \
    BOOST_COMPUTE_DEFINE_VECTOR_TYPE_NAME_FUNCTION(scalar, 16)

BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(char)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(uchar)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(short)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(ushort)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(int)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(uint)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(long)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(ulong)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(float)
BOOST_COMPUTE_DEFINE_TYPE_NAME_FUNCTIONS(double)

/// \internal_
#define BOOST_COMPUTE_DEFINE_BUILTIN_TYPE_NAME_FUNCTION(type) \
    template<> \
    struct type_name_trait<type> \
    { \
        static const char* value() \
        { \
            return #type; \
        } \
    };

BOOST_COMPUTE_DEFINE_BUILTIN_TYPE_NAME_FUNCTION(bool)
BOOST_COMPUTE_DEFINE_BUILTIN_TYPE_NAME_FUNCTION(char)
BOOST_COMPUTE_DEFINE_BUILTIN_TYPE_NAME_FUNCTION(void)

} // end detail namespace

/// Returns the OpenCL type name for the type \c T as a string.
///
/// \return a string containing the type name for \c T
///
/// For example:
/// \code
/// type_name<float>() == "float"
/// type_name<float4_>() == "float4"
/// \endcode
///
/// \see type_definition<T>()
template<class T>
inline const char* type_name()
{
    return detail::type_name_trait<T>::value();
}

} // end compute namespace
} // end boost namespace

/// Registers the OpenCL type for the C++ \p type to \p name.
///
/// For example, the following will allow Eigen's \c Vector2f type
/// to be used with Boost.Compute algorithms and containers as the
/// built-in \c float2 type.
/// \code
/// BOOST_COMPUTE_TYPE_NAME(Eigen::Vector2f, float2)
/// \endcode
///
/// This macro should be invoked in the global namespace.
///
/// \see type_name()
#define BOOST_COMPUTE_TYPE_NAME(type, name) \
    namespace boost { namespace compute { \
    template<> \
    inline const char* type_name<type>() \
    { \
        return #name; \
    }}}

#endif // BOOST_COMPUTE_TYPE_TRAITS_TYPE_NAME_HPP
