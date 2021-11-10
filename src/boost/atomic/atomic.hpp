/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2011 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014, 2020 Andrey Semashev
 */
/*!
 * \file   atomic/atomic.hpp
 *
 * This header contains definition of \c atomic template.
 */

#ifndef BOOST_ATOMIC_ATOMIC_HPP_INCLUDED_
#define BOOST_ATOMIC_ATOMIC_HPP_INCLUDED_

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/capabilities.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/classify.hpp>
#include <boost/atomic/detail/atomic_impl.hpp>
#include <boost/atomic/detail/type_traits/is_trivially_copyable.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {

//! Atomic object
template< typename T >
class atomic :
    public atomics::detail::base_atomic< T, typename atomics::detail::classify< T >::type, false >
{
private:
    typedef atomics::detail::base_atomic< T, typename atomics::detail::classify< T >::type, false > base_type;
    typedef typename base_type::value_arg_type value_arg_type;

public:
    typedef typename base_type::value_type value_type;
    // Deprecated, use value_type instead
    BOOST_ATOMIC_DETAIL_STORAGE_DEPRECATED
    typedef typename base_type::storage_type storage_type;

    BOOST_STATIC_ASSERT_MSG(sizeof(value_type) > 0u, "boost::atomic<T> requires T to be a complete type");
#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_IS_TRIVIALLY_COPYABLE)
    BOOST_STATIC_ASSERT_MSG(atomics::detail::is_trivially_copyable< value_type >::value, "boost::atomic<T> requires T to be a trivially copyable type");
#endif

public:
    BOOST_DEFAULTED_FUNCTION(atomic() BOOST_ATOMIC_DETAIL_DEF_NOEXCEPT_DECL, BOOST_ATOMIC_DETAIL_DEF_NOEXCEPT_IMPL {})
    BOOST_FORCEINLINE BOOST_ATOMIC_DETAIL_CONSTEXPR_UNION_INIT atomic(value_arg_type v) BOOST_NOEXCEPT : base_type(v) {}

    BOOST_FORCEINLINE value_type operator= (value_arg_type v) BOOST_NOEXCEPT
    {
        this->store(v);
        return v;
    }

    BOOST_FORCEINLINE value_type operator= (value_arg_type v) volatile BOOST_NOEXCEPT
    {
        this->store(v);
        return v;
    }

    BOOST_FORCEINLINE operator value_type() const volatile BOOST_NOEXCEPT
    {
        return this->load();
    }

    // Deprecated, use value() instead
    BOOST_ATOMIC_DETAIL_STORAGE_DEPRECATED
    BOOST_FORCEINLINE typename base_type::storage_type& storage() BOOST_NOEXCEPT { return base_type::storage(); }
    BOOST_ATOMIC_DETAIL_STORAGE_DEPRECATED
    BOOST_FORCEINLINE typename base_type::storage_type volatile& storage() volatile BOOST_NOEXCEPT { return base_type::storage(); }
    BOOST_ATOMIC_DETAIL_STORAGE_DEPRECATED
    BOOST_FORCEINLINE typename base_type::storage_type const& storage() const BOOST_NOEXCEPT { return base_type::storage(); }
    BOOST_ATOMIC_DETAIL_STORAGE_DEPRECATED
    BOOST_FORCEINLINE typename base_type::storage_type const volatile& storage() const volatile BOOST_NOEXCEPT { return base_type::storage(); }

    BOOST_DELETED_FUNCTION(atomic(atomic const&))
    BOOST_DELETED_FUNCTION(atomic& operator= (atomic const&))
    BOOST_DELETED_FUNCTION(atomic& operator= (atomic const&) volatile)
};

typedef atomic< char > atomic_char;
typedef atomic< unsigned char > atomic_uchar;
typedef atomic< signed char > atomic_schar;
typedef atomic< uint8_t > atomic_uint8_t;
typedef atomic< int8_t > atomic_int8_t;
typedef atomic< unsigned short > atomic_ushort;
typedef atomic< short > atomic_short;
typedef atomic< uint16_t > atomic_uint16_t;
typedef atomic< int16_t > atomic_int16_t;
typedef atomic< unsigned int > atomic_uint;
typedef atomic< int > atomic_int;
typedef atomic< uint32_t > atomic_uint32_t;
typedef atomic< int32_t > atomic_int32_t;
typedef atomic< unsigned long > atomic_ulong;
typedef atomic< long > atomic_long;
typedef atomic< uint64_t > atomic_uint64_t;
typedef atomic< int64_t > atomic_int64_t;
#ifdef BOOST_HAS_LONG_LONG
typedef atomic< boost::ulong_long_type > atomic_ullong;
typedef atomic< boost::long_long_type > atomic_llong;
#endif
typedef atomic< void* > atomic_address;
typedef atomic< bool > atomic_bool;
typedef atomic< wchar_t > atomic_wchar_t;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811
typedef atomic< char8_t > atomic_char8_t;
#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T)
typedef atomic< char16_t > atomic_char16_t;
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
typedef atomic< char32_t > atomic_char32_t;
#endif

typedef atomic< int_least8_t > atomic_int_least8_t;
typedef atomic< uint_least8_t > atomic_uint_least8_t;
typedef atomic< int_least16_t > atomic_int_least16_t;
typedef atomic< uint_least16_t > atomic_uint_least16_t;
typedef atomic< int_least32_t > atomic_int_least32_t;
typedef atomic< uint_least32_t > atomic_uint_least32_t;
typedef atomic< int_least64_t > atomic_int_least64_t;
typedef atomic< uint_least64_t > atomic_uint_least64_t;
typedef atomic< int_fast8_t > atomic_int_fast8_t;
typedef atomic< uint_fast8_t > atomic_uint_fast8_t;
typedef atomic< int_fast16_t > atomic_int_fast16_t;
typedef atomic< uint_fast16_t > atomic_uint_fast16_t;
typedef atomic< int_fast32_t > atomic_int_fast32_t;
typedef atomic< uint_fast32_t > atomic_uint_fast32_t;
typedef atomic< int_fast64_t > atomic_int_fast64_t;
typedef atomic< uint_fast64_t > atomic_uint_fast64_t;
typedef atomic< intmax_t > atomic_intmax_t;
typedef atomic< uintmax_t > atomic_uintmax_t;

