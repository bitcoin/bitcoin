
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_FUNCTIONAL_HASH_DETAIL_FLOAT_FUNCTIONS_HPP)
#define BOOST_FUNCTIONAL_HASH_DETAIL_FLOAT_FUNCTIONS_HPP

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/config/no_tr1/cmath.hpp>

// Set BOOST_HASH_CONFORMANT_FLOATS to 1 for libraries known to have
// sufficiently good floating point support to not require any
// workarounds.
//
// When set to 0, the library tries to automatically
// use the best available implementation. This normally works well, but
// breaks when ambiguities are created by odd namespacing of the functions.
//
// Note that if this is set to 0, the library should still take full
// advantage of the platform's floating point support.

#if defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#elif defined(__LIBCOMO__)
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#elif defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER)
// Rogue Wave library:
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#elif defined(_LIBCPP_VERSION)
// libc++
#   define BOOST_HASH_CONFORMANT_FLOATS 1
#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)
// GNU libstdc++ 3
#   if defined(__GNUC__) && __GNUC__ >= 4
#       define BOOST_HASH_CONFORMANT_FLOATS 1
#   else
#       define BOOST_HASH_CONFORMANT_FLOATS 0
#   endif
#elif defined(__STL_CONFIG_H)
// generic SGI STL
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#elif defined(__MSL_CPP__)
// MSL standard lib:
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#elif defined(__IBMCPP__)
// VACPP std lib (probably conformant for much earlier version).
#   if __IBMCPP__ >= 1210
#       define BOOST_HASH_CONFORMANT_FLOATS 1
#   else
#       define BOOST_HASH_CONFORMANT_FLOATS 0
#   endif
#elif defined(MSIPL_COMPILE_H)
// Modena C++ standard library
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#elif (defined(_YVALS) && !defined(__IBMCPP__)) || defined(_CPPLIB_VER)
// Dinkumware Library (this has to appear after any possible replacement libraries):
#   if _CPPLIB_VER >= 405
#       define BOOST_HASH_CONFORMANT_FLOATS 1
#   else
#       define BOOST_HASH_CONFORMANT_FLOATS 0
#   endif
#else
#   define BOOST_HASH_CONFORMANT_FLOATS 0
#endif

#if BOOST_HASH_CONFORMANT_FLOATS

// The standard library is known to be compliant, so don't use the
// configuration mechanism.

namespace boost {
    namespace hash_detail {
        template <typename Float>
        struct call_ldexp {
            typedef Float float_type;
            inline Float operator()(Float x, int y) const {
                return std::ldexp(x, y);
            }
        };

        template <typename Float>
        struct call_frexp {
            typedef Float float_type;
            inline Float operator()(Float x, int* y) const {
                return std::frexp(x, y);
            }
        };

        template <typename Float>
        struct select_hash_type
        {
            typedef Float type;
        };
    }
}

#else // BOOST_HASH_CONFORMANT_FLOATS == 0

// The C++ standard requires that the C float functions are overloarded
// for float, double and long double in the std namespace, but some of the older
// library implementations don't support this. On some that don't, the C99
// float functions (frexpf, frexpl, etc.) are available.
//
// The following tries to automatically detect which are available.

namespace boost {
    namespace hash_detail {

        // Returned by dummy versions of the float functions.
    
        struct not_found {
            // Implicitly convertible to float and long double in order to avoid
            // a compile error when the dummy float functions are used.

            inline operator float() const { return 0; }
            inline operator long double() const { return 0; }
        };
          
        // A type for detecting the return type of functions.

        template <typename T> struct is;
        template <> struct is<float> { char x[10]; };
        template <> struct is<double> { char x[20]; };
        template <> struct is<long double> { char x[30]; };
        template <> struct is<boost::hash_detail::not_found> { char x[40]; };
            
        // Used to convert the return type of a function to a type for sizeof.

        template <typename T> is<T> float_type(T);

        // call_ldexp
        //
        // This will get specialized for float and long double
        
        template <typename Float> struct call_ldexp
        {
            typedef double float_type;
            
            inline double operator()(double a, int b) const
            {
                using namespace std;
                return ldexp(a, b);
            }
        };

        // call_frexp
        //
        // This will get specialized for float and long double

        template <typename Float> struct call_frexp
        {
            typedef double float_type;
            
            inline double operator()(double a, int* b) const
            {
                using namespace std;
                return frexp(a, b);
            }
        };
    }
}
            
// A namespace for dummy functions to detect when the actual function we want
// isn't available. ldexpl, ldexpf etc. might be added tby the macros below.
//
// AFAICT these have to be outside of the boost namespace, as if they're in
// the boost namespace they'll always be preferable to any other function
// (since the arguments are built in types, ADL can't be used).

namespace boost_hash_detect_float_functions {
    template <class Float> boost::hash_detail::not_found ldexp(Float, int);
    template <class Float> boost::hash_detail::not_found frexp(Float, int*);    
}

