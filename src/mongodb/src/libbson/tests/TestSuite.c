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

#include <bson.h>

#include <fcntl.h>
#include <stdarg.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <sys/types.h>
#include <inttypes.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#endif

#if defined(BSON_HAVE_CLOCK_GETTIME)
#include <time.h>
#include <sys/time.h>
#endif

#include "TestSuite.h"


static int test_flags;

const int MAX_THREADS = 10;


#define TEST_VERBOSE (1 << 0)
#define TEST_NOFORK (1 << 1)
#define TEST_HELPONLY (1 << 2)
#define TEST_THREADS (1 << 3)
#define TEST_DEBUGOUTPUT (1 << 4)
#define TEST_VALGRIND (1 << 5)


#define NANOSEC_PER_SEC 1000000000UL


#if !defined(_WIN32)
#include <pthread.h>
#define Mutex pthread_mutex_t
#define Mutex_Init(_n) pthread_mutex_init ((_n), NULL)
#define Mutex_Lock pthread_mutex_lock
#define Mutex_Unlock pthread_mutex_unlock
#define Mutex_Destroy pthread_mutex_destroy
#define Thread pthread_t
#define Thread_Create(_t, _f, _d) pthread_create ((_t), NULL, (_f), (_d))
#define Thread_Join(_n) pthread_join ((_n), NULL)
#else
#define Mutex CRITICAL_SECTION
#define Mutex_Init InitializeCriticalSection
#define Mutex_Lock EnterCriticalSection
#define Mutex_Unlock LeaveCriticalSection
#define Mutex_Destroy DeleteCriticalSection
#define Thread HANDLE
#define Thread_Join(_n) WaitForSingleObject ((_n), INFINITE)
#define strdup _strdup
#endif


#if !defined(Memory_Barrier)
#define Memory_Barrier() bson_memory_barrier ()
#endif


#define AtomicInt_DecrementAndTest(p) (bson_atomic_int_add (p, -1) == 0)


#if !defined(BSON_HAVE_TIMESPEC)
struct timespec {
   time_t tv_sec;
   long tv_nsec;
};
#endif


#if defined(_WIN32)
static int
Thread_Create (Thread *thread, void *(*cb) (void *), void *arg)
{
   *thread = CreateThread (NULL, 0, (void *) cb, arg, 0, NULL);
   return 0;
}
#endif


#if defined(_WIN32) && !defined(BSON_HAVE_SNPRINTF)
static int
snprintf (char *str, size_t size, const char *format, ...)
{
   int r = -1;
   va_list ap;

   va_start (ap, format);
   if (size != 0) {
      r = _vsnprintf_s (str, size, _TRUNCATE, format, ap);
   }
   if (r == -1) {
      r = _vscprintf (format, ap);
   }
   va_end (ap);

   return r;
}
#endif


void
_Print_StdOut (const char *format, ...)
{
   va_list ap;

   va_start (ap, format);
   vprintf (format, ap);
   fflush (stdout);
   va_end (ap);
}


void
_Print_StdErr (const char *format, ...)
{
   va_list ap;

   va_start (ap, format);
   vfprintf (stderr, format, ap);
   fflush (stderr);
   va_end (ap);
}


void
_Clock_GetMonotonic (struct timespec *ts) /* OUT */
{
#if defined(BSON_HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
   clock_gettime (CLOCK_MONOTONIC, ts);
#elif defined(__APPLE__)
   static mach_timebase_info_data_t info = {0};
   static double ratio;
   uint64_t atime;

   if (!info.denom) {
      mach_timebase_info (&info);
      ratio = info.numer / info.denom;
   }

   atime = mach_absolute_time () * ratio;
   ts->tv_sec = atime * 1e-9;
   ts->tv_nsec = atime - (ts->tv_sec * 1e9);
#elif defined(_WIN32)
   /* GetTickCount64() returns milliseconds */
   ULONGLONG ticks = GetTickCount64 ();
   ts->tv_sec = ticks / 1000;

   /* milliseconds -> microseconds -> nanoseconds*/
   ts->tv_nsec = (ticks % 1000) * 1000 * 1000;
#elif defined(__hpux__)
   uint64_t nsec = gethrtime ();

   ts->tv_sec = (int64_t) (nsec / 1e9);
   ts->tv_nsec = (int32_t) (nsec - (double) ts->tv_sec * 1e9);
#else
#warning "Monotonic clock is not yet supported on your platform."
#endif
}


void
_Clock_Subtract (struct timespec *ts, /* OUT */
                 struct timespec *x,  /* IN */
                 struct timespec *y)  /* IN */
{
   struct timespec r;

