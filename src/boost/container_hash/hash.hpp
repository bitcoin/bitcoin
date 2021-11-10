
// Copyright 2005-2014 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Based on Peter Dimov's proposal
//  http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1756.pdf
//  issue 6.18.
//
//  This also contains public domain code from MurmurHash. From the
//  MurmurHash header:

// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#if !defined(BOOST_FUNCTIONAL_HASH_HASH_HPP)
#define BOOST_FUNCTIONAL_HASH_HASH_HPP

#include <boost/container_hash/hash_fwd.hpp>
#include <functional>
#include <iterator>
#include <boost/container_hash/detail/hash_float.hpp>
#include <string>
#include <boost/limits.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/cstdint.hpp>

#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
#include <boost/type_traits/is_pointer.hpp>
#endif

#if !defined(BOOST_NO_CXX11_HDR_TYPEINDEX)
#include <typeindex>
#endif

#if !defined(BOOST_NO_CXX11_HDR_SYSTEM_ERROR)
#include <system_error>
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)

#if BOOST_MSVC >= 1400
#pragma warning(disable:6295) // Ill-defined for-loop : 'unsigned int' values
                              // are always of range '0' to '4294967295'.
                              // Loop executes infinitely.
#endif

#endif

#if BOOST_WORKAROUND(__GNUC__, < 3) \
    && !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)
#define BOOST_HASH_CHAR_TRAITS string_char_traits
#else
#define BOOST_HASH_CHAR_TRAITS char_traits
#endif

#if defined(_MSC_VER)
#   define BOOST_FUNCTIONAL_HASH_ROTL32(x, r) _rotl(x,r)
#else
#   define BOOST_FUNCTIONAL_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif

// Detect whether standard library has C++17 headers

#if !defined(BOOST_HASH_CXX17)
#   if defined(BOOST_MSVC)
#       if defined(_HAS_CXX17) && _HAS_CXX17
#           define BOOST_HASH_CXX17 1
#       endif
#   elif defined(__cplusplus) && __cplusplus >= 201703
#       define BOOST_HASH_CXX17 1
#   endif
#endif

#if !defined(BOOST_HASH_CXX17)
#   define BOOST_HASH_CXX17 0
#endif

#if BOOST_HASH_CXX17 && defined(__has_include)
#   if !defined(BOOST_HASH_HAS_STRING_VIEW) && __has_include(<string_view>)
#       define BOOST_HASH_HAS_STRING_VIEW 1
#   endif
#   if !defined(BOOST_HASH_HAS_OPTIONAL) && __has_include(<optional>)
#       define BOOST_HASH_HAS_OPTIONAL 1
#   endif
#   if !defined(BOOST_HASH_HAS_VARIANT) && __has_include(<variant>)
#       define BOOST_HASH_HAS_VARIANT 1
#   endif
#endif

#if !defined(BOOST_HASH_HAS_STRING_VIEW)
#   define BOOST_HASH_HAS_STRING_VIEW 0
#endif

#if !defined(BOOST_HASH_HAS_OPTIONAL)
#   define BOOST_HASH_HAS_OPTIONAL 0
#endif

#if !defined(BOOST_HASH_HAS_VARIANT)
#   define BOOST_HASH_HAS_VARIANT 0
#endif

#if BOOST_HASH_HAS_STRING_VIEW
#   include <string_view>
#endif

#if BOOST_HASH_HAS_OPTIONAL
#   include <optional>
#endif

#if BOOST_HASH_HAS_VARIANT
#   include <variant>
#endif

namespace boost
{
    namespace hash_detail
    {
#if defined(BOOST_NO_CXX98_FUNCTION_BASE)
        template <typename T>
        struct hash_base
        {
            typedef T argument_type;
            typedef std::size_t result_type;
        };
#else
        template <typename T>
        struct hash_base : std::unary_function<T, std::size_t> {};
#endif

        struct enable_hash_value { typedef std::size_t type; };

        template <typename T> struct basic_numbers {};
        template <typename T> struct long_numbers;
        template <typename T> struct ulong_numbers;
        template <typename T> struct float_numbers {};

        template <> struct basic_numbers<bool> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<char> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<unsigned char> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<signed char> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<short> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<unsigned short> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<int> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<unsigned int> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<long> :
            boost::hash_detail::enable_hash_value {};
        template <> struct basic_numbers<unsigned long> :
            boost::hash_detail::enable_hash_value {};

#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
        template <> struct basic_numbers<wchar_t> :
            boost::hash_detail::enable_hash_value {};
#endif

#if !defined(BOOST_NO_CXX11_CHAR16_T)
        template <> struct basic_numbers<char16_t> :
            boost::hash_detail::enable_hash_value {};
#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T)
        template <> struct basic_numbers<char32_t> :
            boost::hash_detail::enable_hash_value {};
#endif