#if !defined(BOOST_ATOMIC_NO_FLOATING_POINT)
typedef atomic< float > atomic_float_t;
typedef atomic< double > atomic_double_t;
typedef atomic< long double > atomic_long_double_t;
#endif

typedef atomic< std::size_t > atomic_size_t;
typedef atomic< std::ptrdiff_t > atomic_ptrdiff_t;

#if defined(BOOST_HAS_INTPTR_T)
typedef atomic< boost::intptr_t > atomic_intptr_t;
typedef atomic< boost::uintptr_t > atomic_uintptr_t;
#endif

// Select the lock-free atomic types that has natively supported waiting/notifying operations.
// Prefer 32-bit types the most as those have the best performance on current 32 and 64-bit architectures.
#if BOOST_ATOMIC_INT32_LOCK_FREE == 2 && BOOST_ATOMIC_HAS_NATIVE_INT32_WAIT_NOTIFY == 2
typedef atomic< uint32_t > atomic_unsigned_lock_free;
typedef atomic< int32_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT64_LOCK_FREE == 2 && BOOST_ATOMIC_HAS_NATIVE_INT64_WAIT_NOTIFY == 2
typedef atomic< uint64_t > atomic_unsigned_lock_free;
typedef atomic< int64_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT16_LOCK_FREE == 2 && BOOST_ATOMIC_HAS_NATIVE_INT16_WAIT_NOTIFY == 2
typedef atomic< uint16_t > atomic_unsigned_lock_free;
typedef atomic< int16_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT8_LOCK_FREE == 2 && BOOST_ATOMIC_HAS_NATIVE_INT8_WAIT_NOTIFY == 2
typedef atomic< uint8_t > atomic_unsigned_lock_free;
typedef atomic< int8_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT32_LOCK_FREE == 2
typedef atomic< uint32_t > atomic_unsigned_lock_free;
typedef atomic< int32_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT64_LOCK_FREE == 2
typedef atomic< uint64_t > atomic_unsigned_lock_free;
typedef atomic< int64_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT16_LOCK_FREE == 2
typedef atomic< uint16_t > atomic_unsigned_lock_free;
typedef atomic< int16_t > atomic_signed_lock_free;
#elif BOOST_ATOMIC_INT8_LOCK_FREE == 2
typedef atomic< uint8_t > atomic_unsigned_lock_free;
typedef atomic< int8_t > atomic_signed_lock_free;
#else
#define BOOST_ATOMIC_DETAIL_NO_LOCK_FREE_TYPEDEFS
#endif

} // namespace atomics

using atomics::atomic;

using atomics::atomic_char;
using atomics::atomic_uchar;
using atomics::atomic_schar;
using atomics::atomic_uint8_t;
using atomics::atomic_int8_t;
using atomics::atomic_ushort;
using atomics::atomic_short;
using atomics::atomic_uint16_t;
using atomics::atomic_int16_t;
using atomics::atomic_uint;
using atomics::atomic_int;
using atomics::atomic_uint32_t;
using atomics::atomic_int32_t;
using atomics::atomic_ulong;
using atomics::atomic_long;
using atomics::atomic_uint64_t;
using atomics::atomic_int64_t;
#ifdef BOOST_HAS_LONG_LONG
using atomics::atomic_ullong;
using atomics::atomic_llong;
#endif
using atomics::atomic_address;
using atomics::atomic_bool;
using atomics::atomic_wchar_t;
#if defined(__cpp_char8_t) && __cpp_char8_t >= 201811
using atomics::atomic_char8_t;
#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T)
using atomics::atomic_char16_t;
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
using atomics::atomic_char32_t;
#endif

using atomics::atomic_int_least8_t;
using atomics::atomic_uint_least8_t;
using atomics::atomic_int_least16_t;
using atomics::atomic_uint_least16_t;
using atomics::atomic_int_least32_t;
using atomics::atomic_uint_least32_t;
using atomics::atomic_int_least64_t;
using atomics::atomic_uint_least64_t;
using atomics::atomic_int_fast8_t;
using atomics::atomic_uint_fast8_t;
using atomics::atomic_int_fast16_t;
using atomics::atomic_uint_fast16_t;
using atomics::atomic_int_fast32_t;
using atomics::atomic_uint_fast32_t;
using atomics::atomic_int_fast64_t;
using atomics::atomic_uint_fast64_t;
using atomics::atomic_intmax_t;
using atomics::atomic_uintmax_t;

#if !defined(BOOST_ATOMIC_NO_FLOATING_POINT)
using atomics::atomic_float_t;
using atomics::atomic_double_t;
using atomics::atomic_long_double_t;
#endif

using atomics::atomic_size_t;
using atomics::atomic_ptrdiff_t;

#if defined(BOOST_HAS_INTPTR_T)
using atomics::atomic_intptr_t;
using atomics::atomic_uintptr_t;
#endif

#if !defined(BOOST_ATOMIC_DETAIL_NO_LOCK_FREE_TYPEDEFS)
using atomics::atomic_unsigned_lock_free;
using atomics::atomic_signed_lock_free;
#endif
#undef BOOST_ATOMIC_DETAIL_NO_LOCK_FREE_TYPEDEFS

} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_ATOMIC_HPP_INCLUDED_
