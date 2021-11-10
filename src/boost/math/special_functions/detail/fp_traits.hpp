// fp_traits.hpp

#ifndef BOOST_MATH_FP_TRAITS_HPP
#define BOOST_MATH_FP_TRAITS_HPP

// Copyright (c) 2006 Johan Rade

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
To support old compilers, care has been taken to avoid partial template
specialization and meta function forwarding.
With these techniques, the code could be simplified.
*/

#if defined(__vms) && defined(__DECCXX) && !__IEEE_FLOAT
// The VAX floating point formats are used (for float and double)
#   define BOOST_FPCLASSIFY_VAX_FORMAT
#endif

#include <cstring>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <boost/math/tools/is_standalone.hpp>
#include <boost/math/tools/assert.hpp>

// Determine endinaness
#ifndef BOOST_MATH_STANDALONE

#include <boost/predef/other/endian.h>
#define BOOST_MATH_ENDIAN_BIG_BYTE BOOST_ENDIAN_BIG_BYTE
#define BOOST_MATH_ENDIAN_LITTLE_BYTE BOOST_ENDIAN_LITTLE_BYTE

#elif (__cplusplus > 202000L || _MSVC_LANG > 202000L)

#if __has_include(<bit>)
#include <bit>
#define BOOST_MATH_ENDIAN_BIG_BYTE (std::endian::native == std::endian::big)
#define BOOST_MATH_ENDIAN_LITTLE_BYTE (std::endian::native == std::endian::little)
#endif

#elif defined(_WIN32)

#define BOOST_MATH_ENDIAN_BIG_BYTE 1
#define BOOST_MATH_ENDIAN_LITTLE_BYTE 0

#elif defined(__BYTE_ORDER__)

#define BOOST_MATH_ENDIAN_BIG_BYTE (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BOOST_MATH_ENDIAN_LITTLE_BYTE (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#else 
#error Could not determine endian type. Please disable standalone mode, and file an issue at https://github.com/boostorg/math
#endif // Determine endinaness

#ifdef BOOST_NO_STDC_NAMESPACE
  namespace std{ using ::memcpy; }
#endif

#ifndef FP_NORMAL

#define FP_ZERO        0
#define FP_NORMAL      1
#define FP_INFINITE    2
#define FP_NAN         3
#define FP_SUBNORMAL   4

#else

#define BOOST_HAS_FPCLASSIFY

#ifndef fpclassify
#  if (defined(__GLIBCPP__) || defined(__GLIBCXX__)) \
         && defined(_GLIBCXX_USE_C99_MATH) \
         && !(defined(_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC) \
         && (_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC != 0))
#     ifdef _STLP_VENDOR_CSTD
#        if _STLPORT_VERSION >= 0x520
#           define BOOST_FPCLASSIFY_PREFIX ::__std_alias::
#        else
#           define BOOST_FPCLASSIFY_PREFIX ::_STLP_VENDOR_CSTD::
#        endif
#     else
#        define BOOST_FPCLASSIFY_PREFIX ::std::
#     endif
#  else
#     undef BOOST_HAS_FPCLASSIFY
#     define BOOST_FPCLASSIFY_PREFIX
#  endif
#elif (defined(__HP_aCC) && !defined(__hppa))
// aCC 6 appears to do "#define fpclassify fpclassify" which messes us up a bit!
#  define BOOST_FPCLASSIFY_PREFIX ::
#else
#  define BOOST_FPCLASSIFY_PREFIX
#endif

#ifdef __MINGW32__
#  undef BOOST_HAS_FPCLASSIFY
#endif

#endif


//------------------------------------------------------------------------------