        // long_numbers is defined like this to allow for separate
        // specialization for long_long and int128_type, in case
        // they conflict.
        template <typename T> struct long_numbers2 {};
        template <typename T> struct ulong_numbers2 {};
        template <typename T> struct long_numbers : long_numbers2<T> {};
        template <typename T> struct ulong_numbers : ulong_numbers2<T> {};

#if !defined(BOOST_NO_LONG_LONG)
        template <> struct long_numbers<boost::long_long_type> :
            boost::hash_detail::enable_hash_value {};
        template <> struct ulong_numbers<boost::ulong_long_type> :
            boost::hash_detail::enable_hash_value {};
#endif

#if defined(BOOST_HAS_INT128)
        template <> struct long_numbers2<boost::int128_type> :
            boost::hash_detail::enable_hash_value {};
        template <> struct ulong_numbers2<boost::uint128_type> :
            boost::hash_detail::enable_hash_value {};
#endif

        template <> struct float_numbers<float> :
            boost::hash_detail::enable_hash_value {};
        template <> struct float_numbers<double> :
            boost::hash_detail::enable_hash_value {};
        template <> struct float_numbers<long double> :
            boost::hash_detail::enable_hash_value {};
    }

    template <typename T>
    typename boost::hash_detail::basic_numbers<T>::type hash_value(T);
    template <typename T>
    typename boost::hash_detail::long_numbers<T>::type hash_value(T);
    template <typename T>
    typename boost::hash_detail::ulong_numbers<T>::type hash_value(T);

    template <typename T>
    typename boost::enable_if<boost::is_enum<T>, std::size_t>::type
        hash_value(T);

#if !BOOST_WORKAROUND(__DMC__, <= 0x848)
    template <class T> std::size_t hash_value(T* const&);
#else
    template <class T> std::size_t hash_value(T*);
#endif

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
    template< class T, unsigned N >
    std::size_t hash_value(const T (&x)[N]);

    template< class T, unsigned N >
    std::size_t hash_value(T (&x)[N]);
#endif

    template <class Ch, class A>
    std::size_t hash_value(
        std::basic_string<Ch, std::BOOST_HASH_CHAR_TRAITS<Ch>, A> const&);

#if BOOST_HASH_HAS_STRING_VIEW
    template <class Ch>
    std::size_t hash_value(
        std::basic_string_view<Ch, std::BOOST_HASH_CHAR_TRAITS<Ch> > const&);
#endif

    template <typename T>
    typename boost::hash_detail::float_numbers<T>::type hash_value(T);

#if BOOST_HASH_HAS_OPTIONAL
    template <typename T>
    std::size_t hash_value(std::optional<T> const&);
#endif

#if BOOST_HASH_HAS_VARIANT
    std::size_t hash_value(std::monostate);
    template <typename... Types>
    std::size_t hash_value(std::variant<Types...> const&);
#endif

#if !defined(BOOST_NO_CXX11_HDR_TYPEINDEX)
    std::size_t hash_value(std::type_index);
#endif

#if !defined(BOOST_NO_CXX11_HDR_SYSTEM_ERROR)
    std::size_t hash_value(std::error_code const&);
    std::size_t hash_value(std::error_condition const&);
#endif

    // Implementation

    namespace hash_detail
    {
        template <class T>
        inline std::size_t hash_value_signed(T val)
        {
             const unsigned int size_t_bits = std::numeric_limits<std::size_t>::digits;
             // ceiling(std::numeric_limits<T>::digits / size_t_bits) - 1
             const int length = (std::numeric_limits<T>::digits - 1)
                 / static_cast<int>(size_t_bits);

             std::size_t seed = 0;
             T positive = val < 0 ? -1 - val : val;

             // Hopefully, this loop can be unrolled.
             for(unsigned int i = length * size_t_bits; i > 0; i -= size_t_bits)
             {
                 seed ^= (std::size_t) (positive >> i) + (seed<<6) + (seed>>2);
             }
             seed ^= (std::size_t) val + (seed<<6) + (seed>>2);

             return seed;
        }

