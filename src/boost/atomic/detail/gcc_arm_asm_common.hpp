/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014, 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/gcc_arm_asm_common.hpp
 *
 * This header contains basic utilities for gcc asm-based ARM backend.
 */

#ifndef BOOST_ATOMIC_DETAIL_GCC_ARM_ASM_COMMON_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_GCC_ARM_ASM_COMMON_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/capabilities.hpp>

// A memory barrier is effected using a "co-processor 15" instruction,
// though a separate assembler mnemonic is available for it in v7.
//
// "Thumb 1" is a subset of the ARM instruction set that uses a 16-bit encoding.  It
// doesn't include all instructions and in particular it doesn't include the co-processor
// instruction used for the memory barrier or the load-locked/store-conditional
// instructions.  So, if we're compiling in "Thumb 1" mode, we need to wrap all of our
// asm blocks with code to temporarily change to ARM mode.
//
// You can only change between ARM and Thumb modes when branching using the bx instruction.
// bx takes an address specified in a register.  The least significant bit of the address
// indicates the mode, so 1 is added to indicate that the destination code is Thumb.
// A temporary register is needed for the address and is passed as an argument to these
// macros.  It must be one of the "low" registers accessible to Thumb code, specified
// using the "l" attribute in the asm statement.
//
// Architecture v7 introduces "Thumb 2", which does include (almost?) all of the ARM
// instruction set.  (Actually, there was an extension of v6 called v6T2 which supported
// "Thumb 2" mode, but its architecture manual is no longer available, referring to v7.)
// So in v7 we don't need to change to ARM mode; we can write "universal
// assembler" which will assemble to Thumb 2 or ARM code as appropriate.  The only thing
// we need to do to make this "universal" assembler mode work is to insert "IT" instructions
// to annotate the conditional instructions.  These are ignored in other modes (e.g. v6),
// so they can always be present.

// A note about memory_order_consume. Technically, this architecture allows to avoid
// unnecessary memory barrier after consume load since it supports data dependency ordering.
// However, some compiler optimizations may break a seemingly valid code relying on data
// dependency tracking by injecting bogus branches to aid out of order execution.
// This may happen not only in Boost.Atomic code but also in user's code, which we have no
// control of. See this thread: http://lists.boost.org/Archives/boost/2014/06/213890.php.
// For this reason we promote memory_order_consume to memory_order_acquire.

#if defined(__thumb__) && !defined(__thumb2__)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_START(TMPREG) "adr " #TMPREG ", 8f\n\t" "bx " #TMPREG "\n\t" ".arm\n\t" ".align 4\n\t" "8:\n\t"
#define BOOST_ATOMIC_DETAIL_ARM_ASM_END(TMPREG)   "adr " #TMPREG ", 9f + 1\n\t" "bx " #TMPREG "\n\t" ".thumb\n\t" ".align 2\n\t" "9:\n\t"
#define BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(var) "=&l" (var)
#else
// Indicate that start/end macros are empty and the tmpreg is not needed
#define BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_UNUSED
#define BOOST_ATOMIC_DETAIL_ARM_ASM_START(TMPREG)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_END(TMPREG)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_TMPREG_CONSTRAINT(var) "=&l" (var)
#endif

#if defined(BOOST_ATOMIC_DETAIL_ARM_LITTLE_ENDIAN)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(arg) "%" BOOST_STRINGIZE(arg)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(arg) "%H" BOOST_STRINGIZE(arg)
#else
#define BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_LO(arg) "%H" BOOST_STRINGIZE(arg)
#define BOOST_ATOMIC_DETAIL_ARM_ASM_ARG_HI(arg) "%" BOOST_STRINGIZE(arg)
#endif

#endif // BOOST_ATOMIC_DETAIL_GCC_ARM_ASM_COMMON_HPP_INCLUDED_
