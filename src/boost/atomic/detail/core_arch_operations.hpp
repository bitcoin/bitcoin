/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/core_arch_operations.hpp
 *
 * This header defines core atomic operations, including the emulated version.
 */

#ifndef BOOST_ATOMIC_DETAIL_CORE_ARCH_OPERATIONS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CORE_ARCH_OPERATIONS_HPP_INCLUDED_

#include <boost/atomic/detail/core_arch_operations_fwd.hpp>
#include <boost/atomic/detail/core_operations_emulated.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>
#include <boost/atomic/detail/storage_traits.hpp>

#if defined(BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND_HEADER)
#include BOOST_ATOMIC_DETAIL_CORE_ARCH_BACKEND_HEADER(boost/atomic/detail/core_arch_ops_)
#endif

#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Default specialization that falls back to lock-based implementation
template< std::size_t Size, bool Signed, bool Interprocess >
struct core_arch_operations :
    public core_operations_emulated< Size, storage_traits< Size >::alignment, Signed, Interprocess >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_CORE_ARCH_OPERATIONS_HPP_INCLUDED_