        template <class T>
        inline std::size_t hash_value_unsigned(T val)
        {
             const unsigned int size_t_bits = std::numeric_limits<std::size_t>::digits;
             // ceiling(std::numeric_limits<T>::digits / size_t_bits) - 1
             const int length = (std::numeric_limits<T>::digits - 1)
                 / static_cast<int>(size_t_bits);

             std::size_t seed = 0;

             // Hopefully, this loop can be unrolled.
             for(unsigned int i = length * size_t_bits; i > 0; i -= size_t_bits)
             {
                 seed ^= (std::size_t) (val >> i) + (seed<<6) + (seed>>2);
             }
             seed ^= (std::size_t) val + (seed<<6) + (seed>>2);

             return seed;
        }

        template <typename SizeT>
        inline void hash_combine_impl(SizeT& seed, SizeT value)
        {
            seed ^= value + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        inline void hash_combine_impl(boost::uint32_t& h1,
                boost::uint32_t k1)
        {
            const uint32_t c1 = 0xcc9e2d51;
            const uint32_t c2 = 0x1b873593;

            k1 *= c1;
            k1 = BOOST_FUNCTIONAL_HASH_ROTL32(k1,15);
            k1 *= c2;

            h1 ^= k1;
            h1 = BOOST_FUNCTIONAL_HASH_ROTL32(h1,13);
            h1 = h1*5+0xe6546b64;
        }


// Don't define 64-bit hash combine on platforms without 64 bit integers,
// and also not for 32-bit gcc as it warns about the 64-bit constant.
#if !defined(BOOST_NO_INT64_T) && \
        !(defined(__GNUC__) && ULONG_MAX == 0xffffffff)

        inline void hash_combine_impl(boost::uint64_t& h,
                boost::uint64_t k)
        {
            const boost::uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
            const int r = 47;

            k *= m;
            k ^= k >> r;
            k *= m;

            h ^= k;
            h *= m;

            // Completely arbitrary number, to prevent 0's
            // from hashing to 0.
            h += 0xe6546b64;
        }

#endif // BOOST_NO_INT64_T
    }

    template <typename T>
    typename boost::hash_detail::basic_numbers<T>::type hash_value(T v)
    {
        return static_cast<std::size_t>(v);
    }

    template <typename T>
    typename boost::hash_detail::long_numbers<T>::type hash_value(T v)
    {
        return hash_detail::hash_value_signed(v);
    }

    template <typename T>
    typename boost::hash_detail::ulong_numbers<T>::type hash_value(T v)
    {
        return hash_detail::hash_value_unsigned(v);
    }

    template <typename T>
    typename boost::enable_if<boost::is_enum<T>, std::size_t>::type
        hash_value(T v)
    {
        return static_cast<std::size_t>(v);
    }

    // Implementation by Alberto Barbati and Dave Harris.
#if !BOOST_WORKAROUND(__DMC__, <= 0x848)
    template <class T> std::size_t hash_value(T* const& v)
#else
    template <class T> std::size_t hash_value(T* v)
#endif
    {
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
    // for some reason ptrdiff_t on OpenVMS compiler with
    // 64 bit is not 64 bit !!!
        std::size_t x = static_cast<std::size_t>(
           reinterpret_cast<long long int>(v));
#else
        std::size_t x = static_cast<std::size_t>(
           reinterpret_cast<std::ptrdiff_t>(v));
#endif
        return x + (x >> 3);
    }

#if defined(BOOST_MSVC)
#pragma warning(push)
#if BOOST_MSVC <= 1400
#pragma warning(disable:4267) // 'argument' : conversion from 'size_t' to
                              // 'unsigned int', possible loss of data
                              // A misguided attempt to detect 64-bit
                              // incompatability.
#endif
#endif

    template <class T>
    inline void hash_combine(std::size_t& seed, T const& v)
    {
        boost::hash<T> hasher;
        return boost::hash_detail::hash_combine_impl(seed, hasher(v));
    }

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

    template <class It>
    inline std::size_t hash_range(It first, It last)
    {
        std::size_t seed = 0;

        for(; first != last; ++first)
        {
            hash_combine<typename std::iterator_traits<It>::value_type>(seed, *first);
        }

        return seed;
    }

