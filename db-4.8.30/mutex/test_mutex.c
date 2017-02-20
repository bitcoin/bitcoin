/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * Standalone mutex tester for Berkeley DB mutexes.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef DB_WIN32
#define	MUTEX_THREAD_TEST	1

extern int getopt(int, char * const *, const char *);

typedef HANDLE os_pid_t;
typedef HANDLE os_thread_t;

#define	os_thread_create(thrp, attr, func, arg)				\
    (((*(thrp) = CreateThread(NULL, 0,					\
	(LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define	os_thread_join(thr, statusp)					\
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&		\
    GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)
#define	os_thread_self() GetCurrentThreadId()

#else /* !DB_WIN32 */

#include <sys/wait.h>

typedef pid_t os_pid_t;

/*
 * There's only one mutex implementation that can't support thread-level
 * locking: UNIX/fcntl mutexes.
 *
 * The general Berkeley DB library configuration doesn't look for the POSIX
 * pthread functions, with one exception -- pthread_yield.
 *
 * Use these two facts to decide if we're going to build with or without
 * threads.
 */
#if !defined(HAVE_MUTEX_FCNTL) && defined(HAVE_PTHREAD_YIELD)
#define	MUTEX_THREAD_TEST	1

#include <pthread.h>

typedef pthread_t os_thread_t;

#define	os_thread_create(thrp, attr, func, arg)				\
    pthread_create((thrp), (attr), (func), (arg))
#define	os_thread_join(thr, statusp) pthread_join((thr), (statusp))
#define	os_thread_self() pthread_self()
#endif /* HAVE_PTHREAD_YIELD */
#endif /* !DB_WIN32 */

#define	OS_BAD_PID ((os_pid_t)-1)

#define	TESTDIR		"TESTDIR"		/* Working area */
#define	MT_FILE		"TESTDIR/mutex.file"
#define	MT_FILE_QUIT	"TESTDIR/mutex.file.quit"

/*
 * The backing data layout:
 *	TM[1]			per-thread mutex array lock
 *	TM[nthreads]		per-thread mutex array
 *	TM[maxlocks]		per-lock mutex array
 */
typedef struct {
	db_mutex_t mutex;			/* Mutex. */
	u_long	   id;				/* Holder's ID. */
	u_int	   wakeme;			/* Request to awake. */
} TM;

DB_ENV	*dbenv;					/* Backing environment */
ENV	*env;
size_t	 len;					/* Backing data chunk size. */

u_int8_t *gm_addr;				/* Global mutex */
u_int8_t *lm_addr;				/* Locker mutexes */
u_int8_t *tm_addr;				/* Thread mutexes */

#ifdef MUTEX_THREAD_TEST
os_thread_t *kidsp;				/* Locker threads */
os_thread_t  wakep;				/* Wakeup thread */
#endif

#ifndef	HAVE_MMAP
u_int	nprocs = 1;				/* -p: Processes. */
u_int	nthreads = 20;				/* -t: Threads. */
#elif	MUTEX_THREAD_TEST
u_int	nprocs = 5;				/* -p: Processes. */
u_int	nthreads = 4;				/* -t: Threads. */
#else
u_int	nprocs = 20;				/* -p: Processes. */
u_int	nthreads = 1;				/* -t: Threads. */
#endif

u_int	maxlocks = 20;				/* -l: Backing locks. */
u_int	nlocks = 10000;				/* -n: Locks per process. */
int	verbose;				/* -v: Verbosity. */

const char *progname;

void	 data_off(u_int8_t *, DB_FH *);
void	 data_on(u_int8_t **, u_int8_t **, u_int8_t **, DB_FH **, int);
int	 locker_start(u_long);
int	 locker_wait(void);
os_pid_t os_spawn(const char *, char *const[]);
int	 os_wait(os_pid_t *, u_int);
void	*run_lthread(void *);
void	*run_wthread(void *);
os_pid_t spawn_proc(u_long, char *, char *);
void	 tm_env_close(void);
int	 tm_env_init(void);
void	 tm_mutex_destroy(void);
void	 tm_mutex_init(void);
void	 tm_mutex_stats(void);
int	 usage(void);
int	 wakeup_start(u_long);
int	 wakeup_wait(void);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	enum {LOCKER, WAKEUP, PARENT} rtype;
	extern int optind;
	extern char *optarg;
	os_pid_t wakeup_pid, *pids;
	u_long id;
	u_int i;
	DB_FH *fhp, *map_fhp;
	int ch, err;
	char *p, *tmpath, cmd[1024];

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	rtype = PARENT;
	id = 0;
	tmpath = argv[0];
	while ((ch = getopt(argc, argv, "l:n:p:T:t:v")) != EOF)
		switch (ch) {
		case 'l':
			maxlocks = (u_int)atoi(optarg);
			break;
		case 'n':
			nlocks = (u_int)atoi(optarg);
			break;
		case 'p':
			nprocs = (u_int)atoi(optarg);
			break;
		case 't':
			if ((nthreads = (u_int)atoi(optarg)) == 0)
				nthreads = 1;
#if !defined(MUTEX_THREAD_TEST)
			if (nthreads != 1) {
				fprintf(stderr,
    "%s: thread support not available or not compiled for this platform.\n",
				    progname);
				return (EXIT_FAILURE);
			}
#endif
			break;
		case 'T':
			if (!memcmp(optarg, "locker", sizeof("locker") - 1))
				rtype = LOCKER;
			else if (
			    !memcmp(optarg, "wakeup", sizeof("wakeup") - 1))
				rtype = WAKEUP;
			else
				return (usage());
			if ((p = strchr(optarg, '=')) == NULL)
				return (usage());
			id = (u_long)atoi(p + 1);
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	/*
	 * If we're not running a multi-process test, we should be running
	 * a multi-thread test.
	 */
	if (nprocs == 1 && nthreads == 1) {
		fprintf(stderr,
	    "%s: running in a single process requires multiple threads\n",
		    progname);
		return (EXIT_FAILURE);
	}

	len = sizeof(TM) * (1 + nthreads * nprocs + maxlocks);

	/*
	 * In the multi-process test, the parent spawns processes that exec
	 * the original binary, ending up here.  Each process joins the DB
	 * environment separately and then calls the supporting function.
	 */
	if (rtype == LOCKER || rtype == WAKEUP) {
		__os_yield(env, 3, 0);		/* Let everyone catch up. */
						/* Initialize random numbers. */
		srand((u_int)time(NULL) % (u_int)getpid());

		if (tm_env_init() != 0)		/* Join the environment. */
			exit(EXIT_FAILURE);
						/* Join the backing data. */
		data_on(&gm_addr, &tm_addr, &lm_addr, &map_fhp, 0);
		if (verbose)
			printf(
	    "Backing file: global (%#lx), threads (%#lx), locks (%#lx)\n",
			    (u_long)gm_addr, (u_long)tm_addr, (u_long)lm_addr);

		if ((rtype == LOCKER ?
		    locker_start(id) : wakeup_start(id)) != 0)
			exit(EXIT_FAILURE);
		if ((rtype == LOCKER ? locker_wait() : wakeup_wait()) != 0)
			exit(EXIT_FAILURE);

		data_off(gm_addr, map_fhp);	/* Detach from backing data. */

		tm_env_close();			/* Detach from environment. */

		exit(EXIT_SUCCESS);
	}

	/*
	 * The following code is only executed by the original parent process.
	 *
	 * Clean up from any previous runs.
	 */
	snprintf(cmd, sizeof(cmd), "rm -rf %s", TESTDIR);
	(void)system(cmd);
	snprintf(cmd, sizeof(cmd), "mkdir %s", TESTDIR);
	(void)system(cmd);

	printf(
    "%s: %u processes, %u threads/process, %u lock requests from %u locks\n",
	    progname, nprocs, nthreads, nlocks, maxlocks);
	printf("%s: backing data %lu bytes\n", progname, (u_long)len);

	if (tm_env_init() != 0)		/* Create the environment. */
		exit(EXIT_FAILURE);
					/* Create the backing data. */
	data_on(&gm_addr, &tm_addr, &lm_addr, &map_fhp, 1);
	if (verbose)
		printf(
	    "backing data: global (%#lx), threads (%#lx), locks (%#lx)\n",
		    (u_long)gm_addr, (u_long)tm_addr, (u_long)lm_addr);

	tm_mutex_init();		/* Initialize mutexes. */

	if (nprocs > 1) {		/* Run the multi-process test. */
		/* Allocate array of locker process IDs. */
		if ((pids = calloc(nprocs, sizeof(os_pid_t))) == NULL) {
			fprintf(stderr, "%s: %s\n", progname, strerror(errno));
			goto fail;
		}

		/* Spawn locker processes and threads. */
		for (i = 0; i < nprocs; ++i) {
			if ((pids[i] =
			    spawn_proc(id, tmpath, "locker")) == OS_BAD_PID) {
				fprintf(stderr,
				    "%s: failed to spawn a locker\n", progname);
				goto fail;
			}
			id += nthreads;
		}

		/* Spawn wakeup process/thread. */
		if ((wakeup_pid =
		    spawn_proc(id, tmpath, "wakeup")) == OS_BAD_PID) {
			fprintf(stderr,
			    "%s: failed to spawn waker\n", progname);
			goto fail;
		}
		++id;

		/* Wait for all lockers to exit. */
		if ((err = os_wait(pids, nprocs)) != 0) {
			fprintf(stderr, "%s: locker wait failed with %d\n",
			    progname, err);
			goto fail;
		}

		/* Signal wakeup process to exit. */
		if ((err = __os_open(
		    env, MT_FILE_QUIT, 0, DB_OSO_CREATE, 0664, &fhp)) != 0) {
			fprintf(stderr,
			    "%s: open %s\n", progname, db_strerror(err));
			goto fail;
		}
		(void)__os_closehandle(env, fhp);

		/* Wait for wakeup process/thread. */
		if ((err = os_wait(&wakeup_pid, 1)) != 0) {
			fprintf(stderr, "%s: %lu: exited %d\n",
			    progname, (u_long)wakeup_pid, err);
			goto fail;
		}
	} else {			/* Run the single-process test. */
		/* Spawn locker threads. */
		if (locker_start(0) != 0)
			goto fail;

		/* Spawn wakeup thread. */
		if (wakeup_start(nthreads) != 0)
			goto fail;

		/* Wait for all lockers to exit. */
		if (locker_wait() != 0)
			goto fail;

		/* Signal wakeup process to exit. */
		if ((err = __os_open(
		    env, MT_FILE_QUIT, 0, DB_OSO_CREATE, 0664, &fhp)) != 0) {
			fprintf(stderr,
			    "%s: open %s\n", progname, db_strerror(err));
			goto fail;
		}
		(void)__os_closehandle(env, fhp);

		/* Wait for wakeup thread. */
		if (wakeup_wait() != 0)
			goto fail;
	}

	tm_mutex_stats();		/* Display run statistics. */
	tm_mutex_destroy();		/* Destroy mutexes. */

	data_off(gm_addr, map_fhp);	/* Detach from backing data. */

	tm_env_close();			/* Detach from environment. */

	printf("%s: test succeeded\n", progname);
	return (EXIT_SUCCESS);

fail:	printf("%s: FAILED!\n", progname);
	return (EXIT_FAILURE);
}

int
locker_start(id)
	u_long id;
{
#if defined(MUTEX_THREAD_TEST)
	u_int i;
	int err;

	/*
	 * Spawn off threads.  We have nthreads all locking and going to
	 * sleep, and one other thread cycling through and waking them up.
	 */
	if ((kidsp =
	    (os_thread_t *)calloc(sizeof(os_thread_t), nthreads)) == NULL) {
		fprintf(stderr, "%s: %s\n", progname, strerror(errno));
		return (1);
	}
	for (i = 0; i < nthreads; i++)
		if ((err = os_thread_create(
		    &kidsp[i], NULL, run_lthread, (void *)(id + i))) != 0) {
			fprintf(stderr, "%s: failed spawning thread: %s\n",
			    progname, db_strerror(err));
			return (1);
		}
	return (0);
#else
	return (run_lthread((void *)id) == NULL ? 0 : 1);
#endif
}

int
locker_wait()
{
#if defined(MUTEX_THREAD_TEST)
	u_int i;
	void *retp;

	/* Wait for the threads to exit. */
	for (i = 0; i < nthreads; i++) {
		(void)os_thread_join(kidsp[i], &retp);
		if (retp != NULL) {
			fprintf(stderr,
			    "%s: thread exited with error\n", progname);
			return (1);
		}
	}
	free(kidsp);
#endif
	return (0);
}

void *
run_lthread(arg)
	void *arg;
{
	TM *gp, *mp, *tp;
	u_long id, tid;
	u_int lock, nl;
	int err, i;

	id = (u_long)arg;
#if defined(MUTEX_THREAD_TEST)
	tid = (u_long)os_thread_self();
#else
	tid = 0;
#endif
	printf("Locker: ID %03lu (PID: %lu; TID: %lx)\n",
	    id, (u_long)getpid(), tid);

	gp = (TM *)gm_addr;
	tp = (TM *)(tm_addr + id * sizeof(TM));

	for (nl = nlocks; nl > 0;) {
		/* Select and acquire a data lock. */
		lock = (u_int)rand() % maxlocks;
		mp = (TM *)(lm_addr + lock * sizeof(TM));
		if (verbose)
			printf("%03lu: lock %d (mtx: %lu)\n",
			    id, lock, (u_long)mp->mutex);

		if ((err = dbenv->mutex_lock(dbenv, mp->mutex)) != 0) {
			fprintf(stderr, "%s: %03lu: never got lock %d: %s\n",
			    progname, id, lock, db_strerror(err));
			return ((void *)1);
		}
		if (mp->id != 0) {
			fprintf(stderr,
			    "%s: RACE! (%03lu granted lock %d held by %03lu)\n",
			    progname, id, lock, mp->id);
			return ((void *)1);
		}
		mp->id = id;

		/*
		 * Pretend to do some work, periodically checking to see if
		 * we still hold the mutex.
		 */
		for (i = 0; i < 3; ++i) {
			__os_yield(env, 0, (u_long)rand() % 3);
			if (mp->id != id) {
				fprintf(stderr,
			    "%s: RACE! (%03lu stole lock %d from %03lu)\n",
				    progname, mp->id, lock, id);
				return ((void *)1);
			}
		}

		/*
		 * Test self-blocking and unlocking by other threads/processes:
		 *
		 *	acquire the global lock
		 *	set our wakeup flag
		 *	release the global lock
		 *	acquire our per-thread lock
		 *
		 * The wakeup thread will wake us up.
		 */
		if ((err = dbenv->mutex_lock(dbenv, gp->mutex)) != 0) {
			fprintf(stderr, "%s: %03lu: global lock: %s\n",
			    progname, id, db_strerror(err));
			return ((void *)1);
		}
		if (tp->id != 0 && tp->id != id) {
			fprintf(stderr,
		    "%s: %03lu: per-thread mutex isn't mine, owned by %03lu\n",
			    progname, id, tp->id);
			return ((void *)1);
		}
		tp->id = id;
		if (verbose)
			printf("%03lu: self-blocking (mtx: %lu)\n",
			    id, (u_long)tp->mutex);
		if (tp->wakeme) {
			fprintf(stderr,
			    "%s: %03lu: wakeup flag incorrectly set\n",
			    progname, id);
			return ((void *)1);
		}
		tp->wakeme = 1;
		if ((err = dbenv->mutex_unlock(dbenv, gp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: %03lu: global unlock: %s\n",
			    progname, id, db_strerror(err));
			return ((void *)1);
		}
		if ((err = dbenv->mutex_lock(dbenv, tp->mutex)) != 0) {
			fprintf(stderr, "%s: %03lu: per-thread lock: %s\n",
			    progname, id, db_strerror(err));
			return ((void *)1);
		}
		/* Time passes... */
		if (tp->wakeme) {
			fprintf(stderr, "%s: %03lu: wakeup flag not cleared\n",
			    progname, id);
			return ((void *)1);
		}

		if (verbose)
			printf("%03lu: release %d (mtx: %lu)\n",
			    id, lock, (u_long)mp->mutex);

		/* Release the data lock. */
		mp->id = 0;
		if ((err = dbenv->mutex_unlock(dbenv, mp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: %03lu: lock release: %s\n",
			    progname, id, db_strerror(err));
			return ((void *)1);
		}

		if (--nl % 1000 == 0)
			printf("%03lu: %d\n", id, nl);
	}

	return (NULL);
}

int
wakeup_start(id)
	u_long id;
{
#if defined(MUTEX_THREAD_TEST)
	int err;

	/*
	 * Spawn off wakeup thread.
	 */
	if ((err = os_thread_create(
	    &wakep, NULL, run_wthread, (void *)id)) != 0) {
		fprintf(stderr, "%s: failed spawning wakeup thread: %s\n",
		    progname, db_strerror(err));
		return (1);
	}
	return (0);
#else
	return (run_wthread((void *)id) == NULL ? 0 : 1);
#endif
}

int
wakeup_wait()
{
#if defined(MUTEX_THREAD_TEST)
	void *retp;

	/*
	 * A file is created when the wakeup thread is no longer needed.
	 */
	(void)os_thread_join(wakep, &retp);
	if (retp != NULL) {
		fprintf(stderr,
		    "%s: wakeup thread exited with error\n", progname);
		return (1);
	}
#endif
	return (0);
}

/*
 * run_wthread --
 *	Thread to wake up other threads that are sleeping.
 */
void *
run_wthread(arg)
	void *arg;
{
	TM *gp, *tp;
	u_long id, tid;
	u_int check_id;
	int err, quitcheck;

	id = (u_long)arg;
	quitcheck = 0;
#if defined(MUTEX_THREAD_TEST)
	tid = (u_long)os_thread_self();
#else
	tid = 0;
#endif
	printf("Wakeup: ID %03lu (PID: %lu; TID: %lx)\n",
	    id, (u_long)getpid(), tid);

	gp = (TM *)gm_addr;

	/* Loop, waking up sleepers and periodically sleeping ourselves. */
	for (check_id = 0;; ++check_id) {
		/* Check to see if the locking threads have finished. */
		if (++quitcheck >= 100) {
			quitcheck = 0;
		if (__os_exists(env, MT_FILE_QUIT, NULL) == 0)
			break;
		}

		/* Check for ID wraparound. */
		if (check_id == nthreads * nprocs)
			check_id = 0;

		/* Check for a thread that needs a wakeup. */
		tp = (TM *)(tm_addr + check_id * sizeof(TM));
		if (!tp->wakeme)
			continue;

		if (verbose) {
			printf("%03lu: wakeup thread %03lu (mtx: %lu)\n",
			    id, tp->id, (u_long)tp->mutex);
			(void)fflush(stdout);
		}

		/* Acquire the global lock. */
		if ((err = dbenv->mutex_lock(dbenv, gp->mutex)) != 0) {
			fprintf(stderr, "%s: wakeup: global lock: %s\n",
			    progname, db_strerror(err));
			return ((void *)1);
		}

		tp->wakeme = 0;
		if ((err = dbenv->mutex_unlock(dbenv, tp->mutex)) != 0) {
			fprintf(stderr, "%s: wakeup: unlock: %s\n",
			    progname, db_strerror(err));
			return ((void *)1);
		}

		if ((err = dbenv->mutex_unlock(dbenv, gp->mutex)) != 0) {
			fprintf(stderr, "%s: wakeup: global unlock: %s\n",
			    progname, db_strerror(err));
			return ((void *)1);
		}

		__os_yield(env, 0, (u_long)rand() % 3);
	}
	return (NULL);
}

/*
 * tm_env_init --
 *	Create the backing database environment.
 */
int
tm_env_init()
{
	u_int32_t flags;
	int ret;
	char *home;

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "%s: %s\n", progname, db_strerror(ret));
		return (1);
	}
	env = dbenv->env;
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	/* Allocate enough mutexes. */
	if ((ret = dbenv->mutex_set_increment(dbenv,
	    1 + nthreads * nprocs + maxlocks)) != 0) {
		dbenv->err(dbenv, ret, "dbenv->mutex_set_increment");
		return (1);
	}

	flags = DB_CREATE;
	if (nprocs == 1) {
		home = NULL;
		flags |= DB_PRIVATE;
	} else
		home = TESTDIR;
	if (nthreads != 1)
		flags |= DB_THREAD;
	if ((ret = dbenv->open(dbenv, home, flags, 0)) != 0) {
		dbenv->err(dbenv, ret, "environment open: %s", home);
		return (1);
	}

	return (0);
}

/*
 * tm_env_close --
 *	Close the backing database environment.
 */
void
tm_env_close()
{
	(void)dbenv->close(dbenv, 0);
}

/*
 * tm_mutex_init --
 *	Initialize the mutexes.
 */
void
tm_mutex_init()
{
	TM *mp;
	u_int i;
	int err;

	if (verbose)
		printf("Allocate the global mutex: ");
	mp = (TM *)gm_addr;
	if ((err = dbenv->mutex_alloc(dbenv, 0, &mp->mutex)) != 0) {
		fprintf(stderr, "%s: DB_ENV->mutex_alloc (global): %s\n",
		    progname, db_strerror(err));
		exit(EXIT_FAILURE);
	}
	if (verbose)
		printf("%lu\n", (u_long)mp->mutex);

	if (verbose)
		printf(
		    "Allocate %d per-thread, self-blocking mutexes: ",
		    nthreads * nprocs);
	for (i = 0; i < nthreads * nprocs; ++i) {
		mp = (TM *)(tm_addr + i * sizeof(TM));
		if ((err = dbenv->mutex_alloc(
		    dbenv, DB_MUTEX_SELF_BLOCK, &mp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: DB_ENV->mutex_alloc (per-thread %d): %s\n",
			    progname, i, db_strerror(err));
			exit(EXIT_FAILURE);
		}
		if ((err = dbenv->mutex_lock(dbenv, mp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: DB_ENV->mutex_lock (per-thread %d): %s\n",
			    progname, i, db_strerror(err));
			exit(EXIT_FAILURE);
		}
		if (verbose)
			printf("%lu ", (u_long)mp->mutex);
	}
	if (verbose)
		printf("\n");

	if (verbose)
		printf("Allocate %d per-lock mutexes: ", maxlocks);
	for (i = 0; i < maxlocks; ++i) {
		mp = (TM *)(lm_addr + i * sizeof(TM));
		if ((err = dbenv->mutex_alloc(dbenv, 0, &mp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: DB_ENV->mutex_alloc (per-lock: %d): %s\n",
			    progname, i, db_strerror(err));
			exit(EXIT_FAILURE);
		}
		if (verbose)
			printf("%lu ", (u_long)mp->mutex);
	}
	if (verbose)
		printf("\n");
}

/*
 * tm_mutex_destroy --
 *	Destroy the mutexes.
 */
void
tm_mutex_destroy()
{
	TM *gp, *mp;
	u_int i;
	int err;

	if (verbose)
		printf("Destroy the global mutex.\n");
	gp = (TM *)gm_addr;
	if ((err = dbenv->mutex_free(dbenv, gp->mutex)) != 0) {
		fprintf(stderr, "%s: DB_ENV->mutex_free (global): %s\n",
		    progname, db_strerror(err));
		exit(EXIT_FAILURE);
	}

	if (verbose)
		printf("Destroy the per-thread mutexes.\n");
	for (i = 0; i < nthreads * nprocs; ++i) {
		mp = (TM *)(tm_addr + i * sizeof(TM));
		if ((err = dbenv->mutex_free(dbenv, mp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: DB_ENV->mutex_free (per-thread %d): %s\n",
			    progname, i, db_strerror(err));
			exit(EXIT_FAILURE);
		}
	}

	if (verbose)
		printf("Destroy the per-lock mutexes.\n");
	for (i = 0; i < maxlocks; ++i) {
		mp = (TM *)(lm_addr + i * sizeof(TM));
		if ((err = dbenv->mutex_free(dbenv, mp->mutex)) != 0) {
			fprintf(stderr,
			    "%s: DB_ENV->mutex_free (per-lock: %d): %s\n",
			    progname, i, db_strerror(err));
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * tm_mutex_stats --
 *	Display mutex statistics.
 */
void
tm_mutex_stats()
{
#ifdef HAVE_STATISTICS
	TM *mp;
	uintmax_t set_wait, set_nowait;
	u_int i;

	printf("Per-lock mutex statistics.\n");
	for (i = 0; i < maxlocks; ++i) {
		mp = (TM *)(lm_addr + i * sizeof(TM));
		__mutex_set_wait_info(env, mp->mutex, &set_wait, &set_nowait);
		printf("mutex %2d: wait: %lu; no wait %lu\n", i,
		    (u_long)set_wait, (u_long)set_nowait);
	}
#endif
}

/*
 * data_on --
 *	Map in or allocate the backing data space.
 */
void
data_on(gm_addrp, tm_addrp, lm_addrp, fhpp, init)
	u_int8_t **gm_addrp, **tm_addrp, **lm_addrp;
	DB_FH **fhpp;
	int init;
{
	DB_FH *fhp;
	size_t nwrite;
	int err;
	void *addr;

	fhp = NULL;

	/*
	 * In a single process, use heap memory.
	 */
	if (nprocs == 1) {
		if (init) {
			if ((err =
			    __os_calloc(env, (size_t)len, 1, &addr)) != 0)
				exit(EXIT_FAILURE);
		} else {
			fprintf(stderr,
			    "%s: init should be set for single process call\n",
			    progname);
			exit(EXIT_FAILURE);
		}
	} else {
		if (init) {
			if (verbose)
				printf("Create the backing file.\n");

			if ((err = __os_open(env, MT_FILE, 0,
			    DB_OSO_CREATE | DB_OSO_TRUNC, 0666, &fhp)) == -1) {
				fprintf(stderr, "%s: %s: open: %s\n",
				    progname, MT_FILE, db_strerror(err));
				exit(EXIT_FAILURE);
			}

			if ((err = 
			    __os_seek(env, fhp, 0, 0, (u_int32_t)len)) != 0 ||
			    (err =
			    __os_write(env, fhp, &err, 1, &nwrite)) != 0 ||
			    nwrite != 1) {
				fprintf(stderr, "%s: %s: seek/write: %s\n",
				    progname, MT_FILE, db_strerror(err));
				exit(EXIT_FAILURE);
			}
		} else
			if ((err = __os_open(env, MT_FILE, 0, 0, 0, &fhp)) != 0)
				exit(EXIT_FAILURE);

		if ((err =
		    __os_mapfile(env, MT_FILE, fhp, len, 0, &addr)) != 0)
			exit(EXIT_FAILURE);
	}

	*gm_addrp = (u_int8_t *)addr;
	addr = (u_int8_t *)addr + sizeof(TM);
	*tm_addrp = (u_int8_t *)addr;
	addr = (u_int8_t *)addr + sizeof(TM) * (nthreads * nprocs);
	*lm_addrp = (u_int8_t *)addr;

	if (fhpp != NULL)
		*fhpp = fhp;
}

/*
 * data_off --
 *	Discard or de-allocate the backing data space.
 */
void
data_off(addr, fhp)
	u_int8_t *addr;
	DB_FH *fhp;
{
	if (nprocs == 1)
		__os_free(env, addr);
	else {
		if (__os_unmapfile(env, addr, len) != 0)
			exit(EXIT_FAILURE);
		if (__os_closehandle(env, fhp) != 0)
			exit(EXIT_FAILURE);
	}
}

/*
 * usage --
 *
 */
int
usage()
{
	fprintf(stderr, "usage: %s %s\n\t%s\n", progname,
	    "[-v] [-l maxlocks]",
	    "[-n locks] [-p procs] [-T locker=ID|wakeup=ID] [-t threads]");
	return (EXIT_FAILURE);
}

/*
 * os_wait --
 *	Wait for an array of N procs.
 */
int
os_wait(procs, n)
	os_pid_t *procs;
	u_int n;
{
	u_int i;
	int status;
#if defined(DB_WIN32)
	DWORD ret;
#endif

	status = 0;

#if defined(DB_WIN32)
	do {
		ret = WaitForMultipleObjects(n, procs, FALSE, INFINITE);
		i = ret - WAIT_OBJECT_0;
		if (i < 0 || i >= n)
			return (__os_posix_err(__os_get_syserr()));

		if ((GetExitCodeProcess(procs[i], &ret) == 0) || (ret != 0))
			return (ret);

		/* remove the process handle from the list */
		while (++i < n)
			procs[i - 1] = procs[i];
	} while (--n);
#elif !defined(HAVE_VXWORKS)
	do {
		if (wait(&status) == -1)
			return (__os_posix_err(__os_get_syserr()));

		if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
			for (i = 0; i < n; i++)
				(void)kill(procs[i], SIGKILL);
			return (WEXITSTATUS(status));
		}
	} while (--n);
#endif

	return (0);
}

os_pid_t
spawn_proc(id, tmpath, typearg)
	u_long id;
	char *tmpath, *typearg;
{
	char *const vbuf = verbose ?  "-v" : NULL;
	char *args[13], lbuf[16], nbuf[16], pbuf[16], tbuf[16], Tbuf[256];

	args[0] = tmpath;
	args[1] = "-l";
	snprintf(lbuf, sizeof(lbuf),  "%d", maxlocks);
	args[2] = lbuf;
	args[3] = "-n";
	snprintf(nbuf, sizeof(nbuf),  "%d", nlocks);
	args[4] = nbuf;
	args[5] = "-p";
	snprintf(pbuf, sizeof(pbuf),  "%d", nprocs);
	args[6] = pbuf;
	args[7] = "-t";
	snprintf(tbuf, sizeof(tbuf),  "%d", nthreads);
	args[8] = tbuf;
	args[9] = "-T";
	snprintf(Tbuf, sizeof(Tbuf),  "%s=%lu", typearg, id);
	args[10] = Tbuf;
	args[11] = vbuf;
	args[12] = NULL;

	return (os_spawn(tmpath, args));
}

os_pid_t
os_spawn(path, argv)
	const char *path;
	char *const argv[];
{
	os_pid_t pid;
	int status;

	COMPQUIET(pid, 0);
	COMPQUIET(status, 0);

#ifdef HAVE_VXWORKS
	fprintf(stderr, "%s: os_spawn not supported for VxWorks.\n", progname);
	return (OS_BAD_PID);
#elif defined(HAVE_QNX)
	/*
	 * For QNX, we cannot fork if we've ever used threads.  So
	 * we'll use their spawn function.  We use 'spawnl' which
	 * is NOT a POSIX function.
	 *
	 * The return value of spawnl is just what we want depending
	 * on the value of the 'wait' arg.
	 */
	return (spawnv(P_NOWAIT, path, argv));
#elif defined(DB_WIN32)
	return (os_pid_t)(_spawnv(P_NOWAIT, path, argv));
#else
	if ((pid = fork()) != 0) {
		if (pid == -1)
			return (OS_BAD_PID);
		return (pid);
	} else {
		(void)execv(path, argv);
		exit(EXIT_FAILURE);
	}
#endif
}
