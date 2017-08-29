/*
 * Copyright 2013 MongoDB, Inc.
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

#ifndef MONGOC_COUNTERS_PRIVATE_H
#define MONGOC_COUNTERS_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#ifdef __linux__
#include <sched.h>
#include <sys/sysinfo.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
   defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#elif defined(__hpux__)
#include <sys/pstat.h>
#endif


BSON_BEGIN_DECLS


void
_mongoc_counters_init (void);
void
_mongoc_counters_cleanup (void);


static BSON_INLINE unsigned
_mongoc_get_cpu_count (void)
{
#if defined(__linux__)
   return get_nprocs ();
#elif defined(__hpux__)
   struct pst_dynamic psd;

   if (pstat_getdynamic (&psd, sizeof (psd), (size_t) 1, 0) != -1) {
      return psd.psd_max_proc_cnt;
   }
   return 1;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
   defined(__OpenBSD__)
   int mib[2];
   int maxproc;
   size_t len;

   mib[0] = CTL_HW;
   mib[1] = HW_NCPU;
   len = sizeof (maxproc);

   if (-1 == sysctl (mib, 2, &maxproc, &len, NULL, 0)) {
      return 1;
   }

   return len;
#elif defined(__APPLE__) || defined(__sun)
   int ncpu;

   ncpu = (int) sysconf (_SC_NPROCESSORS_ONLN);
   return (ncpu > 0) ? ncpu : 1;
#elif defined(_MSC_VER) || defined(_WIN32)
   SYSTEM_INFO si;
   GetSystemInfo (&si);
   return si.dwNumberOfProcessors;
#else
#warning "_mongoc_get_cpu_count() not supported, defaulting to 1."
   return 1;
#endif
}


#define _mongoc_counter_add(v, count) bson_atomic_int64_add (&(v), (count))


#if defined(ENABLE_RDTSCP)
static BSON_INLINE unsigned
_mongoc_sched_getcpu (void)
{
   volatile uint32_t rax, rdx, rcx;
   __asm__ volatile("rdtscp\n" : "=a"(rax), "=d"(rdx), "=c"(rcx) : :);
   unsigned node_id, core_id;
   // node_id = (rcx & 0xFFF000)>>12;  // node_id is unused
   core_id = rcx & 0xFFF;
   return core_id;
}
#elif defined(HAVE_SCHED_GETCPU)
#define _mongoc_sched_getcpu sched_getcpu
#else
#define _mongoc_sched_getcpu() (0)
#endif


#ifndef SLOTS_PER_CACHELINE
#define SLOTS_PER_CACHELINE 8
#endif


typedef struct {
   int64_t slots[SLOTS_PER_CACHELINE];
} mongoc_counter_slots_t;


typedef struct {
   mongoc_counter_slots_t *cpus;
} mongoc_counter_t;


#define COUNTER(ident, Category, Name, Description) \
   extern mongoc_counter_t __mongoc_counter_##ident;
#include "mongoc-counters.defs"
#undef COUNTER


enum {
#define COUNTER(ident, Category, Name, Description) COUNTER_##ident,
#include "mongoc-counters.defs"
#undef COUNTER
   LAST_COUNTER
};


#define COUNTER(ident, Category, Name, Description)                   \
   static BSON_INLINE void mongoc_counter_##ident##_add (int64_t val) \
   {                                                                  \
      (void) _mongoc_counter_add (                                    \
         __mongoc_counter_##ident.cpus[_mongoc_sched_getcpu ()]       \
            .slots[COUNTER_##ident % SLOTS_PER_CACHELINE],            \
         val);                                                        \
   }                                                                  \
   static BSON_INLINE void mongoc_counter_##ident##_inc (void)        \
   {                                                                  \
      mongoc_counter_##ident##_add (1);                               \
   }                                                                  \
   static BSON_INLINE void mongoc_counter_##ident##_dec (void)        \
   {                                                                  \
      mongoc_counter_##ident##_add (-1);                              \
   }                                                                  \
   static BSON_INLINE void mongoc_counter_##ident##_reset (void)      \
   {                                                                  \
      uint32_t i;                                                     \
      for (i = 0; i < _mongoc_get_cpu_count (); i++) {                \
         __mongoc_counter_##ident.cpus[i]                             \
            .slots[COUNTER_##ident % SLOTS_PER_CACHELINE] = 0;        \
      }                                                               \
      bson_memory_barrier ();                                         \
   }
#include "mongoc-counters.defs"
#undef COUNTER


BSON_END_DECLS


#endif /* MONGOC_COUNTERS_PRIVATE_H */