namespace boost {
namespace math {
namespace detail {

//------------------------------------------------------------------------------

/*
The following classes are used to tag the different methods that are used
for floating point classification
*/

struct native_tag {};
template <bool has_limits>
struct generic_tag {};
struct ieee_tag {};
struct ieee_copy_all_bits_tag : public ieee_tag {};
struct ieee_copy_leading_bits_tag : public ieee_tag {};

#ifdef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
//
// These helper functions are used only when numeric_limits<>
// members are not compile time constants:
//
inline bool is_generic_tag_false(const generic_tag<false>*)
{
   return true;
}
inline bool is_generic_tag_false(const void*)
{
   return false;
}
#endif

//------------------------------------------------------------------------------

/*
Most processors support three different floating point precisions:
single precision (32 bits), double precision (64 bits)
and extended double precision (80 - 128 bits, depending on the processor)

Note that the C++ type long double can be implemented
both as double precision and extended double precision.
*/

struct unknown_precision{};
struct single_precision {};
struct double_precision {};
struct extended_double_precision {};

// native_tag version --------------------------------------------------------------

template<class T> struct fp_traits_native
{
    typedef native_tag method;
};

// generic_tag version -------------------------------------------------------------

template<class T, class U> struct fp_traits_non_native
{
#ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   typedef generic_tag<std::numeric_limits<T>::is_specialized> method;
#else
   typedef generic_tag<false> method;
#endif
};

// ieee_tag versions ---------------------------------------------------------------

/*
These specializations of fp_traits_non_native contain information needed
to "parse" the binary representation of a floating point number.

Typedef members:

  bits -- the target type when copying the leading bytes of a floating
      point number. It is a typedef for uint32_t or uint64_t.

  method -- tells us whether all bytes are copied or not.
      It is a typedef for ieee_copy_all_bits_tag or ieee_copy_leading_bits_tag.

Static data members:

  sign, exponent, flag, significand -- bit masks that give the meaning of the
  bits in the leading bytes.

Static function members:

  get_bits(), set_bits() -- provide access to the leading bytes.

*/

// ieee_tag version, float (32 bits) -----------------------------------------------

#ifndef BOOST_FPCLASSIFY_VAX_FORMAT

template<> struct fp_traits_non_native<float, single_precision>
{
    typedef ieee_copy_all_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7f800000;
    static constexpr uint32_t flag        = 0x00000000;
    static constexpr uint32_t significand = 0x007fffff;

    typedef uint32_t bits;
    static void get_bits(float x, uint32_t& a) { std::memcpy(&a, &x, 4); }
    static void set_bits(float& x, uint32_t a) { std::memcpy(&x, &a, 4); }
};

// ieee_tag version, double (64 bits) ----------------------------------------------

#if defined(BOOST_NO_INT64_T) || defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION) \
   || defined(BOOST_BORLANDC) || defined(__CODEGEAR__)

template<> struct fp_traits_non_native<double, double_precision>
{
    typedef ieee_copy_leading_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7ff00000;
    static constexpr uint32_t flag        = 0;
    static constexpr uint32_t significand = 0x000fffff;

    typedef uint32_t bits;

    static void get_bits(double x, uint32_t& a)
    {
        std::memcpy(&a, reinterpret_cast<const unsigned char*>(&x) + offset_, 4);
    }

    static void set_bits(double& x, uint32_t a)
    {
        std::memcpy(reinterpret_cast<unsigned char*>(&x) + offset_, &a, 4);
    }

private:

#if BOOST_MATH_ENDIAN_BIG_BYTE
    static constexpr int offset_ = 0;
#elif BOOST_MATH_ENDIAN_LITTLE_BYTE
    static constexpr int offset_ = 4;
#else
    static_assert(sizeof(double_precision) == 0, "Endian type could not be identified");
#endif
};

//..............................................................................

#else

template<> struct fp_traits_non_native<double, double_precision>
{
    typedef ieee_copy_all_bits_tag method;

    static constexpr uint64_t sign     = ((uint64_t)0x80000000u) << 32;
    static constexpr uint64_t exponent = ((uint64_t)0x7ff00000) << 32;
    static constexpr uint64_t flag     = 0;
    static constexpr uint64_t significand
        = (((uint64_t)0x000fffff) << 32) + ((uint64_t)0xffffffffu);

    typedef uint64_t bits;
    static void get_bits(double x, uint64_t& a) { std::memcpy(&a, &x, 8); }
    static void set_bits(double& x, uint64_t a) { std::memcpy(&x, &a, 8); }
};

#endif

#endif  // #ifndef BOOST_FPCLASSIFY_VAX_FORMAT

// long double (64 bits) -------------------------------------------------------

#if defined(BOOST_NO_INT64_T) || defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION)\
   || defined(BOOST_BORLANDC) || defined(__CODEGEAR__)

template<> struct fp_traits_non_native<long double, double_precision>
{
    typedef ieee_copy_leading_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7ff00000;
    static constexpr uint32_t flag        = 0;
    static constexpr uint32_t significand = 0x000fffff;

    typedef uint32_t bits;

    static void get_bits(long double x, uint32_t& a)
    {
        std::memcpy(&a, reinterpret_cast<const unsigned char*>(&x) + offset_, 4);
    }

    static void set_bits(long double& x, uint32_t a)
    {
        std::memcpy(reinterpret_cast<unsigned char*>(&x) + offset_, &a, 4);
    }

private:

#if BOOST_MATH_ENDIAN_BIG_BYTE
    static constexpr int offset_ = 0;
#elif BOOST_MATH_ENDIAN_LITTLE_BYTE
    static constexpr int offset_ = 4;
#else
    static_assert(sizeof(double_precision) == 0, "Endian type could not be identified");
#endif
};

//..............................................................................

#else

template<> struct fp_traits_non_native<long double, double_precision>
{
    typedef ieee_copy_all_bits_tag method;

    static const uint64_t sign     = (uint64_t)0x80000000u << 32;
    static const uint64_t exponent = (uint64_t)0x7ff00000 << 32;
    static const uint64_t flag     = 0;
    static const uint64_t significand
        = ((uint64_t)0x000fffff << 32) + (uint64_t)0xffffffffu;

    typedef uint64_t bits;
    static void get_bits(long double x, uint64_t& a) { std::memcpy(&a, &x, 8); }
    static void set_bits(long double& x, uint64_t a) { std::memcpy(&x, &a, 8); }
};

#endif


// long double (>64 bits), x86 and x64 -----------------------------------------

#if defined(__i386) || defined(__i386__) || defined(_M_IX86) \
    || defined(__amd64) || defined(__amd64__)  || defined(_M_AMD64) \
    || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)

// Intel extended double precision format (80 bits)

template<>
struct fp_traits_non_native<long double, extended_double_precision>
{
    typedef ieee_copy_leading_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7fff0000;
    static constexpr uint32_t flag        = 0x00008000;
    static constexpr uint32_t significand = 0x00007fff;