// Macros for generating specializations of call_ldexp and call_frexp.
//
// check_cpp and check_c99 check if the C++ or C99 functions are available.
//
// Then the call_* functions select an appropriate implementation.
//
// I used c99_func in a few places just to get a unique name.
//
// Important: when using 'using namespace' at namespace level, include as
// little as possible in that namespace, as Visual C++ has an odd bug which
// can cause the namespace to be imported at the global level. This seems to
// happen mainly when there's a template in the same namesapce.

#define BOOST_HASH_CALL_FLOAT_FUNC(cpp_func, c99_func, type1, type2)    \
namespace boost_hash_detect_float_functions {                           \
    template <class Float>                                              \
    boost::hash_detail::not_found c99_func(Float, type2);               \
}                                                                       \
                                                                        \
namespace boost {                                                       \
    namespace hash_detail {                                             \
        namespace c99_func##_detect {                                   \
            using namespace std;                                        \
            using namespace boost_hash_detect_float_functions;          \
                                                                        \
            struct check {                                              \
                static type1 x;                                         \
                static type2 y;                                         \
                BOOST_STATIC_CONSTANT(bool, cpp =                       \
                    sizeof(float_type(cpp_func(x,y)))                   \
                        == sizeof(is<type1>));                          \
                BOOST_STATIC_CONSTANT(bool, c99 =                       \
                    sizeof(float_type(c99_func(x,y)))                   \
                        == sizeof(is<type1>));                          \
            };                                                          \
        }                                                               \
                                                                        \
        template <bool x>                                               \
        struct call_c99_##c99_func :                                    \
            boost::hash_detail::call_##cpp_func<double> {};             \
                                                                        \
        template <>                                                     \
        struct call_c99_##c99_func<true> {                              \
            typedef type1 float_type;                                   \
                                                                        \
            template <typename T>                                       \
            inline type1 operator()(type1 a, T b)  const                \
            {                                                           \
                using namespace std;                                    \
                return c99_func(a, b);                                  \
            }                                                           \
        };                                                              \
                                                                        \
        template <bool x>                                               \
        struct call_cpp_##c99_func :                                    \
            call_c99_##c99_func<                                        \
                ::boost::hash_detail::c99_func##_detect::check::c99     \
            > {};                                                       \
                                                                        \
        template <>                                                     \
        struct call_cpp_##c99_func<true> {                              \
            typedef type1 float_type;                                   \
                                                                        \
            template <typename T>                                       \
            inline type1 operator()(type1 a, T b)  const                \
            {                                                           \
                using namespace std;                                    \
                return cpp_func(a, b);                                  \
            }                                                           \
        };                                                              \
                                                                        \
        template <>                                                     \
        struct call_##cpp_func<type1> :                                 \
            call_cpp_##c99_func<                                        \
                ::boost::hash_detail::c99_func##_detect::check::cpp     \
            > {};                                                       \
    }                                                                   \
}

#define BOOST_HASH_CALL_FLOAT_MACRO(cpp_func, c99_func, type1, type2)   \
namespace boost {                                                       \
    namespace hash_detail {                                             \
                                                                        \
        template <>                                                     \
        struct call_##cpp_func<type1> {                                 \
            typedef type1 float_type;                                   \
            inline type1 operator()(type1 x, type2 y) const {           \
                return c99_func(x, y);                                  \
            }                                                           \
        };                                                              \
    }                                                                   \
}

#if defined(ldexpf)
BOOST_HASH_CALL_FLOAT_MACRO(ldexp, ldexpf, float, int)
#else
BOOST_HASH_CALL_FLOAT_FUNC(ldexp, ldexpf, float, int)
#endif

#if defined(ldexpl)
BOOST_HASH_CALL_FLOAT_MACRO(ldexp, ldexpl, long double, int)
#else
BOOST_HASH_CALL_FLOAT_FUNC(ldexp, ldexpl, long double, int)
#endif

#if defined(frexpf)
BOOST_HASH_CALL_FLOAT_MACRO(frexp, frexpf, float, int*)
#else
BOOST_HASH_CALL_FLOAT_FUNC(frexp, frexpf, float, int*)
#endif

#if defined(frexpl)
BOOST_HASH_CALL_FLOAT_MACRO(frexp, frexpl, long double, int*)
#else
BOOST_HASH_CALL_FLOAT_FUNC(frexp, frexpl, long double, int*)
#endif

#undef BOOST_HASH_CALL_FLOAT_MACRO
#undef BOOST_HASH_CALL_FLOAT_FUNC


namespace boost
{
    namespace hash_detail
    {
        template <typename Float1, typename Float2>
        struct select_hash_type_impl {
            typedef double type;
        };

        template <>
        struct select_hash_type_impl<float, float> {
            typedef float type;
        };

        template <>
        struct select_hash_type_impl<long double, long double> {
            typedef long double type;
        };


        // select_hash_type
        //
        // If there is support for a particular floating point type, use that
        // otherwise use double (there's always support for double).
             
        template <typename Float>
        struct select_hash_type : select_hash_type_impl<
                BOOST_DEDUCED_TYPENAME call_ldexp<Float>::float_type,
                BOOST_DEDUCED_TYPENAME call_frexp<Float>::float_type
            > {};            
    }
}

#endif // BOOST_HASH_CONFORMANT_FLOATS

#endif
