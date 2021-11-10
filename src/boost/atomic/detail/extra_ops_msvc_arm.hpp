/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2017 Andrey Semashev
 */
/*!
 * \file   atomic/detail/extra_ops_msvc_arm.hpp
 *
 * This header contains implementation of the extra atomic operations for ARM.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXTRA_OPS_MSVC_ARM_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXTRA_OPS_MSVC_ARM_HPP_INCLUDED_

#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/interlocked.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/extra_operations_fwd.hpp>
#include <boost/atomic/detail/extra_ops_generic.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

#if defined(BOOST_ATOMIC_INTERLOCKED_BTS) && defined(BOOST_ATOMIC_INTERLOCKED_BTR)

template< typename Base, std::size_t Size, bool Signed >
struct extra_operations< Base, 4u, Signed, true > :
    public extra_operations_generic< Base, 4u, Signed >
{
    typedef extra_operations_generic< Base, 4u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE bool bit_test_and_set(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
#if defined(BOOST_ATOMIC_INTERLOCKED_BTS_RELAXED) && defined(BOOST_ATOMIC_INTERLOCKED_BTS_ACQUIRE) && defined(BOOST_ATOMIC_INTERLOCKED_BTS_RELEASE)
        bool result;
        switch (order)
        {
        case memory_order_relaxed:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTS_RELAXED(&storage, bit_number);
            break;
        case memory_order_consume:
        case memory_order_acquire:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTS_ACQUIRE(&storage, bit_number);
            break;
        case memory_order_release:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTS_RELEASE(&storage, bit_number);
            break;
        case memory_order_acq_rel:
        case memory_order_seq_cst:
        default:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTS(&storage, bit_number);
            break;
        }
        return result;
#else
        return !!BOOST_ATOMIC_INTERLOCKED_BTS(&storage, bit_number);
#endif
    }

    static BOOST_FORCEINLINE bool bit_test_and_reset(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
#if defined(BOOST_ATOMIC_INTERLOCKED_BTR_RELAXED) && defined(BOOST_ATOMIC_INTERLOCKED_BTR_ACQUIRE) && defined(BOOST_ATOMIC_INTERLOCKED_BTR_RELEASE)
        bool result;
        switch (order)
        {
        case memory_order_relaxed:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTR_RELAXED(&storage, bit_number);
            break;
        case memory_order_consume:
        case memory_order_acquire:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTR_ACQUIRE(&storage, bit_number);
            break;
        case memory_order_release:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTR_RELEASE(&storage, bit_number);
            break;
        case memory_order_acq_rel:
        case memory_order_seq_cst:
        default:
            result = !!BOOST_ATOMIC_INTERLOCKED_BTR(&storage, bit_number);
            break;
        }
        return result;
#else
        return !!BOOST_ATOMIC_INTERLOCKED_BTR(&storage, bit_number);
#endif
    }
};

#endif // defined(BOOST_ATOMIC_INTERLOCKED_BTS) && defined(BOOST_ATOMIC_INTERLOCKED_BTR)

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_EXTRA_OPS_MSVC_ARM_HPP_INCLUDED_
