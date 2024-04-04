/***********************************************************************
 * Copyright (c) 2022 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

/* The code here is inspired by Kris Kwiatkowski's approach in
 * https://github.com/kriskwiatkowski/pqc/blob/main/src/common/ct_check.h
 * to provide a general interface for memory-checking mechanisms, primarily
 * for constant-time checking.
 */

/* These macros are defined by this header file:
 *
 * - SECP256K1_CHECKMEM_ENABLED:
 *   - 1 if memory-checking integration is available, 0 otherwise.
 *     This is just a compile-time macro. Use the next macro to check it is actually
 *     available at runtime.
 * - SECP256K1_CHECKMEM_RUNNING():
 *   - Acts like a function call, returning 1 if memory checking is available
 *     at runtime.
 * - SECP256K1_CHECKMEM_CHECK(p, len):
 *   - Assert or otherwise fail in case the len-byte memory block pointed to by p is
 *     not considered entirely defined.
 * - SECP256K1_CHECKMEM_CHECK_VERIFY(p, len):
 *   - Like SECP256K1_CHECKMEM_CHECK, but only works in VERIFY mode.
 * - SECP256K1_CHECKMEM_UNDEFINE(p, len):
 *   - marks the len-byte memory block pointed to by p as undefined data (secret data,
 *     in the context of constant-time checking).
 * - SECP256K1_CHECKMEM_DEFINE(p, len):
 *   - marks the len-byte memory pointed to by p as defined data (public data, in the
 *     context of constant-time checking).
 * - SECP256K1_CHECKMEM_MSAN_DEFINE(p, len):
 *   - Like SECP256K1_CHECKMEM_DEFINE, but applies only to memory_sanitizer.
 *
 */

#ifndef SECP256K1_CHECKMEM_H
#define SECP256K1_CHECKMEM_H

/* Define a statement-like macro that ignores the arguments. */
#define SECP256K1_CHECKMEM_NOOP(p, len) do { (void)(p); (void)(len); } while(0)

/* If compiling under msan, map the SECP256K1_CHECKMEM_* functionality to msan.
 * Choose this preferentially, even when VALGRIND is defined, as msan-compiled
 * binaries can't be run under valgrind anyway. */
#if defined(__has_feature)
#  if __has_feature(memory_sanitizer)
#    include <sanitizer/msan_interface.h>
#    define SECP256K1_CHECKMEM_ENABLED 1
#    define SECP256K1_CHECKMEM_UNDEFINE(p, len) __msan_allocated_memory((p), (len))
#    define SECP256K1_CHECKMEM_DEFINE(p, len) __msan_unpoison((p), (len))
#    define SECP256K1_CHECKMEM_MSAN_DEFINE(p, len) __msan_unpoison((p), (len))
#    define SECP256K1_CHECKMEM_CHECK(p, len) __msan_check_mem_is_initialized((p), (len))
#    define SECP256K1_CHECKMEM_RUNNING() (1)
#  endif
#endif

#if !defined SECP256K1_CHECKMEM_MSAN_DEFINE
#  define SECP256K1_CHECKMEM_MSAN_DEFINE(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#endif

/* If valgrind integration is desired (through the VALGRIND define), implement the
 * SECP256K1_CHECKMEM_* macros using valgrind. */
#if !defined SECP256K1_CHECKMEM_ENABLED
#  if defined VALGRIND
#    include <stddef.h>
#  if defined(__clang__) && defined(__APPLE__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wreserved-identifier"
#  endif
#    include <valgrind/memcheck.h>
#  if defined(__clang__) && defined(__APPLE__)
#    pragma clang diagnostic pop
#  endif
#    define SECP256K1_CHECKMEM_ENABLED 1
#    define SECP256K1_CHECKMEM_UNDEFINE(p, len) VALGRIND_MAKE_MEM_UNDEFINED((p), (len))
#    define SECP256K1_CHECKMEM_DEFINE(p, len) VALGRIND_MAKE_MEM_DEFINED((p), (len))
#    define SECP256K1_CHECKMEM_CHECK(p, len) VALGRIND_CHECK_MEM_IS_DEFINED((p), (len))
     /* VALGRIND_MAKE_MEM_DEFINED returns 0 iff not running on memcheck.
      * This is more precise than the RUNNING_ON_VALGRIND macro, which
      * checks for valgrind in general instead of memcheck specifically. */
#    define SECP256K1_CHECKMEM_RUNNING() (VALGRIND_MAKE_MEM_DEFINED(NULL, 0) != 0)
#  endif
#endif

/* As a fall-back, map these macros to dummy statements. */
#if !defined SECP256K1_CHECKMEM_ENABLED
#  define SECP256K1_CHECKMEM_ENABLED 0
#  define SECP256K1_CHECKMEM_UNDEFINE(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#  define SECP256K1_CHECKMEM_DEFINE(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#  define SECP256K1_CHECKMEM_CHECK(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#  define SECP256K1_CHECKMEM_RUNNING() (0)
#endif

#if defined VERIFY
#define SECP256K1_CHECKMEM_CHECK_VERIFY(p, len) SECP256K1_CHECKMEM_CHECK((p), (len))
#else
#define SECP256K1_CHECKMEM_CHECK_VERIFY(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#endif

#endif /* SECP256K1_CHECKMEM_H */