   ASSERT (x);
   ASSERT (y);

   r.tv_sec = (x->tv_sec - y->tv_sec);

   if ((r.tv_nsec = (x->tv_nsec - y->tv_nsec)) < 0) {
      r.tv_nsec += NANOSEC_PER_SEC;
      r.tv_sec -= 1;
   }

   *ts = r;
}


static void
TestSuite_SeedRand (TestSuite *suite, /* IN */
                    Test *test)       /* IN */
{
#ifndef BSON_OS_WIN32
   int fd = open ("/dev/urandom", O_RDONLY);
   int n_read;
   unsigned seed;
   if (fd != -1) {
      n_read = read (fd, &seed, 4);
      BSON_ASSERT (n_read == 4);
      close (fd);
      test->seed = seed;
      return;
   } else {
      test->seed = (unsigned) time (NULL) * (unsigned) getpid ();
   }
#else
   test->seed = (unsigned) time (NULL);
#endif
}


void
TestSuite_Init (TestSuite *suite, const char *name, int argc, char **argv)
{
   const char *filename;
   int i;

   memset (suite, 0, sizeof *suite);

   suite->name = bson_strdup (name);
   suite->flags = 0;
   suite->prgname = bson_strdup (argv[0]);

   for (i = 0; i < argc; i++) {
      if (0 == strcmp ("-v", argv[i])) {
         suite->flags |= TEST_VERBOSE;
      } else if (0 == strcmp ("-d", argv[i])) {
         suite->flags |= TEST_DEBUGOUTPUT;
      } else if (0 == strcmp ("--no-fork", argv[i])) {
         suite->flags |= TEST_NOFORK;
      } else if (0 == strcmp ("--threads", argv[i])) {
         suite->flags |= TEST_THREADS;
      } else if (0 == strcmp ("-F", argv[i])) {
         if (argc - 1 == i) {
            _Print_StdErr ("-F requires a filename argument.\n");
            exit (EXIT_FAILURE);
         }
         filename = argv[++i];
         if (0 != strcmp ("-", filename)) {
#ifdef _WIN32
            if (0 != fopen_s (&suite->outfile, filename, "w")) {
               suite->outfile = NULL;
            }
#else
            suite->outfile = fopen (filename, "w");
#endif
            if (!suite->outfile) {
               _Print_StdErr ("Failed to open log file: %s\n", filename);
            }
         }
      } else if ((0 == strcmp ("-h", argv[i])) ||
                 (0 == strcmp ("--help", argv[i]))) {
         suite->flags |= TEST_HELPONLY;
      } else if ((0 == strcmp ("-l", argv[i]))) {
         if (argc - 1 == i) {
            _Print_StdErr ("-l requires an argument.\n");
            exit (EXIT_FAILURE);
         }
         suite->testname = bson_strdup (argv[++i]);
      }
   }

   /* HACK: copy flags to global var */
   test_flags = suite->flags;
}


static int
TestSuite_CheckDummy (void)
{
   return 1;
}

static void
TestSuite_AddHelper (void *cb_)
{
   TestFunc cb = (TestFunc) cb_;

   cb ();
}

void
TestSuite_Add (TestSuite *suite, /* IN */
               const char *name, /* IN */
               TestFunc func)    /* IN */
{
   TestSuite_AddFull (suite,
                      name,
                      TestSuite_AddHelper,
                      NULL,
                      (void *) func,
                      TestSuite_CheckDummy);
}


void
TestSuite_AddWC (TestSuite *suite,  /* IN */
                 const char *name,  /* IN */
                 TestFuncWC func,   /* IN */
                 TestFuncDtor dtor, /* IN */
                 void *ctx)         /* IN */
{
   TestSuite_AddFull (suite, name, func, dtor, ctx, TestSuite_CheckDummy);
}


void
TestSuite_AddFull (TestSuite *suite,  /* IN */
                   const char *name,  /* IN */
                   TestFuncWC func,   /* IN */
                   TestFuncDtor dtor, /* IN */
                   void *ctx,
                   int (*check) (void)) /* IN */
{
   Test *test;
   Test *iter;

   test = (Test *) calloc (1, sizeof *test);
   test->name = bson_strdup (name);
   test->func = func;
   test->check = check;
   test->next = NULL;
   test->dtor = dtor;
   test->ctx = ctx;
   TestSuite_SeedRand (suite, test);

   if (!suite->tests) {
      suite->tests = test;
      return;
   }

   for (iter = suite->tests; iter->next; iter = iter->next) {
   }

   iter->next = test;
}


