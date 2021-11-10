/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2018 Andrey Semashev
 */
/*!
 * \file   atomic/detail/extra_fp_ops_generic.hpp
 *
 * This header contains generic implementation of the extra floating point atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXTRA_FP_OPS_GENERIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXTRA_FP_OPS_GENERIC_HPP_INCLUDED_

#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/bitwise_fp_cast.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/extra_fp_operations_fwd.hpp>
#include <boost/atomic/detail/type_traits/is_iec559.hpp>
#include <boost/atomic/detail/type_traits/is_integral.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_GCC) && BOOST_GCC >= 60000
#pragma GCC diagnostic push
// ignoring attributes on template argument X - this warning is because we need to pass storage_type as a template argument; no problem in this case
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Negate implementation
template<
    typename Base,
    typename Value,
    std::size_t Size
#if defined(BOOST_ATOMIC_DETAIL_INT_FP_ENDIAN_MATCH)
    , bool = atomics::detail::is_iec559< Value >::value && atomics::detail::is_integral< typename Base::storage_type >::value
#endif
>
struct extra_fp_negate_generic :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;
    typedef Value value_type;

    static BOOST_FORCEINLINE value_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_storage, new_storage;
        value_type old_val, new_val;
        atomics::detail::non_atomic_load(storage, old_storage);
        do
        {
            old_val = atomics::detail::bitwise_fp_cast< value_type >(old_storage);
            new_val = -old_val;
            new_storage = atomics::detail::bitwise_fp_cast< storage_type >(new_val);
        }
        while (!base_type::compare_exchange_weak(storage, old_storage, new_storage, order, memory_order_relaxed));
        return old_val;
    }

    static BOOST_FORCEINLINE value_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_storage, new_storage;
        value_type old_val, new_val;
        atomics::detail::non_atomic_load(storage, old_storage);
        do
        {
            old_val = atomics::detail::bitwise_fp_cast< value_type >(old_storage);
            new_val = -old_val;
            new_storage = atomics::detail::bitwise_fp_cast< storage_type >(new_val);
        }
        while (!base_type::compare_exchange_weak(storage, old_storage, new_storage, order, memory_order_relaxed));
        return new_val;
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        fetch_negate(storage, order);
    }
};

#if defined(BOOST_ATOMIC_DETAIL_INT_FP_ENDIAN_MATCH)

//! Negate implementation for IEEE 754 / IEC 559 floating point types. We leverage the fact that the sign bit is the most significant bit in the value.
template< typename Base, typename Value, std::size_t Size >
struct extra_fp_negate_generic< Base, Value, Size, true > :
    public Base
{
    typedef Base base_type;
    typedef typename base_type::storage_type storage_type;
    typedef Value value_type;

    //! The mask with only one sign bit set to 1
    static BOOST_CONSTEXPR_OR_CONST storage_type sign_mask = static_cast< storage_type >(1u) << (atomics::detail::value_sizeof< value_type >::value * 8u - 1u);

    static BOOST_FORCEINLINE value_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return atomics::detail::bitwise_fp_cast< value_type >(base_type::fetch_xor(storage, sign_mask, order));
    }

    static BOOST_FORCEINLINE value_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return atomics::detail::bitwise_fp_cast< value_type >(base_type::bitwise_xor(storage, sign_mask, order));
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::opaque_xor(storage, sign_mask, order);
    }
};

#endif // defined(BOOST_ATOMIC_DETAIL_INT_FP_ENDIAN_MATCH)

//! Generic implementation of floating point operations
template< typename Base, typename Value, std::size_t Size >
struct extra_fp_operations_generic :
    public extra_fp_negate_generic< Base, Value, Size >
{
    typedef extra_fp_negate_generic< Base, Value, Size > base_type;
    typedef typename base_type::storage_type storage_type;
    typedef Value value_type;

    static BOOST_FORCEINLINE value_type add(storage_type volatile& storage, value_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_storage, new_storage;
        value_type old_val, new_val;
        atomics::detail::non_atomic_load(storage, old_storage);
        do
        {
            old_val = atomics::detail::bitwise_fp_cast< value_type >(old_storage);
            new_val = old_val + v;
            new_storage = atomics::detail::bitwise_fp_cast< storage_type >(new_val);
        }
        while (!base_type::compare_exchange_weak(storage, old_storage, new_storage, order, memory_order_relaxed));
        return new_val;
    }

    static BOOST_FORCEINLINE value_type sub(storage_type volatile& storage, value_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type old_storage, new_storage;
        value_type old_val, new_val;
        atomics::detail::non_atomic_load(storage, old_storage);
        do
        {
            old_val = atomics::detail::bitwise_fp_cast< value_type >(old_storage);
            new_val = old_val - v;
            new_storage = atomics::detail::bitwise_fp_cast< storage_type >(new_val);
        }
        while (!base_type::compare_exchange_weak(storage, old_storage, new_storage, order, memory_order_relaxed));
        return new_val;
    }

    static BOOST_FORCEINLINE void opaque_add(storage_type volatile& storage, value_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fetch_add(storage, v, order);
    }

    static BOOST_FORCEINLINE void opaque_sub(storage_type volatile& storage, value_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fetch_sub(storage, v, order);
    }
};

// Default extra_fp_operations template definition will be used unless specialized for a specific platform
template< typename Base, typename Value, std::size_t Size >
struct extra_fp_operations< Base, Value, Size, true > :
    public extra_fp_operations_generic< Base, Value, Size >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#if defined(BOOST_GCC) && BOOST_GCC >= 60000
#pragma GCC diagnostic pop
#endif

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_FP_OPS_GENERIC_HPP_INCLUDED_
