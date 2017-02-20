/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "bench.h"

#ifdef _POSIX_THREADS
typedef struct {
	pthread_t	id;
	DB_ENV		*dbenv;
	int		iterations;
	db_mutex_t	mutex;
	int		contentions;
} threadinfo_t;

static void *latch_threadmain __P((void *));
#endif

static int   time_latches __P((DB_ENV *, db_mutex_t, int));

#define	LATCH_THREADS_MAX	100

/* Return the environment needed for __mutex_lock(), depending on release.
 */
#if DB_VERSION_MAJOR <4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 7
#define	ENV_ARG(dbenv)	(dbenv)
#else
#define	ENV_ARG(dbenv)	((dbenv)->env)
#endif

/*
 * In the mulithreaded latch test each thread locks and updates this variable.
 * It detects contention when the value of this counter changes during the
 * mutex lock call.
 */
static int CurrentCounter = 0;
static int   latch_usage __P((void));

static int
latch_usage()
{
	(void)fprintf(stderr, "usage: b_latch [-c number of %s",
	    "lock+unlock pairs] [-n number of threads]\n");
	return (EXIT_FAILURE);
}

/*
 * time_latches --
 *	Repeat acquire and release of an exclusive latch, counting the
 *	number of times that 'someone else' got it just as we tried to.
 */
static int time_latches(dbenv, mutex, iterations)
	DB_ENV *dbenv;
	db_mutex_t mutex;
	int iterations;
{
	int contended, i, previous;

	contended = 0;
	for (i = 0; i < iterations; ++i) {
		previous = CurrentCounter;
		DB_BENCH_ASSERT(__mutex_lock(ENV_ARG(dbenv), mutex) == 0);
		if (previous != CurrentCounter)
			contended++;
		CurrentCounter++;
		DB_BENCH_ASSERT(__mutex_unlock(ENV_ARG(dbenv), mutex) == 0);
	}
	return (contended);
}

#ifdef _POSIX_THREADS
/*
 * latch_threadmain --
 *	Entry point for multithreaded latching test.
 *
 *	Currently only supported for POSIX threads.
 */
static void *
latch_threadmain(arg)
	void *arg;
{
	threadinfo_t	*info = arg;

	info->contentions = time_latches(info->dbenv,
	    info->mutex, info->iterations);

	return ((void *) 0);
}
#endif

/*
 * b_latch --
 *	Measure the speed of latching and mutex operations.
 *
 *
 */
int
b_latch(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB_ENV *dbenv;
	int ch, count, nthreads;
#ifdef _POSIX_THREADS
	threadinfo_t threads[LATCH_THREADS_MAX];
	int i, ret;
	void *status;
#endif
	db_mutex_t mutex;
	int contended;

	contended = 0;
	count = 1000000;
	nthreads = 0;	/* Default to running the test without extra threads */
	while ((ch = getopt(argc, argv, "c:n:")) != EOF)
		switch (ch) {
		case 'c':
			count = atoi(optarg);
			break;
		case 'n':
			nthreads = atoi(optarg);
			break;
		case '?':
		default:
			return (latch_usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0 || count < 1 || nthreads < 0 ||
	    nthreads > LATCH_THREADS_MAX)
		return (latch_usage());
#ifndef _POSIX_THREADS
	if (nthreads > 1) {
		(void)fprintf(stderr,
		    "Sorry, support for -n %d: threads not yet available\n",
		    nthreads);
		exit(EXIT_FAILURE);
	}
#endif

	/* Create the environment. */
	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);
	dbenv->set_errfile(dbenv, stderr);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR,
	    NULL, DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR,
	    DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE, 0666) == 0);
#endif
	DB_BENCH_ASSERT(dbenv->mutex_alloc(dbenv, DB_MUTEX_SELF_BLOCK,
	    &mutex) == 0);
#ifdef _POSIX_THREADS
	for (i = 0; i < nthreads; i++)  {
		threads[i].dbenv = dbenv;
		threads[i].mutex = mutex;
		threads[i].iterations =
		    nthreads <= 1 ? count : count / nthreads;
	}
#endif

	/* Start and acquire and release a mutex count times. If there's
	 * posix support and a non-zero number of threads start them.
	 */
	TIMER_START;
#ifdef _POSIX_THREADS
	if (nthreads > 0) {
		for (i = 0; i < nthreads; i++)
			DB_BENCH_ASSERT(pthread_create(&threads[i].id,
			    NULL, latch_threadmain, &threads[i]) == 0);
		for (i = 0; i < nthreads; i++) {
			ret = pthread_join(threads[i].id, &status);
			DB_BENCH_ASSERT(ret == 0);
			contended += threads[i].contentions;
		}

	} else
#endif
		contended = time_latches(dbenv, mutex, count);
	TIMER_STOP;

	printf("# %d mutex lock-unlock pairs of %d thread%s\n", count,
	    nthreads, nthreads == 1 ? "" : "s");
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbenv->mutex_free(dbenv, mutex) == 0);
	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);
	COMPQUIET(contended, 0);

	return (0);
}
