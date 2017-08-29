/*
 * Copyright 2013-2014 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef BSON_ATOMIC_H
#define BSON_ATOMIC_H


#include "bson-config.h"
#include "bson-compat.h"
#include "bson-macros.h"


BSON_BEGIN_DECLS


#if defined(__sun) && defined(__SVR4)
/* Solaris */
#include <atomic.h>
#define bson_atomic_int_add(p, v) \
   atomic_add_32_nv ((volatile uint32_t *) p, (v))
#define bson_atomic_int64_add(p, v) \
   atomic_add_64_nv ((volatile uint64_t *) p, (v))
#elif defined(_WIN32)
/* MSVC/MinGW */
#define bson_atomic_int_add(p, v) \
   (InterlockedExchangeAdd ((volatile LONG *) (p), (LONG) (v)) + (LONG) (v))
#define bson_atomic_int64_add(p, v)                                        \
   (InterlockedExchangeAdd64 ((volatile LONGLONG *) (p), (LONGLONG) (v)) + \
    (LONGLONG) (v))
#else
#ifdef BSON_HAVE_ATOMIC_32_ADD_AND_FETCH
#define bson_atomic_int_add(p, v) __sync_add_and_fetch ((p), (v))
#else
#define __BSON_NEED_ATOMIC_32
#endif
#ifdef BSON_HAVE_ATOMIC_64_ADD_AND_FETCH
#if BSON_GNUC_IS_VERSION(4, 1)
/*
 * GCC 4.1 on i386 can generate buggy 64-bit atomic increment.
 * So we will work around with a fallback.
 *
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=40693
 */
#define __BSON_NEED_ATOMIC_64
#else
#define bson_atomic_int64_add(p, v) \
   __sync_add_and_fetch ((volatile int64_t *) (p), (int64_t) (v))
#endif
#else
#define __BSON_NEED_ATOMIC_64
#endif
#endif

#ifdef __BSON_NEED_ATOMIC_32
BSON_EXPORT (int32_t)
bson_atomic_int_add (volatile int32_t *p, int32_t n);
#endif
#ifdef __BSON_NEED_ATOMIC_64
BSON_EXPORT (int64_t)
bson_atomic_int64_add (volatile int64_t *p, int64_t n);
#endif


#if defined(_WIN32)
#define bson_memory_barrier() MemoryBarrier ()
#elif defined(__GNUC__)
#if BSON_GNUC_CHECK_VERSION(4, 1)
#define bson_memory_barrier() __sync_synchronize ()
#else
#warning "GCC Pre-4.1 discovered, using inline assembly for memory barrier."
#define bson_memory_barrier() __asm__ volatile("" ::: "memory")
#endif
#elif defined(__SUNPRO_C)
#include <mbarrier.h>
#define bson_memory_barrier() __machine_rw_barrier ()
#elif defined(__xlC__)
#define bson_memory_barrier() __sync ()
#else
#define __BSON_NEED_BARRIER 1
#warning "Unknown compiler, using lock for compiler barrier."
BSON_EXPORT (void)
bson_memory_barrier (void);
#endif


BSON_END_DECLS


#endif /* BSON_ATOMIC_H */
