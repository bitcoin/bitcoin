/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_aarch64_common.hpp
 *
 * This header contains basic utilities for gcc AArch64 backend.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_AARCH64_COMMON_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_AARCH64_COMMON_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#define BOOST_ATOMIC_DETAIL_AARCH64_MO_SWITCH(mo)\
    switch (mo)\
    {\
    case memory_order_relaxed:\
        BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN("", "")\
        break;\
    \
    case memory_order_consume:\
    case memory_order_acquire:\
        BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN("a", "")\
        break;\
    \
    case memory_order_release:\
        BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN("", "l")\
        break;\
    \
    default:\
        BOOST_ATOMIC_DETAIL_AARCH64_MO_INSN("a", "l")\
        break;\
    }

#if defined(BOOST_ATOMIC_DETAIL_AARCH64_LITTLE_ENDIAN)
#define BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "0"
#define BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "1"
#else
#define BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_LO "1"
#define BOOST_ATOMIC_DETAIL_AARCH64_ASM_ARG_HI "0"
#endif

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_AARCH64_COMMON_HPP_INCLUDED_