    typedef uint32_t bits;

    static void get_bits(long double x, uint32_t& a)
    {
        std::memcpy(&a, reinterpret_cast<const unsigned char*>(&x) + 6, 4);
    }

    static void set_bits(long double& x, uint32_t a)
    {
        std::memcpy(reinterpret_cast<unsigned char*>(&x) + 6, &a, 4);
    }
};


// long double (>64 bits), Itanium ---------------------------------------------

#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)

// The floating point format is unknown at compile time
// No template specialization is provided.
// The generic_tag definition is used.

// The Itanium supports both
// the Intel extended double precision format (80 bits) and
// the IEEE extended double precision format with 15 exponent bits (128 bits).

#elif defined(__GNUC__) && (LDBL_MANT_DIG == 106)

//
// Define nothing here and fall though to generic_tag:
// We have GCC's "double double" in effect, and any attempt
// to handle it via bit-fiddling is pretty much doomed to fail...
//

// long double (>64 bits), PowerPC ---------------------------------------------

#elif defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) \
    || defined(__ppc) || defined(__ppc__) || defined(__PPC__)

// PowerPC extended double precision format (128 bits)

template<>
struct fp_traits_non_native<long double, extended_double_precision>
{
    typedef ieee_copy_leading_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7ff00000;
    static constexpr uint32_t flag        = 0x00000000;
    static constexpr uint32_t significand = 0x000fffff;

    typedef uint32_t bits;

    static void get_bits(long double x, uint32_t& a)
    {
        std::memcpy(&a, reinterpret_cast<const unsigned char*>(&x) + offset_, 4);
    }

    static void set_bits(long double& x, uint32_t a)
    {
        std::memcpy(reinterpret_cast<unsigned char*>(&x) + offset_, &a, 4);
    }

private:

#if BOOST_MATH_ENDIAN_BIG_BYTE
    static constexpr int offset_ = 0;
#elif BOOST_MATH_ENDIAN_LITTLE_BYTE
    static constexpr int offset_ = 12;
#else
    static_assert(sizeof(extended_double_precision) == 0, "Endian type could not be identified");
#endif
};


// long double (>64 bits), Motorola 68K ----------------------------------------

#elif defined(__m68k) || defined(__m68k__) \
    || defined(__mc68000) || defined(__mc68000__) \

// Motorola extended double precision format (96 bits)

// It is the same format as the Intel extended double precision format,
// except that 1) it is big-endian, 2) the 3rd and 4th byte are padding, and
// 3) the flag bit is not set for infinity

template<>
struct fp_traits_non_native<long double, extended_double_precision>
{
    typedef ieee_copy_leading_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7fff0000;
    static constexpr uint32_t flag        = 0x00008000;
    static constexpr uint32_t significand = 0x00007fff;

