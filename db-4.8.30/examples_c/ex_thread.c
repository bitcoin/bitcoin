/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>
#include <sys/time.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

/*
 * NB: This application is written using POSIX 1003.1b-1993 pthreads
 * interfaces, which may not be portable to your system.
 */
extern int sched_yield __P((void));		/* Pthread yield function. */

int	db_init __P((const char *));
void   *deadlock __P((void *));
void	fatal __P((const char *, int, int));
void	onint __P((int));
int	main __P((int, char *[]));
int	reader __P((int));
void	stats __P((void));
void   *trickle __P((void *));
void   *tstart __P((void *));
int	usage __P((void));
void	word __P((void));
int	writer __P((int));

int	quit;					/* Interrupt handling flag. */

struct _statistics {
	int aborted;				/* Write. */
	int aborts;				/* Read/write. */
	int adds;				/* Write. */
	int deletes;				/* Write. */
	int txns;				/* Write. */
	int found;				/* Read. */
	int notfound;				/* Read. */
} *perf;

const char
	*progname = "ex_thread";		/* Program name. */

#define	DATABASE	"access.db"		/* Database name. */
#define	WORDLIST	"../test/wordlist"	/* Dictionary. */

/*
 * We can seriously increase the number of collisions and transaction
 * aborts by yielding the scheduler after every DB call.  Specify the
 * -p option to do this.
 */
int	punish;					/* -p */
int	nlist;					/* -n */
int	nreaders;				/* -r */
int	verbose;				/* -v */
int	nwriters;				/* -w */

DB     *dbp;					/* Database handle. */
DB_ENV *dbenv;					/* Database environment. */
int	nthreads;				/* Total threads. */
char  **list;					/* Word list. */

/*
 * ex_thread --
 *	Run a simple threaded application of some numbers of readers and
 *	writers competing for a set of words.
 *
 * Example UNIX shell script to run this program:
 *	% rm -rf TESTDIR
 *	% mkdir TESTDIR
 *	% ex_thread -h TESTDIR
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int errno, optind;
	DB_TXN *txnp;
	pthread_t *tids;
	int ch, i, ret;
	const char *home;
	void *retp;

	txnp = NULL;
	nlist = 1000;
	nreaders = nwriters = 4;
	home = "TESTDIR";
	while ((ch = getopt(argc, argv, "h:pn:r:vw:")) != EOF)
		switch (ch) {
		case 'h':
			home = optarg;
			break;
		case 'p':
			punish = 1;
			break;
		case 'n':
			nlist = atoi(optarg);
			break;
		case 'r':
			nreaders = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			nwriters = atoi(optarg);
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	/* Initialize the random number generator. */
	srand(getpid() | time(NULL));

	/* Register the signal handler. */
	(void)signal(SIGINT, onint);

	/* Build the key list. */
	word();

	/* Remove the previous database. */
	(void)remove(DATABASE);

	/* Initialize the database environment. */
	if ((ret = db_init(home)) != 0)
		return (ret);

	/* Initialize the database. */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		(void)dbenv->close(dbenv, 0);
		return (EXIT_FAILURE);
	}
	if ((ret = dbp->set_pagesize(dbp, 1024)) != 0) {
		dbp->err(dbp, ret, "set_pagesize");
		goto err;
	}

	if ((ret = dbenv->txn_begin(dbenv, NULL, &txnp, 0)) != 0)
		fatal("txn_begin", ret, 1);
	if ((ret = dbp->open(dbp, txnp,
	     DATABASE, NULL, DB_BTREE, DB_CREATE | DB_THREAD, 0664)) != 0) {
		dbp->err(dbp, ret, "%s: open", DATABASE);
		goto err;
	} else {
		ret = txnp->commit(txnp, 0);
		txnp = NULL;
		if (ret != 0)
			goto err;
	}

	nthreads = nreaders + nwriters + 2;
	printf("Running: readers %d, writers %d\n", nreaders, nwriters);
	fflush(stdout);

	/* Create statistics structures, offset by 1. */
	if ((perf = calloc(nreaders + nwriters + 1, sizeof(*perf))) == NULL)
		fatal(NULL, errno, 1);

	/* Create thread ID structures. */
	if ((tids = malloc(nthreads * sizeof(pthread_t))) == NULL)
		fatal(NULL, errno, 1);

	/* Create reader/writer threads. */
	for (i = 0; i < nreaders + nwriters; ++i)
		if ((ret = pthread_create(
		    &tids[i], NULL, tstart, (void *)(uintptr_t)i)) != 0)
			fatal("pthread_create", ret > 0 ? ret : errno, 1);

	/* Create buffer pool trickle thread. */
	if (pthread_create(&tids[i], NULL, trickle, &i))
		fatal("pthread_create", errno, 1);
	++i;

	/* Create deadlock detector thread. */
	if (pthread_create(&tids[i], NULL, deadlock, &i))
		fatal("pthread_create", errno, 1);

	/* Wait for the threads. */
	for (i = 0; i < nthreads; ++i)
		(void)pthread_join(tids[i], &retp);

	printf("Exiting\n");
	stats();