    template <class It>
    inline void hash_range(std::size_t& seed, It first, It last)
    {
        for(; first != last; ++first)
        {
            hash_combine<typename std::iterator_traits<It>::value_type>(seed, *first);
        }
    }

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
    template <class T>
    inline std::size_t hash_range(T* first, T* last)
    {
        std::size_t seed = 0;

        for(; first != last; ++first)
        {
            boost::hash<T> hasher;
            seed ^= hasher(*first) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        return seed;
    }

    template <class T>
    inline void hash_range(std::size_t& seed, T* first, T* last)
    {
        for(; first != last; ++first)
        {
            boost::hash<T> hasher;
            seed ^= hasher(*first) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }
    }
#endif

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
    template< class T, unsigned N >
    inline std::size_t hash_value(const T (&x)[N])
    {
        return hash_range(x, x + N);
    }

    template< class T, unsigned N >
    inline std::size_t hash_value(T (&x)[N])
    {
        return hash_range(x, x + N);
    }
#endif

    template <class Ch, class A>
    inline std::size_t hash_value(
        std::basic_string<Ch, std::BOOST_HASH_CHAR_TRAITS<Ch>, A> const& v)
    {
        return hash_range(v.begin(), v.end());
    }

#if BOOST_HASH_HAS_STRING_VIEW
    template <class Ch>
    inline std::size_t hash_value(
        std::basic_string_view<Ch, std::BOOST_HASH_CHAR_TRAITS<Ch> > const& v)
    {
        return hash_range(v.begin(), v.end());
    }
#endif

    template <typename T>
    typename boost::hash_detail::float_numbers<T>::type hash_value(T v)
    {
        return boost::hash_detail::float_hash_value(v);
    }

#if BOOST_HASH_HAS_OPTIONAL
    template <typename T>
    inline std::size_t hash_value(std::optional<T> const& v) {
        if (!v) {
            // Arbitray value for empty optional.
            return 0x12345678;
        } else {
            boost::hash<T> hf;
            return hf(*v);
        }
    }
#endif

#if BOOST_HASH_HAS_VARIANT
    inline std::size_t hash_value(std::monostate) {
        return 0x87654321;
    }

    template <typename... Types>
    inline std::size_t hash_value(std::variant<Types...> const& v) {
        std::size_t seed = 0;
        hash_combine(seed, v.index());
        std::visit([&seed](auto&& x) { hash_combine(seed, x); }, v);
        return seed;
    }
#endif


#if !defined(BOOST_NO_CXX11_HDR_TYPEINDEX)
    inline std::size_t hash_value(std::type_index v)
    {
        return v.hash_code();
    }
#endif

#if !defined(BOOST_NO_CXX11_HDR_SYSTEM_ERROR)
    inline std::size_t hash_value(std::error_code const& v) {
        std::size_t seed = 0;
        hash_combine(seed, v.value());
        hash_combine(seed, &v.category());
        return seed;
    }

    inline std::size_t hash_value(std::error_condition const& v) {
        std::size_t seed = 0;
        hash_combine(seed, v.value());
        hash_combine(seed, &v.category());
        return seed;
    }
#endif

    //
    // boost::hash
    //

    // Define the specializations required by the standard. The general purpose
    // boost::hash is defined later in extensions.hpp if
    // BOOST_HASH_NO_EXTENSIONS is not defined.

    // BOOST_HASH_SPECIALIZE - define a specialization for a type which is
    // passed by copy.
    //
    // BOOST_HASH_SPECIALIZE_REF - define a specialization for a type which is
    // passed by const reference.
    //
    // These are undefined later.

#define BOOST_HASH_SPECIALIZE(type) \
    template <> struct hash<type> \
         : public boost::hash_detail::hash_base<type> \
    { \
        std::size_t operator()(type v) const \
        { \
            return boost::hash_value(v); \
        } \
    };

#define BOOST_HASH_SPECIALIZE_REF(type) \
    template <> struct hash<type> \
         : public boost::hash_detail::hash_base<type> \
    { \
        std::size_t operator()(type const& v) const \
        { \
            return boost::hash_value(v); \
        } \
    };

#define BOOST_HASH_SPECIALIZE_TEMPLATE_REF(type) \
    struct hash<type> \
         : public boost::hash_detail::hash_base<type> \
    { \
        std::size_t operator()(type const& v) const \
        { \
            return boost::hash_value(v); \
        } \
    };

    BOOST_HASH_SPECIALIZE(bool)
    BOOST_HASH_SPECIALIZE(char)
    BOOST_HASH_SPECIALIZE(signed char)
    BOOST_HASH_SPECIALIZE(unsigned char)
#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    BOOST_HASH_SPECIALIZE(wchar_t)
#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T)
    BOOST_HASH_SPECIALIZE(char16_t)
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
    BOOST_HASH_SPECIALIZE(char32_t)
#endif
    BOOST_HASH_SPECIALIZE(short)
    BOOST_HASH_SPECIALIZE(unsigned short)
    BOOST_HASH_SPECIALIZE(int)
    BOOST_HASH_SPECIALIZE(unsigned int)
    BOOST_HASH_SPECIALIZE(long)
    BOOST_HASH_SPECIALIZE(unsigned long)

    BOOST_HASH_SPECIALIZE(float)
    BOOST_HASH_SPECIALIZE(double)
    BOOST_HASH_SPECIALIZE(long double)

    BOOST_HASH_SPECIALIZE_REF(std::string)
#if !defined(BOOST_NO_STD_WSTRING) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    BOOST_HASH_SPECIALIZE_REF(std::wstring)
#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T)
    BOOST_HASH_SPECIALIZE_REF(std::basic_string<char16_t>)
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
    BOOST_HASH_SPECIALIZE_REF(std::basic_string<char32_t>)
#endif

#if BOOST_HASH_HAS_STRING_VIEW
    BOOST_HASH_SPECIALIZE_REF(std::string_view)
#   if !defined(BOOST_NO_STD_WSTRING) && !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    BOOST_HASH_SPECIALIZE_REF(std::wstring_view)
#   endif
#   if !defined(BOOST_NO_CXX11_CHAR16_T)
    BOOST_HASH_SPECIALIZE_REF(std::basic_string_view<char16_t>)
#   endif
#   if !defined(BOOST_NO_CXX11_CHAR32_T)
    BOOST_HASH_SPECIALIZE_REF(std::basic_string_view<char32_t>)
#   endif
#endif

#if !defined(BOOST_NO_LONG_LONG)
    BOOST_HASH_SPECIALIZE(boost::long_long_type)
    BOOST_HASH_SPECIALIZE(boost::ulong_long_type)
#endif

#if defined(BOOST_HAS_INT128)
    BOOST_HASH_SPECIALIZE(boost::int128_type)
    BOOST_HASH_SPECIALIZE(boost::uint128_type)
#endif

#if BOOST_HASH_HAS_OPTIONAL
    template <typename T>
    BOOST_HASH_SPECIALIZE_TEMPLATE_REF(std::optional<T>)
#endif

#if !defined(BOOST_HASH_HAS_VARIANT)
    template <typename... T>
    BOOST_HASH_SPECIALIZE_TEMPLATE_REF(std::variant<T...>)
    BOOST_HASH_SPECIALIZE(std::monostate)
#endif

#if !defined(BOOST_NO_CXX11_HDR_TYPEINDEX)
    BOOST_HASH_SPECIALIZE(std::type_index)
#endif

#undef BOOST_HASH_SPECIALIZE
#undef BOOST_HASH_SPECIALIZE_REF
#undef BOOST_HASH_SPECIALIZE_TEMPLATE_REF

// Specializing boost::hash for pointers.

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template <class T>
    struct hash<T*>
        : public boost::hash_detail::hash_base<T*>
    {
        std::size_t operator()(T* v) const
        {
#if !BOOST_WORKAROUND(__SUNPRO_CC, <= 0x590)
            return boost::hash_value(v);
#else
            std::size_t x = static_cast<std::size_t>(
                reinterpret_cast<std::ptrdiff_t>(v));

            return x + (x >> 3);
#endif
        }
    };

#else

    // For compilers without partial specialization, we define a
    // boost::hash for all remaining types. But hash_impl is only defined
    // for pointers in 'extensions.hpp' - so when BOOST_HASH_NO_EXTENSIONS
    // is defined there will still be a compile error for types not supported
    // in the standard.

    namespace hash_detail
    {
        template <bool IsPointer>
        struct hash_impl;

        template <>
        struct hash_impl<true>
        {
            template <class T>
            struct inner
                : public boost::hash_detail::hash_base<T>
            {
                std::size_t operator()(T val) const
                {
#if !BOOST_WORKAROUND(__SUNPRO_CC, <= 590)
                    return boost::hash_value(val);
#else
                    std::size_t x = static_cast<std::size_t>(
                        reinterpret_cast<std::ptrdiff_t>(val));

                    return x + (x >> 3);
#endif
                }
            };
        };
    }

    template <class T> struct hash
        : public boost::hash_detail::hash_impl<boost::is_pointer<T>::value>
            ::BOOST_NESTED_TEMPLATE inner<T>
    {
    };

#endif
}

#undef BOOST_HASH_CHAR_TRAITS
#undef BOOST_FUNCTIONAL_HASH_ROTL32

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_FUNCTIONAL_HASH_HASH_HPP

// Include this outside of the include guards in case the file is included
// twice - once with BOOST_HASH_NO_EXTENSIONS defined, and then with it
// undefined.

#if !defined(BOOST_HASH_NO_EXTENSIONS) \
    && !defined(BOOST_FUNCTIONAL_HASH_EXTENSIONS_HPP)
#include <boost/container_hash/extensions.hpp>
#endif