#if !defined(_WIN32)
static int
TestSuite_RunFuncInChild (TestSuite *suite, /* IN */
                          Test *test)       /* IN */
{
   pid_t child;
   int exit_code = -1;
   int fd;

   if (suite->outfile) {
      fflush (suite->outfile);
   }

   if (-1 == (child = fork ())) {
      return -1;
   }

   if (!child) {
      if (suite->outfile) {
         fclose (suite->outfile);
         suite->outfile = NULL;
      }
      fd = open ("/dev/null", O_WRONLY);
      dup2 (fd, STDOUT_FILENO);
      close (fd);
      srand (test->seed);
      test->func (test->ctx);

      TestSuite_Destroy (suite);
      exit (0);
   }

   if (-1 == waitpid (child, &exit_code, 0)) {
      perror ("waitpid()");
   }

   return exit_code;
}
#endif


static int
TestSuite_RunTest (TestSuite *suite, /* IN */
                   Test *test,       /* IN */
                   Mutex *mutex,     /* IN */
                   int *count)       /* INOUT */
{
   struct timespec ts1;
   struct timespec ts2;
   struct timespec ts3;
   char name[MAX_TEST_NAME_LENGTH];
   char buf[MAX_TEST_NAME_LENGTH + 500];
   int status = 0;

   snprintf (name, sizeof name, "%s%s", suite->name, test->name);
   name[sizeof name - 1] = '\0';

   if (!test->check || test->check ()) {
      _Clock_GetMonotonic (&ts1);

/*
 * TODO: If not verbose, close()/dup(/dev/null) for stdout.
 */

/* Tracing is superduper slow */
#if defined(_WIN32)
      srand (test->seed);

      if (suite->flags & TEST_DEBUGOUTPUT) {
         _Print_StdOut ("Begin %s, seed %u\n", name, test->seed);
      }

      test->func (test->ctx);
      status = 0;
#else
      if (suite->flags & TEST_DEBUGOUTPUT) {
         _Print_StdOut ("Begin %s, seed %u\n", name, test->seed);
      }

      if ((suite->flags & TEST_NOFORK)) {
         srand (test->seed);
         test->func (test->ctx);
         status = 0;
      } else {
         status = TestSuite_RunFuncInChild (suite, test);
      }
#endif

      _Clock_GetMonotonic (&ts2);
      _Clock_Subtract (&ts3, &ts2, &ts1);

      Mutex_Lock (mutex);
      snprintf (buf,
                sizeof buf,
                "    { \"status\": \"%s\", "
                "\"test_file\": \"%s\", "
                "\"seed\": \"%u\", "
                "\"start\": %u.%09u, "
                "\"end\": %u.%09u, "
                "\"elapsed\": %u.%09u }%s\n",
                (status == 0) ? "PASS" : "FAIL",
                name,
                test->seed,
                (unsigned) ts1.tv_sec,
                (unsigned) ts1.tv_nsec,
                (unsigned) ts2.tv_sec,
                (unsigned) ts2.tv_nsec,
                (unsigned) ts3.tv_sec,
                (unsigned) ts3.tv_nsec,
                ((*count) == 1) ? "" : ",");
      buf[sizeof buf - 1] = 0;
      _Print_StdOut ("%s", buf);
      if (suite->outfile) {
         fprintf (suite->outfile, "%s", buf);
         fflush (suite->outfile);
      }
      Mutex_Unlock (mutex);
   } else {
      status = 0;
      Mutex_Lock (mutex);
      snprintf (buf,
                sizeof buf,
                "    { \"status\": \"SKIP\", \"test_file\": \"%s\" },\n",
                test->name);
      buf[sizeof buf - 1] = '\0';
      _Print_StdOut ("%s", buf);
      if (suite->outfile) {
         fprintf (suite->outfile, "%s", buf);
         fflush (suite->outfile);
      }
      Mutex_Unlock (mutex);
   }

   return status ? 1 : 0;
}


static void
TestSuite_PrintHelp (TestSuite *suite, /* IN */
                     FILE *stream)     /* IN */
{
   Test *iter;

   fprintf (stream,
            "usage: %s [OPTIONS]\n"
            "\n"
            "Options:\n"
            "    -h, --help   Show this help menu.\n"
            "    -f           Do not fork() before running tests.\n"
            "    -l NAME      Run test by name, e.g. \"/Client/command\" or "
            "\"/Client/*\".\n"
            "    -p           Do not run tests in parallel.\n"
            "    -v           Be verbose with logs.\n"
            "    -F FILENAME  Write test results (JSON) to FILENAME.\n"
            "    -d           Print debug output (useful if a test hangs).\n"
            "\n"
            "Tests:\n",
            suite->prgname);

   for (iter = suite->tests; iter; iter = iter->next) {
      fprintf (stream, "    %s%s\n", suite->name, iter->name);
   }

   fprintf (stream, "\n");
}