err:	if (txnp != NULL)
		(void)txnp->abort(txnp);
	(void)dbp->close(dbp, 0);
	(void)dbenv->close(dbenv, 0);

	return (EXIT_SUCCESS);
}

int
reader(id)
	int id;
{
	DBT key, data;
	int n, ret;
	char buf[64];

	/*
	 * DBT's must use local memory or malloc'd memory if the DB handle
	 * is accessed in a threaded fashion.
	 */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	data.flags = DB_DBT_MALLOC;

	/*
	 * Read-only threads do not require transaction protection, unless
	 * there's a need for repeatable reads.
	 */
	while (!quit) {
		/* Pick a key at random, and look it up. */
		n = rand() % nlist;
		key.data = list[n];
		key.size = strlen(key.data);

		if (verbose) {
			sprintf(buf, "reader: %d: list entry %d\n", id, n);
			write(STDOUT_FILENO, buf, strlen(buf));
		}

		switch (ret = dbp->get(dbp, NULL, &key, &data, 0)) {
		case DB_LOCK_DEADLOCK:		/* Deadlock. */
			++perf[id].aborts;
			break;
		case 0:				/* Success. */
			++perf[id].found;
			free(data.data);
			break;
		case DB_NOTFOUND:		/* Not found. */
			++perf[id].notfound;
			break;
		default:
			sprintf(buf,
			    "reader %d: dbp->get: %s", id, (char *)key.data);
			fatal(buf, ret, 0);
		}
	}
	return (0);
}

