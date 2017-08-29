/*
 * Copyright 2016 MongoDB, Inc.
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


#ifndef TEST_SUITE_H
#define TEST_SUITE_H


#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifndef BINARY_DIR
#define BINARY_DIR "tests/binary"
#endif


#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT BSON_ASSERT


#ifdef ASSERT_OR_PRINT
#undef ASSERT_OR_PRINT
#endif

#define ASSERT_OR_PRINT(_statement, _err)             \
   do {                                               \
      if (!(_statement)) {                            \
         fprintf (stderr,                             \
                  "FAIL:%s:%d  %s()\n  %s\n  %s\n\n", \
                  __FILE__,                           \
                  __LINE__,                           \
                  BSON_FUNC,                          \
                  #_statement,                        \
                  _err.message);                      \
         fflush (stderr);                             \
         abort ();                                    \
      }                                               \
   } while (0)


#define ASSERT_CMPINT_HELPER(a, eq, b, fmt)                        \
   do {                                                            \
      if (!((a) eq (b))) {                                         \
         fprintf (stderr,                                          \
                  "FAIL\n\nAssert Failure: %" fmt " %s %" fmt "\n" \
                  "%s:%d  %s()\n",                                 \
                  a,                                               \
                  #eq,                                             \
                  b,                                               \
                  __FILE__,                                        \
                  __LINE__,                                        \
                  BSON_FUNC);                                      \
         abort ();                                                 \
      }                                                            \
   } while (0)


#define ASSERT_CMPINT(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "d")
#define ASSERT_CMPUINT(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "u")
#define ASSERT_CMPLONG(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "ld")
#define ASSERT_CMPULONG(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "lu")
#define ASSERT_CMPINT32(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, PRId32)
#define ASSERT_CMPINT64(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, PRId64)
#define ASSERT_CMPUINT16(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "hu")
#define ASSERT_CMPUINT32(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, PRIu32)
#define ASSERT_CMPUINT64(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, PRIu64)
#define ASSERT_CMPSIZE_T(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "zd")
#define ASSERT_CMPSSIZE_T(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "zx")
#define ASSERT_CMPDOUBLE(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "f")

#define ASSERT_MEMCMP(a, b, n)                                       \
   do {                                                              \
      if (0 != memcmp (a, b, n)) {                                   \
         fprintf (stderr,                                            \
                  "Failed comparing %d bytes: \"%.*s\" != \"%.*s\"", \
                  n,                                                 \
                  n,                                                 \
                  (char *) a,                                        \
                  n,                                                 \
                  (char *) b);                                       \
         abort ();                                                   \
      }                                                              \
   } while (0)


#ifdef ASSERT_ALMOST_EQUAL
#undef ASSERT_ALMOST_EQUAL
#endif
#define ASSERT_ALMOST_EQUAL(a, b)                        \
   do {                                                  \
      /* evaluate once */                                \
      int64_t _a = (a);                                  \
      int64_t _b = (b);                                  \
      if (!(_a > (_b * 4) / 5 && (_a < (_b * 6) / 5))) { \
         fprintf (stderr,                                \
                  "FAIL\n\nAssert Failure: %" PRId64     \
                  " not within 20%% of %" PRId64 "\n"    \
                  "%s:%d  %s()\n",                       \
                  _a,                                    \
                  _b,                                    \
                  __FILE__,                              \
                  __LINE__,                              \
                  BSON_FUNC);                            \
         abort ();                                       \
      }                                                  \
   } while (0)


#define ASSERT_CMPSTR(a, b)                                                    \
   do {                                                                        \
      if (((a) != (b)) && !!strcmp ((a), (b))) {                               \
         fprintf (stderr, "FAIL\n\nAssert Failure: \"%s\" != \"%s\"\n", a, b); \
         abort ();                                                             \
      }                                                                        \
   } while (0)

#define ASSERT_CMPUINT8(a, b) ASSERT_CMPSTR ((const char *) a, (const char *) b)


#define ASSERT_CMPJSON(_a, _b)                                             \
   do {                                                                    \
      size_t i = 0;                                                        \
      char *a = (char *) _a;                                               \
      char *b = (char *) _b;                                               \
      char *aa = bson_malloc0 (strlen (a) + 1);                            \
      char *bb = bson_malloc0 (strlen (b) + 1);                            \
      char *f = a;                                                         \
      do {                                                                 \
         while (isspace (*a))                                              \
            a++;                                                           \
         aa[i++] = *a++;                                                   \
      } while (*a);                                                        \
      i = 0;                                                               \
      do {                                                                 \
         while (isspace (*b))                                              \
            b++;                                                           \
         bb[i++] = *b++;                                                   \
      } while (*b);                                                        \
      if (!!strcmp ((aa), (bb))) {                                         \
         fprintf (                                                         \
            stderr, "FAIL\n\nAssert Failure: \"%s\" != \"%s\"\n"           \
                  "%s:%d  %s()\n",                                         \
                  aa, bb,                                                  \
                  __FILE__, __LINE__, BSON_FUNC);                          \
         abort ();                                                         \
      }                                                                    \
      bson_free (aa);                                                      \
      bson_free (bb);                                                      \
      bson_free (f);                                                       \
   } while (0)


