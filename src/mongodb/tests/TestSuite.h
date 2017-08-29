/*
 * Copyright 2014 MongoDB, Inc.
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
#include <math.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS_RELEASE_FILE_DIR
#define OS_RELEASE_FILE_DIR "tests/release_files"
#endif


#ifndef BINARY_DIR
#define BINARY_DIR "tests/binary"
#endif

#ifndef JSON_DIR
#define JSON_DIR "tests/json"
#endif

#ifndef CERT_TEST_DIR
#define CERT_TEST_DIR "tests/x509gen"
#endif

#define CERT_CA CERT_TEST_DIR "/ca.pem"
#define CERT_CRL CERT_TEST_DIR "/crl.pem"
#define CERT_SERVER CERT_TEST_DIR "/server.pem" /* 127.0.0.1 & localhost */
#define CERT_CLIENT CERT_TEST_DIR "/client.pem"
#define CERT_ALTNAME                                                   \
   CERT_TEST_DIR "/altname.pem"             /* alternative.mongodb.org \
                                               */
#define CERT_WILD CERT_TEST_DIR "/wild.pem" /* *.mongodb.org */
#define CERT_COMMONNAME \
   CERT_TEST_DIR "/commonName.pem"                /* 127.0.0.1 & localhost */
#define CERT_EXPIRED CERT_TEST_DIR "/expired.pem" /* 127.0.0.1 & localhost */
#define CERT_PASSWORD "qwerty"
#define CERT_PASSWORD_PROTECTED CERT_TEST_DIR "/password_protected.pem"


void
test_error (const char *format, ...) BSON_GNUC_PRINTF (1, 2);

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

#define ASSERT_CURSOR_NEXT(_cursor, _doc)              \
   do {                                                \
      bson_error_t _err;                               \
      if (!mongoc_cursor_next ((_cursor), (_doc))) {   \
         if (mongoc_cursor_error ((_cursor), &_err)) { \
            fprintf (stderr,                           \
                     "FAIL:%s:%d  %s()\n  %s\n\n",     \
                     __FILE__,                         \
                     __LINE__,                         \
                     BSON_FUNC,                        \
                     _err.message);                    \
         } else {                                      \
            fprintf (stderr,                           \
                     "FAIL:%s:%d  %s()\n  %s\n\n",     \
                     __FILE__,                         \
                     __LINE__,                         \
                     BSON_FUNC,                        \
                     "empty cursor");                  \
         }                                             \
         fflush (stderr);                              \
         abort ();                                     \
      }                                                \
   } while (0)


#define ASSERT_CURSOR_DONE(_cursor)                 \
   do {                                             \
      bson_error_t _err;                            \
      const bson_t *_doc;                           \
      if (mongoc_cursor_next ((_cursor), &_doc)) {  \
         fprintf (stderr,                           \
                  "FAIL:%s:%d  %s()\n  %s\n\n",     \
                  __FILE__,                         \
                  __LINE__,                         \
                  BSON_FUNC,                        \
                  "non-empty cursor");              \
         fflush (stderr);                           \
         abort ();                                  \
      }                                             \
      if (mongoc_cursor_error ((_cursor), &_err)) { \
         fprintf (stderr,                           \
                  "FAIL:%s:%d  %s()\n  %s\n\n",     \
                  __FILE__,                         \
                  __LINE__,                         \
                  BSON_FUNC,                        \
                  _err.message);                    \
         fflush (stderr);                           \
         abort ();                                  \
      }                                             \
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
#define ASSERT_CMPVOID(a, eq, b) ASSERT_CMPINT_HELPER (a, eq, b, "p")

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
      if (!(_a > (_b * 2) / 3 && (_a < (_b * 3) / 2))) { \
         fprintf (stderr,                                \
                  "FAIL\n\nAssert Failure: %" PRId64     \
                  " not within 50%% of %" PRId64 "\n"    \
                  "%s:%d  %s()\n",                       \
                  _a,                                    \
                  _b,                                    \
                  __FILE__,                              \
                  __LINE__,                              \
                  BSON_FUNC);                            \
         abort ();                                       \
      }                                                  \
   } while (0)

#ifdef ASSERT_EQUAL_DOUBLE
#undef ASSERT_EQUAL_DOUBLE
#endif
#define ASSERT_EQUAL_DOUBLE(a, b)                                      \
   do {                                                                \
      double _a = fabs ((double) a);                                   \
      double _b = fabs ((double) b);                                   \
      if (!(_a > (_b * 4) / 5 && (_a < (_b * 6) / 5))) {               \
         fprintf (stderr,                                              \
                  "FAIL\n\nAssert Failure: %f not within 20%% of %f\n" \
                  "%s:%d  %s()\n",                                     \
                  (double) a,                                          \
                  (double) b,                                          \
                  __FILE__,                                            \
                  __LINE__,                                            \
                  BSON_FUNC);                                          \
         abort ();                                                     \
      }                                                                \
   } while (0)


