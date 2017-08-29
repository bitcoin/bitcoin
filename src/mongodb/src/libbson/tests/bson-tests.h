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


#ifndef BSON_TESTS_H
#define BSON_TESTS_H


#include <bson.h>
#include <stdio.h>
#include <time.h>


BSON_BEGIN_DECLS


#define BSON_ASSERT_CMPSTR(a, b)                                          \
   do {                                                                   \
      if (((a) != (b)) && !!strcmp ((a), (b))) {                          \
         fprintf (stderr,                                                 \
                  "FAIL\n\nAssert Failure: (line#%d) \"%s\" != \"%s\"\n", \
                  __LINE__,                                               \
                  a,                                                      \
                  b);                                                     \
         abort ();                                                        \
      }                                                                   \
   } while (0)


#define BSON_ASSERT_CMPINT(a, eq, b)                                          \
   do {                                                                       \
      if (!((a) eq (b))) {                                                    \
         fprintf (stderr,                                                     \
                  "FAIL\n\nAssert Failure: (line#%d)" #a " " #eq " " #b "\n", \
                  __LINE__);                                                  \
         abort ();                                                            \
      }                                                                       \
   } while (0)


#ifdef BSON_OS_WIN32
#include <stdarg.h>
#include <share.h>
static __inline int
bson_open (const char *filename, int flags, ...)
{
   int fd = -1;
   int mode = 0;

   if (_sopen_s (
          &fd, filename, flags | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE) ==
       NO_ERROR) {
      return fd;
   }

   return -1;
}
#define bson_close _close
#define bson_read(f, b, c) ((ssize_t) _read ((f), (b), (int) (c)))
#define bson_write _write
#else
#define bson_open open
#define bson_read read
#define bson_close close
#define bson_write write
#endif


#define bson_eq_bson(bson, expected)                                          \
   do {                                                                       \
      char *bson_json, *expected_json;                                        \
      const uint8_t *bson_data = bson_get_data ((bson));                      \
      const uint8_t *expected_data = bson_get_data ((expected));              \
      int unequal;                                                            \
      unsigned o;                                                             \
      int off = -1;                                                           \
      unequal = ((expected)->len != (bson)->len) ||                           \
                memcmp (bson_get_data ((expected)),                           \
                        bson_get_data ((bson)),                               \
                        (expected)->len);                                     \
      if (unequal) {                                                          \
         bson_json = bson_as_canonical_extended_json (bson, NULL);            \
         expected_json = bson_as_canonical_extended_json ((expected), NULL);  \
         for (o = 0; o < (bson)->len && o < (expected)->len; o++) {           \
            if (bson_data[o] != expected_data[o]) {                           \
               off = o;                                                       \
               break;                                                         \
            }                                                                 \
         }                                                                    \
         if (off == -1) {                                                     \
            off = BSON_MAX ((expected)->len, (bson)->len) - 1;                \
         }                                                                    \
         fprintf (stderr,                                                     \
                  "bson objects unequal (byte %u):\n(%s)\n(%s)\n",            \
                  off,                                                        \
                  bson_json,                                                  \
                  expected_json);                                             \
         {                                                                    \
            int fd1 = bson_open ("failure.bad.bson", O_RDWR | O_CREAT, 0640); \
            int fd2 =                                                         \
               bson_open ("failure.expected.bson", O_RDWR | O_CREAT, 0640);   \
            BSON_ASSERT (fd1 != -1);                                               \
            BSON_ASSERT (fd2 != -1);                                               \
            BSON_ASSERT ((bson)->len == bson_write (fd1, bson_data, (bson)->len)); \
            BSON_ASSERT ((expected)->len ==                                        \
                    bson_write (fd2, expected_data, (expected)->len));        \
            bson_close (fd1);                                                 \
            bson_close (fd2);                                                 \
         }                                                                    \
         BSON_ASSERT (0);                                                          \
      }                                                                       \
   } while (0)


static BSON_INLINE void
run_test (const char *name, void (*func) (void))
{
   struct timeval begin;
   struct timeval end;
   struct timeval diff;
   long usec;
   double format;

   fprintf (stdout, "%-42s : ", name);
   fflush (stdout);
   bson_gettimeofday (&begin);
   func ();
   bson_gettimeofday (&end);
   fprintf (stdout, "PASS");

   diff.tv_sec = end.tv_sec - begin.tv_sec;
   diff.tv_usec = usec = end.tv_usec - begin.tv_usec;
   if (usec < 0) {
      diff.tv_sec -= 1;
      diff.tv_usec = usec + 1000000;
   }

   format = diff.tv_sec + (diff.tv_usec / 1000000.0);
   fprintf (stdout, " : %lf\n", format);
}


BSON_END_DECLS

#endif /* BSON_TESTS_H */
