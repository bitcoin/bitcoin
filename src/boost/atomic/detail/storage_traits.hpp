/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2012 Tim Blechmann
 * Copyright (c) 2013 - 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/storage_traits.hpp
 *
 * This header defines underlying types used as storage
 */

#ifndef BOOST_ATOMIC_DETAIL_STORAGE_TRAITS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_STORAGE_TRAITS_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/string_ops.hpp>
#include <boost/atomic/detail/aligned_variable.hpp>
#include <boost/atomic/detail/type_traits/alignment_of.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename T >
BOOST_FORCEINLINE void non_atomic_load(T const volatile& from, T& to) BOOST_NOEXCEPT
{
    to = from;
}

template< std::size_t Size, std::size_t Alignment = 1u >
struct BOOST_ATOMIC_DETAIL_MAY_ALIAS buffer_storage
{
    typedef unsigned char data_type[Size];
    BOOST_ATOMIC_DETAIL_ALIGNED_VAR_TPL(Alignment, data_type, data);

    BOOST_FORCEINLINE bool operator! () const BOOST_NOEXCEPT
    {
        return (data[0] == 0u && BOOST_ATOMIC_DETAIL_MEMCMP(data, data + 1, Size - 1u) == 0);
    }

    BOOST_FORCEINLINE bool operator== (buffer_storage const& that) const BOOST_NOEXCEPT
    {
        return BOOST_ATOMIC_DETAIL_MEMCMP(data, that.data, Size) == 0;
    }

    BOOST_FORCEINLINE bool operator!= (buffer_storage const& that) const BOOST_NOEXCEPT
    {
        return BOOST_ATOMIC_DETAIL_MEMCMP(data, that.data, Size) != 0;
    }
};

template< std::size_t Size, std::size_t Alignment >
BOOST_FORCEINLINE void non_atomic_load(buffer_storage< Size, Alignment > const volatile& from, buffer_storage< Size, Alignment >& to) BOOST_NOEXCEPT
{
    BOOST_ATOMIC_DETAIL_MEMCPY(to.data, const_cast< unsigned char const* >(from.data), Size);
}

template< std::size_t Size >
struct storage_traits
{
    typedef buffer_storage< Size, 1u > type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = 1u;

    // By default, prefer the maximum supported alignment
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 16u;
};

template< >
struct storage_traits< 1u >
{
    typedef boost::uint8_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = 1u;
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 1u;
};

template< >
struct storage_traits< 2u >
{
    typedef boost::uint16_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = atomics::detail::alignment_of< boost::uint16_t >::value;
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 2u;
};

template< >
struct storage_traits< 4u >
{
    typedef boost::uint32_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = atomics::detail::alignment_of< boost::uint32_t >::value;
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 4u;
};

template< >
struct storage_traits< 8u >
{
    typedef boost::uint64_t BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = atomics::detail::alignment_of< boost::uint64_t >::value;
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 8u;
};

#if defined(BOOST_HAS_INT128)

template< >
struct storage_traits< 16u >
{
    typedef boost::uint128_type BOOST_ATOMIC_DETAIL_MAY_ALIAS type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = atomics::detail::alignment_of< boost::uint128_type >::value;
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 16u;
};

#else

#if (__cplusplus >= 201103L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201103L)) &&\
    (!defined(BOOST_GCC_VERSION) || BOOST_GCC_VERSION >= 40900)
using std::max_align_t;
#else

#if defined(BOOST_MSVC)
#pragma warning(push)
// alignment is sensitive to packing
#pragma warning(disable: 4121)
#endif

class max_align_helper;
union max_align_t
{
    void* ptr;
    void (*fun_ptr)();
    int max_align_helper::*mem_ptr;
    void (max_align_helper::*mem_fun_ptr)();
    long long ll;
    long double ld;
#if defined(BOOST_HAS_INT128)
    boost::int128_type i128;
#endif
#if defined(BOOST_HAS_FLOAT128)
    boost::float128_type f128;
#endif
};

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // __cplusplus >= 201103L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201103L)

template< >
struct storage_traits< 16u >
{
    typedef buffer_storage< 16u, atomics::detail::alignment_of< atomics::detail::max_align_t >::value > type;

    static BOOST_CONSTEXPR_OR_CONST std::size_t native_alignment = atomics::detail::alignment_of< atomics::detail::max_align_t >::value;
    static BOOST_CONSTEXPR_OR_CONST std::size_t alignment = 16u;
};

#endif

template< typename T >
struct storage_size_of
{
    static BOOST_CONSTEXPR_OR_CONST std::size_t size = sizeof(T);
    static BOOST_CONSTEXPR_OR_CONST std::size_t value = (size == 3u ? 4u : (size >= 5u && size <= 7u ? 8u : (size >= 9u && size <= 15u ? 16u : size)));
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_STORAGE_TRAITS_HPP_INCLUDED_