int
writer(id)
	int id;
{
	DBT key, data;
	DB_TXN *tid;
	time_t now, then;
	int n, ret;
	char buf[256], dbuf[10000];

	time(&now);
	then = now;

	/*
	 * DBT's must use local memory or malloc'd memory if the DB handle
	 * is accessed in a threaded fashion.
	 */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	data.data = dbuf;
	data.ulen = sizeof(dbuf);
	data.flags = DB_DBT_USERMEM;

	while (!quit) {
		/* Pick a random key. */
		n = rand() % nlist;
		key.data = list[n];
		key.size = strlen(key.data);

		if (verbose) {
			sprintf(buf, "writer: %d: list entry %d\n", id, n);
			write(STDOUT_FILENO, buf, strlen(buf));
		}

		/* Abort and retry. */
		if (0) {
retry:			if ((ret = tid->abort(tid)) != 0)
				fatal("DB_TXN->abort", ret, 1);
			++perf[id].aborts;
			++perf[id].aborted;
		}

		/* Thread #1 prints out the stats every 20 seconds. */
		if (id == 1) {
			time(&now);
			if (now - then >= 20) {
				stats();
				then = now;
			}
		}

		/* Begin the transaction. */
		if ((ret = dbenv->txn_begin(dbenv, NULL, &tid, 0)) != 0)
			fatal("txn_begin", ret, 1);

		/*
		 * Get the key.  If it doesn't exist, add it.  If it does
		 * exist, delete it.
		 */
		switch (ret = dbp->get(dbp, tid, &key, &data, 0)) {
		case DB_LOCK_DEADLOCK:
			goto retry;
		case 0:
			goto delete;
		case DB_NOTFOUND:
			goto add;
		}

		sprintf(buf, "writer: %d: dbp->get", id);
		fatal(buf, ret, 1);
		/* NOTREACHED */

delete:		/* Delete the key. */
		switch (ret = dbp->del(dbp, tid, &key, 0)) {
		case DB_LOCK_DEADLOCK:
			goto retry;
		case 0:
			++perf[id].deletes;
			goto commit;
		}

		sprintf(buf, "writer: %d: dbp->del", id);
		fatal(buf, ret, 1);
		/* NOTREACHED */

add:		/* Add the key.  1 data item in 30 is an overflow item. */
		data.size = 20 + rand() % 128;
		if (rand() % 30 == 0)
			data.size += 8192;

		switch (ret = dbp->put(dbp, tid, &key, &data, 0)) {
		case DB_LOCK_DEADLOCK:
			goto retry;
		case 0:
			++perf[id].adds;
			goto commit;
		default:
			sprintf(buf, "writer: %d: dbp->put", id);
			fatal(buf, ret, 1);
		}

commit:		/* The transaction finished, commit it. */
		if ((ret = tid->commit(tid, 0)) != 0)
			fatal("DB_TXN->commit", ret, 1);

		/*
		 * Every time the thread completes 20 transactions, show
		 * our progress.
		 */
		if (++perf[id].txns % 20 == 0) {
			sprintf(buf,
"writer: %2d: adds: %4d: deletes: %4d: aborts: %4d: txns: %4d\n",
			    id, perf[id].adds, perf[id].deletes,
			    perf[id].aborts, perf[id].txns);
			write(STDOUT_FILENO, buf, strlen(buf));
		}

		/*
		 * If this thread was aborted more than 5 times before
		 * the transaction finished, complain.
		 */
		if (perf[id].aborted > 5) {
			sprintf(buf,
"writer: %2d: adds: %4d: deletes: %4d: aborts: %4d: txns: %4d: ABORTED: %2d\n",
			    id, perf[id].adds, perf[id].deletes,
			    perf[id].aborts, perf[id].txns, perf[id].aborted);
			write(STDOUT_FILENO, buf, strlen(buf));
		}
		perf[id].aborted = 0;
	}
	return (0);
}

/*
 * stats --
 *	Display reader/writer thread statistics.  To display the statistics
 *	for the mpool trickle or deadlock threads, use db_stat(1).
 */
void
stats()
{
	int id;
	char *p, buf[8192];

	p = buf + sprintf(buf, "-------------\n");
	for (id = 0; id < nreaders + nwriters;)
		if (id++ < nwriters)
			p += sprintf(p,
	"writer: %2d: adds: %4d: deletes: %4d: aborts: %4d: txns: %4d\n",
			    id, perf[id].adds,
			    perf[id].deletes, perf[id].aborts, perf[id].txns);
		else
			p += sprintf(p,
	"reader: %2d: found: %5d: notfound: %5d: aborts: %4d\n",
			    id, perf[id].found,
			    perf[id].notfound, perf[id].aborts);
	p += sprintf(p, "-------------\n");

	write(STDOUT_FILENO, buf, p - buf);
}

/*
 * db_init --
 *	Initialize the environment.
 */