static void
TestSuite_PrintJsonSystemHeader (FILE *stream)
{
#ifdef _WIN32
#define INFO_BUFFER_SIZE 32767

   SYSTEM_INFO si;
   DWORD version = 0;
   DWORD major_version = 0;
   DWORD minor_version = 0;
   DWORD build = 0;

   GetSystemInfo (&si);
   version = GetVersion ();

   major_version = (DWORD) (LOBYTE (LOWORD (version)));
   minor_version = (DWORD) (HIBYTE (LOWORD (version)));

   if (version < 0x80000000) {
      build = (DWORD) (HIWORD (version));
   }

   fprintf (stream,
            "  \"host\": {\n"
            "    \"sysname\": \"Windows\",\n"
            "    \"release\": \"%ld.%ld (%ld)\",\n"
            "    \"machine\": \"%ld\",\n"
            "    \"memory\": {\n"
            "      \"pagesize\": %ld,\n"
            "      \"npages\": %d\n"
            "    }\n"
            "  },\n",
            major_version,
            minor_version,
            build,
            si.dwProcessorType,
            si.dwPageSize,
            0);
#else
   struct utsname u;
   uint64_t pagesize;
   uint64_t npages = 0;

   if (uname (&u) == -1) {
      perror ("uname()");
      return;
   }

   pagesize = sysconf (_SC_PAGE_SIZE);

#if defined(_SC_PHYS_PAGES)
   npages = sysconf (_SC_PHYS_PAGES);
#endif
   fprintf (stream,
            "  \"host\": {\n"
            "    \"sysname\": \"%s\",\n"
            "    \"release\": \"%s\",\n"
            "    \"machine\": \"%s\",\n"
            "    \"memory\": {\n"
            "      \"pagesize\": %" PRIu64 ",\n"
            "      \"npages\": %" PRIu64 "\n"
            "    }\n"
            "  },\n",
            u.sysname,
            u.release,
            u.machine,
            pagesize,
            npages);
#endif
}

static void
TestSuite_PrintJsonHeader (TestSuite *suite, /* IN */
                           FILE *stream)     /* IN */
{
   ASSERT (suite);

   fprintf (stream, "{\n");
   TestSuite_PrintJsonSystemHeader (stream);
   fprintf (stream,
            "  \"options\": {\n"
            "    \"parallel\": %s,\n"
            "    \"fork\": %s\n"
            "  },\n"
            "  \"results\": [\n",
            (suite->flags & TEST_THREADS) ? "true" : "false",
            (suite->flags & TEST_NOFORK) ? "false" : "true");

   fflush (stream);
}


static void
TestSuite_PrintJsonFooter (FILE *stream) /* IN */
{
   fprintf (stream, "  ]\n}\n");
   fflush (stream);
}


typedef struct {
   TestSuite *suite;
   Test *test;
   int n_tests;
   Mutex *mutex;
   int *count;
} ParallelInfo;


static void *
TestSuite_ParallelWorker (void *data) /* IN */
{
   ParallelInfo *info = (ParallelInfo *) data;
   Test *test;
   int i;
   int status;

   ASSERT (info);
   test = info->test;

   /* there's MAX_THREADS threads, each runs n_tests tests */
   for (i = 0; i < info->n_tests; i++) {
      ASSERT (test);
      status = TestSuite_RunTest (info->suite, test, info->mutex, info->count);
      if (AtomicInt_DecrementAndTest (info->count)) {
         TestSuite_PrintJsonFooter (stdout);
         if (info->suite->outfile) {
            TestSuite_PrintJsonFooter (info->suite->outfile);
         }
         exit (status);
      }

      test = test->next;
   }

   /* an info is allocated for each thread in TestSuite_RunParallel */
   bson_free (info);

   return NULL;
}

/* easier than having to link with math library */
#define CEIL(d) ((int) (d) == d ? (int) (d) : (int) (d) + 1)


