/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/once_flag.hpp
 *
 * This header declares \c once_flag structure for controlling one time initialization
 */

#ifndef BOOST_ATOMIC_DETAIL_ONCE_FLAG_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ONCE_FLAG_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/aligned_variable.hpp>
#include <boost/atomic/detail/core_operations.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

typedef atomics::detail::core_operations< 1u, false, false > once_flag_operations;

struct once_flag
{
    BOOST_ATOMIC_DETAIL_ALIGNED_VAR(once_flag_operations::storage_alignment, once_flag_operations::storage_type, m_flag);
};

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_ONCE_FLAG_HPP_INCLUDED_