int
db_init(home)
	const char *home;
{
	int ret;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	if (punish)
		(void)dbenv->set_flags(dbenv, DB_YIELDCPU, 1);

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);
	(void)dbenv->set_cachesize(dbenv, 0, 100 * 1024, 0);
	(void)dbenv->set_lg_max(dbenv, 200000);

	if ((ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN | DB_THREAD, 0)) != 0) {
		dbenv->err(dbenv, ret, NULL);
		(void)dbenv->close(dbenv, 0);
		return (EXIT_FAILURE);
	}

	return (0);
}

/*
 * tstart --
 *	Thread start function for readers and writers.
 */
void *
tstart(arg)
	void *arg;
{
	pthread_t tid;
	u_int id;

	id = (uintptr_t)arg + 1;

	tid = pthread_self();

	if (id <= (u_int)nwriters) {
		printf("write thread %d starting: tid: %lu\n", id, (u_long)tid);
		fflush(stdout);
		writer(id);
	} else {
		printf("read thread %d starting: tid: %lu\n", id, (u_long)tid);
		fflush(stdout);
		reader(id);
	}

	/* NOTREACHED */
	return (NULL);
}

/*
 * deadlock --
 *	Thread start function for DB_ENV->lock_detect.
 */
void *
deadlock(arg)
	void *arg;
{
	struct timeval t;
	pthread_t tid;

	arg = arg;				/* XXX: shut the compiler up. */
	tid = pthread_self();

	printf("deadlock thread starting: tid: %lu\n", (u_long)tid);
	fflush(stdout);

	t.tv_sec = 0;
	t.tv_usec = 100000;
	while (!quit) {
		(void)dbenv->lock_detect(dbenv, 0, DB_LOCK_YOUNGEST, NULL);

		/* Check every 100ms. */
		(void)select(0, NULL, NULL, NULL, &t);
	}

	return (NULL);
}

/*
 * trickle --
 *	Thread start function for memp_trickle.
 */
void *
trickle(arg)
	void *arg;
{
	pthread_t tid;
	int wrote;
	char buf[64];

	arg = arg;				/* XXX: shut the compiler up. */
	tid = pthread_self();

	printf("trickle thread starting: tid: %lu\n", (u_long)tid);
	fflush(stdout);

	while (!quit) {
		(void)dbenv->memp_trickle(dbenv, 10, &wrote);
		if (verbose) {
			sprintf(buf, "trickle: wrote %d\n", wrote);
			write(STDOUT_FILENO, buf, strlen(buf));
		}
		if (wrote == 0) {
			sleep(1);
			sched_yield();
		}
	}

	return (NULL);
}

/*
 * word --
 *	Build the dictionary word list.
 */
void
word()
{
	FILE *fp;
	int cnt;
	char buf[256];

	if ((fp = fopen(WORDLIST, "r")) == NULL)
		fatal(WORDLIST, errno, 1);

	if ((list = malloc(nlist * sizeof(char *))) == NULL)
		fatal(NULL, errno, 1);

	for (cnt = 0; cnt < nlist; ++cnt) {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			break;
		if ((list[cnt] = strdup(buf)) == NULL)
			fatal(NULL, errno, 1);
	}
	nlist = cnt;		/* In case nlist was larger than possible. */
}

/*
 * fatal --
 *	Report a fatal error and quit.
 */
void
fatal(msg, err, syserr)
	const char *msg;
	int err, syserr;
{
	fprintf(stderr, "%s: ", progname);
	if (msg != NULL) {
		fprintf(stderr, "%s", msg);
		if (syserr)
			fprintf(stderr, ": ");
	}
	if (syserr)
		fprintf(stderr, "%s", strerror(err));
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);

	/* NOTREACHED */
}

/*
 * usage --
 *	Usage message.
 */
int
usage()
{
	(void)fprintf(stderr,
    "usage: %s [-pv] [-h home] [-n words] [-r readers] [-w writers]\n",
	    progname);
	return (EXIT_FAILURE);
}

/*
 * onint --
 *	Interrupt signal handler.
 */
void
onint(signo)
	int signo;
{
	signo = 0;		/* Quiet compiler. */
	quit = 1;
}