static void
TestSuite_RunParallel (TestSuite *suite) /* IN */
{
   ParallelInfo *info;
   Thread *threads;
   Mutex mutex;
   Test *test;
   int count = 0;
   int starting_count;
   int tests_per_thread;
   int i;

   ASSERT (suite);

   Mutex_Init (&mutex);

   for (test = suite->tests; test; test = test->next) {
      count++;
   }

   /* threads start decrementing count immediately, save a copy */
   starting_count = count;
   tests_per_thread = CEIL ((double) count / (double) MAX_THREADS);
   threads = (Thread *) calloc (MAX_THREADS, sizeof *threads);

   Memory_Barrier ();

   /* "tests" is a linked list, a->b->c->d->..., iterate it and every
    * tests_per_thread start a new thread, point it at our position in the list,
    * and tell it to run the next tests_per_thread tests. */
   for (test = suite->tests, i = 0; test; test = test->next, i++) {
      if (!(i % tests_per_thread)) {
         info = (ParallelInfo *) calloc (1, sizeof *info);
         info->suite = suite;
         info->test = test;
         info->n_tests = BSON_MIN (tests_per_thread, starting_count - i);
         info->count = &count;
         info->mutex = &mutex;
         Thread_Create (threads, TestSuite_ParallelWorker, info);
         threads++;
      }
   }

/* wait for the thread that runs the last test to call exit (0) */
#ifdef _WIN32
   Sleep (30000);
#else
   sleep (30);
#endif

   _Print_StdErr ("Timed out, aborting!\n");

   abort ();
}


static int
TestSuite_RunSerial (TestSuite *suite) /* IN */
{
   Test *test;
   Mutex mutex;
   int count = 0;
   int status = 0;

   Mutex_Init (&mutex);

   for (test = suite->tests; test; test = test->next) {
      count++;
   }

   for (test = suite->tests; test; test = test->next) {
      status += TestSuite_RunTest (suite, test, &mutex, &count);
      count--;
   }

   TestSuite_PrintJsonFooter (stdout);
   if (suite->outfile) {
      TestSuite_PrintJsonFooter (suite->outfile);
   }

   Mutex_Destroy (&mutex);

   return status;
}


static int
TestSuite_RunNamed (TestSuite *suite,     /* IN */
                    const char *testname) /* IN */
{
   Mutex mutex;
   char name[128];
   Test *test;
   int count = 1;
   bool star = strlen (testname) && testname[strlen (testname) - 1] == '*';
   bool match;
   int status = 0;

   ASSERT (suite);
   ASSERT (testname);

   Mutex_Init (&mutex);

   for (test = suite->tests; test; test = test->next) {
      snprintf (name, sizeof name, "%s%s", suite->name, test->name);
      name[sizeof name - 1] = '\0';
      if (star) {
         /* e.g. testname is "/Client*" and name is "/Client/authenticate" */
         match = (0 == strncmp (name, testname, strlen (testname) - 1));
      } else {
         match = (0 == strcmp (name, testname));
      }

      if (match) {
         status += TestSuite_RunTest (suite, test, &mutex, &count);
      }
   }

   TestSuite_PrintJsonFooter (stdout);
   if (suite->outfile) {
      TestSuite_PrintJsonFooter (suite->outfile);
   }

   Mutex_Destroy (&mutex);
   return status;
}


int
TestSuite_Run (TestSuite *suite) /* IN */
{
   int failures = 0;

   if ((suite->flags & TEST_HELPONLY)) {
      TestSuite_PrintHelp (suite, stderr);
      return 0;
   }

   TestSuite_PrintJsonHeader (suite, stdout);
   if (suite->outfile) {
      TestSuite_PrintJsonHeader (suite, suite->outfile);
   }

   if (suite->tests) {
      if (suite->testname) {
         failures += TestSuite_RunNamed (suite, suite->testname);
      } else if ((suite->flags & TEST_THREADS)) {
         TestSuite_RunParallel (suite);
      } else {
         failures += TestSuite_RunSerial (suite);
      }
   } else {
      TestSuite_PrintJsonFooter (stdout);
      if (suite->outfile) {
         TestSuite_PrintJsonFooter (suite->outfile);
      }
   }

   return failures;
}


void
TestSuite_Destroy (TestSuite *suite)
{
   Test *test;
   Test *tmp;

   for (test = suite->tests; test; test = tmp) {
      tmp = test->next;

      if (test->dtor) {
         test->dtor (test->ctx);
      }
      bson_free (test->name);
      free (test);
   }

   if (suite->outfile) {
      fclose (suite->outfile);
   }

   bson_free (suite->name);
   bson_free (suite->prgname);
   bson_free (suite->testname);
}


int
test_suite_debug_output (void)
{
   return 0 != (test_flags & TEST_DEBUGOUTPUT);
}


int
test_suite_valgrind (void)
{
   return 0 != (test_flags & TEST_VALGRIND);
}