#define ASSERT_CMPSTR(a, b)                                                    \
   do {                                                                        \
      if (((a) != (b)) && !!strcmp ((a), (b))) {                               \
         fprintf (stderr, "FAIL\n\nAssert Failure: \"%s\" != \"%s\"\n", a, b); \
         abort ();                                                             \
      }                                                                        \
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


#define ASSERT_CONTAINS(a, b)                                        \
   do {                                                              \
      if (NULL == strstr ((a), (b))) {                               \
         fprintf (stderr,                                            \
                  "%s:%d %s(): : [%s] does not contain with [%s]\n", \
                  __FILE__,                                          \
                  __LINE__,                                          \
                  BSON_FUNC,                                         \
                  a,                                                 \
                  b);                                                \
         abort ();                                                   \
      }                                                              \
   } while (0)

#define ASSERT_STARTSWITH(a, b)                                    \
   do {                                                            \
      if ((a) != strstr ((a), (b))) {                              \
         fprintf (stderr,                                          \
                  "%s:%d %s(): : [%s] does not start with [%s]\n", \
                  __FILE__,                                        \
                  __LINE__,                                        \
                  BSON_FUNC,                                       \
                  a,                                               \
                  b);                                              \
         abort ();                                                 \
      }                                                            \
   } while (0)

#define ASSERT_ERROR_CONTAINS(error, _domain, _code, _message)               \
   do {                                                                      \
      if (error.domain != _domain) {                                         \
         fprintf (stderr,                                                    \
                  "%s:%d %s(): error domain %d doesn't match expected %d\n", \
                  __FILE__,                                                  \
                  __LINE__,                                                  \
                  BSON_FUNC,                                                 \
                  error.domain,                                              \
                  _domain);                                                  \
         abort ();                                                           \
      };                                                                     \
      if (error.code != _code) {                                             \
         fprintf (stderr,                                                    \
                  "%s:%d %s(): error code %d doesn't match expected %d\n",   \
                  __FILE__,                                                  \
                  __LINE__,                                                  \
                  BSON_FUNC,                                                 \
                  error.code,                                                \
                  _code);                                                    \
         abort ();                                                           \
      };                                                                     \
      ASSERT_CONTAINS (error.message, _message);                             \
   } while (0);

#define ASSERT_CAPTURED_LOG(_info, _level, _msg)                \
   do {                                                         \
      if (!has_captured_log (_level, _msg)) {                   \
         fprintf (stderr,                                       \
                  "%s:%d %s(): testing %s didn't log \"%s\"\n", \
                  __FILE__,                                     \
                  __LINE__,                                     \
                  BSON_FUNC,                                    \
                  _info,                                        \
                  _msg);                                        \
         print_captured_logs ("\t");                            \
         abort ();                                              \
      }                                                         \
   } while (0);

#define ASSERT_NO_CAPTURED_LOGS(_info)                               \
   do {                                                              \
      if (has_captured_logs ()) {                                    \
         fprintf (stderr,                                            \
                  "%s:%d %s(): testing %s shouldn't have logged:\n", \
                  __FILE__,                                          \
                  __LINE__,                                          \
                  BSON_FUNC,                                         \
                  _info);                                            \
         print_captured_logs ("\t");                                 \
         abort ();                                                   \
      }                                                              \
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

#define ASSERT_HAS_NOT_FIELD(_bson, _field)                                \
   do {                                                                    \
      if (bson_has_field ((_bson), (_field))) {                            \
         fprintf (                                                         \
            stderr,                                                        \
            "FAIL\n\nAssert Failure: Unexpected field \"%s\" in \"%s\"\n", \
            (_field),                                                      \
            bson_as_canonical_extended_json (_bson, NULL));                \
         abort ();                                                         \
      }                                                                    \
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

#define ASSERT_COUNT(n, collection)                                     \
   do {                                                                 \
      int count = (int) mongoc_collection_count (                       \
         collection, MONGOC_QUERY_NONE, NULL, 0, 0, NULL, NULL);        \
      if ((n) != count) {                                               \
         fprintf (stderr,                                               \
                  "FAIL\n\nAssert Failure: count of %s is %d, not %d\n" \
                  "%s:%d  %s()\n",                                      \
                  mongoc_collection_get_name (collection),              \
                  count,                                                \
                  n,                                                    \
                  __FILE__,                                             \
                  __LINE__,                                             \
                  BSON_FUNC);                                           \
         abort ();                                                      \
      }                                                                 \
   } while (0)

#define ASSERT_CURSOR_COUNT(_n, _cursor)                                 \
   do {                                                                  \
      int _count = 0;                                                    \
      const bson_t *_doc;                                                \
      while (mongoc_cursor_next (_cursor, &_doc)) {                      \
         _count++;                                                       \
      }                                                                  \
      if ((_n) != _count) {                                              \
         fprintf (stderr,                                                \
                  "FAIL\n\nAssert Failure: cursor count is %d, not %d\n" \
                  "%s:%d  %s()\n",                                       \
                  _count,                                                \
                  _n,                                                    \
                  __FILE__,                                              \
                  __LINE__,                                              \
                  BSON_FUNC);                                            \
         abort ();                                                       \
      }                                                                  \
   } while (0)

#define WAIT_UNTIL(_pred)                                              \
   do {                                                                \
      int64_t _start = bson_get_monotonic_time ();                     \
      while (!(_pred)) {                                               \
         if (bson_get_monotonic_time () - _start > 10 * 1000 * 1000) { \
            fprintf (stderr,                                           \
                     "Predicate \"%s\" timed out\n"                    \
                     "   %s:%d  %s()\n",                               \
                     #_pred,                                           \
                     __FILE__,                                         \
                     __LINE__,                                         \
                     BSON_FUNC);                                       \
            abort ();                                                  \
         }                                                             \
      }                                                                \
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
   int silent;
   bson_string_t *mock_server_log_buf;
   FILE *mock_server_log;
};


void
TestSuite_Init (TestSuite *suite, const char *name, int argc, char **argv);
void
TestSuite_Add (TestSuite *suite, const char *name, TestFunc func);
int
TestSuite_CheckLive (void);
void
TestSuite_AddLive (TestSuite *suite, const char *name, TestFunc func);
int
TestSuite_CheckMockServerAllowed (void);
void
TestSuite_AddMockServerTest (TestSuite *suite, const char *name, TestFunc func);
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
void
test_suite_mock_server_log (const char *msg, ...);

#ifdef __cplusplus
}
#endif


#endif /* TEST_SUITE_H */