    // copy 1st, 2nd, 5th and 6th byte. 3rd and 4th byte are padding.

    typedef uint32_t bits;

    static void get_bits(long double x, uint32_t& a)
    {
        std::memcpy(&a, &x, 2);
        std::memcpy(reinterpret_cast<unsigned char*>(&a) + 2,
               reinterpret_cast<const unsigned char*>(&x) + 4, 2);
    }

    static void set_bits(long double& x, uint32_t a)
    {
        std::memcpy(&x, &a, 2);
        std::memcpy(reinterpret_cast<unsigned char*>(&x) + 4,
               reinterpret_cast<const unsigned char*>(&a) + 2, 2);
    }
};


// long double (>64 bits), All other processors --------------------------------

#else

// IEEE extended double precision format with 15 exponent bits (128 bits)

template<>
struct fp_traits_non_native<long double, extended_double_precision>
{
    typedef ieee_copy_leading_bits_tag method;

    static constexpr uint32_t sign        = 0x80000000u;
    static constexpr uint32_t exponent    = 0x7fff0000;
    static constexpr uint32_t flag        = 0x00000000;
    static constexpr uint32_t significand = 0x0000ffff;

    typedef uint32_t bits;

    static void get_bits(long double x, uint32_t& a)
    {
        std::memcpy(&a, reinterpret_cast<const unsigned char*>(&x) + offset_, 4);
    }

    static void set_bits(long double& x, uint32_t a)
    {
        std::memcpy(reinterpret_cast<unsigned char*>(&x) + offset_, &a, 4);
    }

private:

#if BOOST_MATH_ENDIAN_BIG_BYTE
    static constexpr int offset_ = 0;
#elif BOOST_MATH_ENDIAN_LITTLE_BYTE
    static constexpr int offset_ = 12;
#else
    static_assert(sizeof(extended_double_precision) == 0, "Endian type could not be identified");
#endif
};

#endif

//------------------------------------------------------------------------------

// size_to_precision is a type switch for converting a C++ floating point type
// to the corresponding precision type.

template<int n, bool fp> struct size_to_precision
{
   typedef unknown_precision type;
};

template<> struct size_to_precision<4, true>
{
    typedef single_precision type;
};

template<> struct size_to_precision<8, true>
{
    typedef double_precision type;
};

template<> struct size_to_precision<10, true>
{
    typedef extended_double_precision type;
};

template<> struct size_to_precision<12, true>
{
    typedef extended_double_precision type;
};

template<> struct size_to_precision<16, true>
{
    typedef extended_double_precision type;
};

//------------------------------------------------------------------------------
//
// Figure out whether to use native classification functions based on
// whether T is a built in floating point type or not:
//
template <class T>
struct select_native
{
    typedef typename size_to_precision<sizeof(T), ::std::is_floating_point<T>::value>::type precision;
    typedef fp_traits_non_native<T, precision> type;
};
template<>
struct select_native<float>
{
    typedef fp_traits_native<float> type;
};
template<>
struct select_native<double>
{
    typedef fp_traits_native<double> type;
};
template<>
struct select_native<long double>
{
    typedef fp_traits_native<long double> type;
};

//------------------------------------------------------------------------------

// fp_traits is a type switch that selects the right fp_traits_non_native

#if (defined(BOOST_MATH_USE_C99) && !(defined(__GNUC__) && (__GNUC__ < 4))) \
   && !defined(__hpux) \
   && !defined(__DECCXX)\
   && !defined(__osf__) \
   && !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)\
   && !defined(__FAST_MATH__)\
   && !defined(BOOST_MATH_DISABLE_STD_FPCLASSIFY)\
   && !defined(__INTEL_COMPILER)\
   && !defined(sun)\
   && !defined(__VXWORKS__)
#  define BOOST_MATH_USE_STD_FPCLASSIFY
#endif

template<class T> struct fp_traits
{
    typedef typename size_to_precision<sizeof(T), ::std::is_floating_point<T>::value>::type precision;
#if defined(BOOST_MATH_USE_STD_FPCLASSIFY) && !defined(BOOST_MATH_DISABLE_STD_FPCLASSIFY)
    typedef typename select_native<T>::type type;
#else
    typedef fp_traits_non_native<T, precision> type;
#endif
    typedef fp_traits_non_native<T, precision> sign_change_type;
};

//------------------------------------------------------------------------------

}   // namespace detail
}   // namespace math
}   // namespace boost

#endif