#define ASSERT_CMPOID(a, b)                                 \
   do {                                                     \
      if (bson_oid_compare ((a), (b))) {                    \
         char oid_a[25];                                    \
         char oid_b[25];                                    \
         bson_oid_to_string ((a), oid_a);                   \
         bson_oid_to_string ((b), oid_b);                   \
         fprintf (stderr,                                   \
                  "FAIL\n\nAssert Failure: "                \
                  "ObjectId(\"%s\") != ObjectId(\"%s\")\n", \
                  oid_a,                                    \
                  oid_b);                                   \
         abort ();                                          \
      }                                                     \
   } while (0)


#define ASSERT_CONTAINS(a, b)                                                 \
   do {                                                                       \
      if (NULL == strstr ((a), (b))) {                                        \
         fprintf (stderr,                                                     \
                  "FAIL\n\nAssert Failure: \"%s\" does not contain \"%s\"\n", \
                  a,                                                          \
                  b);                                                         \
         abort ();                                                            \
      }                                                                       \
   } while (0)

#define ASSERT_STARTSWITH(a, b)                                            \
   do {                                                                    \
      if ((a) != strstr ((a), (b))) {                                      \
         fprintf (                                                         \
            stderr,                                                        \
            "FAIL\n\nAssert Failure: \"%s\" does not start with \"%s\"\n", \
            a,                                                             \
            b);                                                            \
         abort ();                                                         \
      }                                                                    \
   } while (0)

#define AWAIT(_condition)                                               \
   do {                                                                 \
      int64_t _start = bson_get_monotonic_time ();                      \
      while (!(_condition)) {                                           \
         if (bson_get_monotonic_time () - _start > 1000 * 1000) {       \
            fprintf (stderr,                                            \
                     "%s:%d %s(): \"%s\" still false after 1 second\n", \
                     __FILE__,                                          \
                     __LINE__,                                          \
                     BSON_FUNC,                                         \
                     #_condition);                                      \
            abort ();                                                   \
         }                                                              \
      }                                                                 \
   } while (0)

#define ASSERT_ERROR_CONTAINS(error, _domain, _code, _message) \
   do {                                                        \
      ASSERT_CMPINT (error.domain, ==, _domain);               \
      ASSERT_CMPINT (error.code, ==, _code);                   \
      ASSERT_CONTAINS (error.message, _message);               \
   } while (0);

#define ASSERT_HAS_FIELD(_bson, _field)                                  \
   do {                                                                  \
      if (!bson_has_field ((_bson), (_field))) {                         \
         fprintf (stderr,                                                \
                  "FAIL\n\nAssert Failure: No field \"%s\" in \"%s\"\n", \
                  (_field),                                              \
                  bson_as_canonical_extended_json (_bson, NULL));        \
         abort ();                                                       \
      }                                                                  \
   } while (0)

/* don't check durations when testing with valgrind */
#define ASSERT_CMPTIME(actual, maxduration)      \
   do {                                          \
      if (!test_suite_valgrind ()) {             \
         ASSERT_CMPINT (actual, <, maxduration); \
      }                                          \
   } while (0)


#ifdef _WIN32
#define gettestpid _getpid
#else
#define gettestpid getpid
#endif

#define ASSERT_OR_PRINT_ERRNO(_statement, _errcode)                          \
   do {                                                                      \
      if (!(_statement)) {                                                   \
         fprintf (                                                           \
            stderr,                                                          \
            "FAIL:%s:%d  %s()\n  %s\n  Failed with error code: %d (%s)\n\n", \
            __FILE__,                                                        \
            __LINE__,                                                        \
            BSON_FUNC,                                                       \
            #_statement,                                                     \
            _errcode,                                                        \
            strerror (_errcode));                                            \
         fflush (stderr);                                                    \
         abort ();                                                           \
      }                                                                      \
   } while (0)

#define MAX_TEST_NAME_LENGTH 500


typedef void (*TestFunc) (void);
typedef void (*TestFuncWC) (void *);
typedef void (*TestFuncDtor) (void *);
typedef struct _Test Test;
typedef struct _TestSuite TestSuite;


struct _Test {
   Test *next;
   char *name;
   TestFuncWC func;
   TestFuncDtor dtor;
   void *ctx;
   int exit_code;
   unsigned seed;
   int (*check) (void);
};


struct _TestSuite {
   char *prgname;
   char *name;
   char *testname;
   Test *tests;
   FILE *outfile;
   int flags;
};


void
TestSuite_Init (TestSuite *suite, const char *name, int argc, char **argv);
void
TestSuite_Add (TestSuite *suite, const char *name, TestFunc func);
void
TestSuite_AddWC (TestSuite *suite,
                 const char *name,
                 TestFuncWC func,
                 TestFuncDtor dtor,
                 void *ctx);
void
TestSuite_AddFull (TestSuite *suite,
                   const char *name,
                   TestFuncWC func,
                   TestFuncDtor dtor,
                   void *ctx,
                   int (*check) (void));
int
TestSuite_Run (TestSuite *suite);
void
TestSuite_Destroy (TestSuite *suite);

int
test_suite_debug_output (void);
int
test_suite_valgrind (void);

#ifdef __cplusplus
}
#endif


#endif /* TEST_SUITE_H */
