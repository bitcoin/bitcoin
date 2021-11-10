/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/core_operations.hpp
 *
 * This header defines core atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_CORE_OPERATIONS_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CORE_OPERATIONS_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/platform.hpp>
#include <boost/atomic/detail/core_arch_operations.hpp>
#include <boost/atomic/detail/core_operations_fwd.hpp>

#if defined(BOOST_ATOMIC_DETAIL_CORE_BACKEND_HEADER)
#include BOOST_ATOMIC_DETAIL_CORE_BACKEND_HEADER(boost/atomic/detail/core_ops_)
#endif

#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

//! Default specialization that falls back to architecture-specific implementation
template< std::size_t Size, bool Signed, bool Interprocess >
struct core_operations :
    public core_arch_operations< Size, Signed, Interprocess >
{
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_CORE_OPERATIONS_HPP_INCLUDED_
