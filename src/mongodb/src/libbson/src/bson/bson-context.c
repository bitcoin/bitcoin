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

#include "bson-compat.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bson-atomic.h"
#include "bson-clock.h"
#include "bson-context.h"
#include "bson-context-private.h"
#include "bson-md5.h"
#include "bson-memory.h"
#include "bson-thread-private.h"

#ifdef BSON_HAVE_SYSCALL_TID
#include <sys/syscall.h>
#endif


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif


/*
 * Globals.
 */
static bson_context_t gContextDefault;


#ifdef BSON_HAVE_SYSCALL_TID
static uint16_t
gettid (void)
{
   return syscall (SYS_gettid);
}
#endif


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_host --
 *
 *       Retrieves the first three bytes of MD5(hostname) and assigns them
 *       to the host portion of oid.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_host (bson_context_t *context, /* IN */
                            bson_oid_t *oid)         /* OUT */
{
   uint8_t *bytes = (uint8_t *) oid;
   uint8_t digest[16];
   bson_md5_t md5;
   char hostname[HOST_NAME_MAX];

   BSON_ASSERT (context);
   BSON_ASSERT (oid);

   gethostname (hostname, sizeof hostname);
   hostname[HOST_NAME_MAX - 1] = '\0';

   bson_md5_init (&md5);
   bson_md5_append (
      &md5, (const uint8_t *) hostname, (uint32_t) strlen (hostname));
   bson_md5_finish (&md5, &digest[0]);

   bytes[4] = digest[0];
   bytes[5] = digest[1];
   bytes[6] = digest[2];
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_host_cached --
 *
 *       Fetch the cached copy of the MD5(hostname).
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_host_cached (bson_context_t *context, /* IN */
                                   bson_oid_t *oid)         /* OUT */
{
   BSON_ASSERT (context);
   BSON_ASSERT (oid);

   oid->bytes[4] = context->md5[0];
   oid->bytes[5] = context->md5[1];
   oid->bytes[6] = context->md5[2];
}


static BSON_INLINE uint16_t
_bson_getpid (void)
{
   uint16_t pid;
#ifdef BSON_OS_WIN32
   DWORD real_pid;

   real_pid = GetCurrentProcessId ();
   pid = (real_pid & 0xFFFF) ^ ((real_pid >> 16) & 0xFFFF);
#else
   pid = getpid ();
#endif

   return pid;
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_pid --
 *
 *       Initialize the pid field of @oid.
 *
 *       The pid field is 2 bytes, big-endian for memcmp().
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_pid (bson_context_t *context, /* IN */
                           bson_oid_t *oid)         /* OUT */
{
   uint16_t pid = _bson_getpid ();
   uint8_t *bytes = (uint8_t *) &pid;

   BSON_ASSERT (context);
   BSON_ASSERT (oid);

   pid = BSON_UINT16_TO_BE (pid);

   oid->bytes[7] = bytes[0];
   oid->bytes[8] = bytes[1];
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_pid_cached --
 *
 *       Fetch the cached copy of the current pid.
 *       This helps avoid multiple calls to getpid() which is slower
 *       on some systems.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_pid_cached (bson_context_t *context, /* IN */
                                  bson_oid_t *oid)         /* OUT */
{
   oid->bytes[7] = context->pidbe[0];
   oid->bytes[8] = context->pidbe[1];
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_seq32 --
 *
 *       32-bit sequence generator, non-thread-safe version.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_seq32 (bson_context_t *context, /* IN */
                             bson_oid_t *oid)         /* OUT */
{
   uint32_t seq = context->seq32++;

   seq = BSON_UINT32_TO_BE (seq);
   memcpy (&oid->bytes[9], ((uint8_t *) &seq) + 1, 3);
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_seq32_threadsafe --
 *
 *       Thread-safe version of 32-bit sequence generator.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_seq32_threadsafe (bson_context_t *context, /* IN */
                                        bson_oid_t *oid)         /* OUT */
{
   int32_t seq = bson_atomic_int_add (&context->seq32, 1);

   seq = BSON_UINT32_TO_BE (seq);
   memcpy (&oid->bytes[9], ((uint8_t *) &seq) + 1, 3);
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_seq64 --
 *
 *       64-bit oid sequence generator, non-thread-safe version.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_seq64 (bson_context_t *context, /* IN */
                             bson_oid_t *oid)         /* OUT */
{
   uint64_t seq;

   BSON_ASSERT (context);
   BSON_ASSERT (oid);

   seq = BSON_UINT64_TO_BE (context->seq64++);
   memcpy (&oid->bytes[4], &seq, sizeof (seq));
}


/*
 *--------------------------------------------------------------------------
 *
 * _bson_context_get_oid_seq64_threadsafe --
 *
 *       Thread-safe 64-bit sequence generator.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is modified.
 *
 *--------------------------------------------------------------------------
 */

static void
_bson_context_get_oid_seq64_threadsafe (bson_context_t *context, /* IN */
                                        bson_oid_t *oid)         /* OUT */
{
   int64_t seq = bson_atomic_int64_add (&context->seq64, 1);

   seq = BSON_UINT64_TO_BE (seq);
   memcpy (&oid->bytes[4], &seq, sizeof (seq));
}


static void
_bson_context_init (bson_context_t *context,    /* IN */
                    bson_context_flags_t flags) /* IN */
{
   struct timeval tv;
   uint16_t pid;
   unsigned int seed[3];
   unsigned int real_seed;
   bson_oid_t oid;

   context->flags = flags;
   context->oid_get_host = _bson_context_get_oid_host_cached;
   context->oid_get_pid = _bson_context_get_oid_pid_cached;
   context->oid_get_seq32 = _bson_context_get_oid_seq32;
   context->oid_get_seq64 = _bson_context_get_oid_seq64;

   /*
    * Generate a seed for our the random starting position of our increment
    * bytes. We mask off the last nibble so that the last digit of the OID will
    * start at zero. Just to be nice.
    *
    * The seed itself is made up of the current time in seconds, milliseconds,
    * and pid xored together. I welcome better solutions if at all necessary.
    */
   bson_gettimeofday (&tv);
   seed[0] = (unsigned int) tv.tv_sec;
   seed[1] = (unsigned int) tv.tv_usec;
   seed[2] = _bson_getpid ();
   real_seed = seed[0] ^ seed[1] ^ seed[2];

#ifdef BSON_OS_WIN32
   /* ms's runtime is multithreaded by default, so no rand_r */
   srand (real_seed);
   context->seq32 = rand () & 0x007FFFF0;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
   defined(__OpenBSD__)
   arc4random_buf (&context->seq32, sizeof (context->seq32));
   context->seq32 &= 0x007FFFF0;
#else
   context->seq32 = rand_r (&real_seed) & 0x007FFFF0;
#endif

   if ((flags & BSON_CONTEXT_DISABLE_HOST_CACHE)) {
      context->oid_get_host = _bson_context_get_oid_host;
   } else {
      _bson_context_get_oid_host (context, &oid);
      context->md5[0] = oid.bytes[4];
      context->md5[1] = oid.bytes[5];
      context->md5[2] = oid.bytes[6];
   }

   if ((flags & BSON_CONTEXT_THREAD_SAFE)) {
      context->oid_get_seq32 = _bson_context_get_oid_seq32_threadsafe;
      context->oid_get_seq64 = _bson_context_get_oid_seq64_threadsafe;
   }

   if ((flags & BSON_CONTEXT_DISABLE_PID_CACHE)) {
      context->oid_get_pid = _bson_context_get_oid_pid;
   } else {
#ifdef BSON_HAVE_SYSCALL_TID
      if ((flags & BSON_CONTEXT_USE_TASK_ID)) {
         int32_t tid;

         /* This call is always successful */
         tid = gettid ();
         pid = BSON_UINT16_TO_BE (tid);
      } else
#endif
      {
         pid = BSON_UINT16_TO_BE (_bson_getpid ());
      }

      memcpy (&context->pidbe[0], &pid, 2);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_context_new --
 *
 *       Initializes a new context with the flags specified.
 *
 *       In most cases, you want to call this with @flags set to
 *       BSON_CONTEXT_NONE.
 *
 *       If you are running on Linux, %BSON_CONTEXT_USE_TASK_ID can result
 *       in a healthy speedup for multi-threaded scenarios.
 *
 *       If you absolutely must have a single context for your application
 *       and use more than one thread, then %BSON_CONTEXT_THREAD_SAFE should
 *       be bitwise-or'd with your flags. This requires synchronization
 *       between threads.
 *
 *       If you expect your hostname to change often, you may consider
 *       specifying %BSON_CONTEXT_DISABLE_HOST_CACHE so that gethostname()
 *       is called for every OID generated. This is much slower.
 *
 *       If you expect your pid to change without notice, such as from an
 *       unexpected call to fork(), then specify
 *       %BSON_CONTEXT_DISABLE_PID_CACHE.
 *
 * Returns:
 *       A newly allocated bson_context_t that should be freed with
 *       bson_context_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_context_t *
bson_context_new (bson_context_flags_t flags)
{
   bson_context_t *context;

   context = bson_malloc0 (sizeof *context);
   _bson_context_init (context, flags);

   return context;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_context_destroy --
 *
 *       Cleans up a bson_context_t and releases any associated resources.
 *       This should be called when you are done using @context.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_context_destroy (bson_context_t *context) /* IN */
{
   if (context != &gContextDefault) {
      memset (context, 0, sizeof *context);
      bson_free (context);
   }
}


static BSON_ONCE_FUN (_bson_context_init_default)
{
   _bson_context_init (
      &gContextDefault,
      (BSON_CONTEXT_THREAD_SAFE | BSON_CONTEXT_DISABLE_PID_CACHE));
   BSON_ONCE_RETURN;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_context_get_default --
 *
 *       Fetches the default, thread-safe implementation of #bson_context_t.
 *       If you need faster generation, it is recommended you create your
 *       own #bson_context_t with bson_context_new().
 *
 * Returns:
 *       A shared instance to the default #bson_context_t. This should not
 *       be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_context_t *
bson_context_get_default (void)
{
   static bson_once_t once = BSON_ONCE_INIT;

   bson_once (&once, _bson_context_init_default);

   return &gContextDefault;
}
