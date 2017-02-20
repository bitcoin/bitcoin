/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define	NS_PER_MS	1000000		/* Nanoseconds in a millisecond */
#define	NS_PER_US	1000		/* Nanoseconds in a microsecond */
#ifdef _WIN32
#include <sys/timeb.h>
extern int getopt(int, char * const *, const char *);
/* Implement a basic high res timer with a POSIX interface for Windows. */
struct timeval {
	time_t tv_sec;
	long tv_usec;
};
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	struct _timeb now;
	_ftime(&now);
	tv->tv_sec = now.time;
	tv->tv_usec = now.millitm * NS_PER_US;
	return (0);
}
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <db.h>

typedef enum { ACCOUNT, BRANCH, TELLER } FTYPE;

DB_ENV	 *db_init __P((const char *, const char *, int, u_int32_t));
int	  hpopulate __P((DB *, int, int, int, int));
int	  populate __P((DB *, u_int32_t, u_int32_t, int, const char *));
u_int32_t random_id __P((FTYPE, int, int, int));
u_int32_t random_int __P((u_int32_t, u_int32_t));
int	  tp_populate __P((DB_ENV *, int, int, int, int, int));
int	  tp_run __P((DB_ENV *, int, int, int, int, int));
int	  tp_txn __P((DB_ENV *, DB *, DB *, DB *, DB *, int, int, int, int));

int	  invarg __P((const char *, int, const char *));
int	  main __P((int, char *[]));
int	  usage __P((const char *));

/*
 * This program implements a basic TPC/B driver program.  To create the
 * TPC/B database, run with the -i (init) flag.  The number of records
 * with which to populate the account, history, branch, and teller tables
 * is specified by the a, s, b, and t flags respectively.  To run a TPC/B
 * test, use the n flag to indicate a number of transactions to run (note
 * that you can run many of these processes in parallel to simulate a
 * multiuser test run).
 */
#define	TELLERS_PER_BRANCH	10
#define	ACCOUNTS_PER_TELLER	10000
#define	HISTORY_PER_BRANCH	2592000

/*
 * The default configuration that adheres to TPCB scaling rules requires
 * nearly 3 GB of space.  To avoid requiring that much space for testing,
 * we set the parameters much lower.  If you want to run a valid 10 TPS
 * configuration, define VALID_SCALING.
 */
#ifdef	VALID_SCALING
#define	ACCOUNTS	 1000000
#define	BRANCHES	      10
#define	TELLERS		     100
#define	HISTORY		25920000
#endif

#ifdef	TINY
#define	ACCOUNTS	    1000
#define	BRANCHES	      10
#define	TELLERS		     100
#define	HISTORY		   10000
#endif

#ifdef	VERY_TINY
#define	ACCOUNTS	     500
#define	BRANCHES	      10
#define	TELLERS		      50
#define	HISTORY		    5000
#endif

#if !defined(VALID_SCALING) && !defined(TINY) && !defined(VERY_TINY)
#define	ACCOUNTS	  100000
#define	BRANCHES	      10
#define	TELLERS		     100
#define	HISTORY		  259200
#endif

#define	HISTORY_LEN	    100
#define	RECLEN		    100
#define	BEGID		1000000

typedef struct _defrec {
	u_int32_t	id;
	u_int32_t	balance;
	u_int8_t	pad[RECLEN - sizeof(u_int32_t) - sizeof(u_int32_t)];
} defrec;

typedef struct _histrec {
	u_int32_t	aid;
	u_int32_t	bid;
	u_int32_t	tid;
	u_int32_t	amount;
	u_int8_t	pad[RECLEN - 4 * sizeof(u_int32_t)];
} histrec;

char *progname = "ex_tpcb";			/* Program name. */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB_ENV *dbenv;
	int accounts, branches, seed, tellers, history;
	int ch, iflag, mpool, ntxns, ret, txn_no_sync, verbose;
	const char *home;

	home = "TESTDIR";
	accounts = branches = history = tellers = 0;
	iflag = mpool = ntxns = txn_no_sync = verbose = 0;
	seed = (int)time(NULL);

	while ((ch = getopt(argc, argv, "a:b:c:fh:in:S:s:t:v")) != EOF)
		switch (ch) {
		case 'a':			/* Number of account records */
			if ((accounts = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'b':			/* Number of branch records */
			if ((branches = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'c':			/* Cachesize in bytes */
			if ((mpool = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'f':			/* Fast mode: no txn sync. */
			txn_no_sync = 1;
			break;
		case 'h':			/* DB  home. */
			home = optarg;
			break;
		case 'i':			/* Initialize the test. */
			iflag = 1;
			break;
		case 'n':			/* Number of transactions */
			if ((ntxns = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'S':			/* Random number seed. */
			if ((seed = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 's':			/* Number of history records */
			if ((history = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 't':			/* Number of teller records */
			if ((tellers = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'v':			/* Verbose option. */
			verbose = 1;
			break;
		case '?':
		default:
			return (usage(progname));
		}
	argc -= optind;
	argv += optind;

	srand((u_int)seed);

	/* Initialize the database environment. */
	if ((dbenv = db_init(home,
	    progname, mpool, txn_no_sync ? DB_TXN_NOSYNC : 0)) == NULL)
		return (EXIT_FAILURE);

	accounts = accounts == 0 ? ACCOUNTS : accounts;
	branches = branches == 0 ? BRANCHES : branches;
	tellers = tellers == 0 ? TELLERS : tellers;
	history = history == 0 ? HISTORY : history;

	if (verbose)
		printf("%ld Accounts, %ld Branches, %ld Tellers, %ld History\n",
		    (long)accounts, (long)branches,
		    (long)tellers, (long)history);

	if (iflag) {
		if (ntxns != 0)
			return (usage(progname));
		tp_populate(dbenv,
		    accounts, branches, history, tellers, verbose);
	} else {
		if (ntxns == 0)
			return (usage(progname));
		tp_run(dbenv, ntxns, accounts, branches, tellers, verbose);
	}

	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "%s: dbenv->close failed: %s\n",
		    progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

int
invarg(progname, arg, str)
	const char *progname;
	int arg;
	const char *str;
{
	(void)fprintf(stderr,
	    "%s: invalid argument for -%c: %s\n", progname, arg, str);
	return (EXIT_FAILURE);
}

int
usage(progname)
	const char *progname;
{
	const char *a1, *a2;

	a1 = "[-fv] [-a accounts] [-b branches]\n";
	a2 = "\t[-c cache_size] [-h home] [-S seed] [-s history] [-t tellers]";
	(void)fprintf(stderr, "usage: %s -i %s %s\n", progname, a1, a2);
	(void)fprintf(stderr,
	    "       %s -n transactions %s %s\n", progname, a1, a2);
	return (EXIT_FAILURE);
}

/*
 * db_init --
 *	Initialize the environment.
 */
DB_ENV *
db_init(home, prefix, cachesize, flags)
	const char *home, *prefix;
	int cachesize;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	u_int32_t local_flags;
	int ret;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (NULL);
	}
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, prefix);
	(void)dbenv->set_lk_detect(dbenv, DB_LOCK_DEFAULT);
	(void)dbenv->set_cachesize(dbenv, 0,
	    cachesize == 0 ? 4 * 1024 * 1024 : (u_int32_t)cachesize, 0);

	if (flags & (DB_TXN_NOSYNC))
		(void)dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1);
	flags &= ~(DB_TXN_NOSYNC);

	local_flags = flags | DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN;
	if ((ret = dbenv->open(dbenv, home, local_flags, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open: %s", home);
		(void)dbenv->close(dbenv, 0);
		return (NULL);
	}
	return (dbenv);
}

/*
 * Initialize the database to the specified number of accounts, branches,
 * history records, and tellers.
 */
int
tp_populate(env, accounts, branches, history, tellers, verbose)
	DB_ENV *env;
	int accounts, branches, history, tellers, verbose;
{
	DB *dbp;
	u_int32_t balance, idnum, oflags;
	u_int32_t end_anum, end_bnum, end_tnum;
	u_int32_t start_anum, start_bnum, start_tnum;
	int ret;

	idnum = BEGID;
	balance = 500000;
	oflags = DB_CREATE;

	if ((ret = db_create(&dbp, env, 0)) != 0) {
		env->err(env, ret, "db_create");
		return (1);
	}
	(void)dbp->set_h_nelem(dbp, (u_int32_t)accounts);

	if ((ret = dbp->open(dbp, NULL, "account", NULL,
	    DB_HASH, oflags, 0644)) != 0) {
		env->err(env, ret, "DB->open: account");
		return (1);
	}

	start_anum = idnum;
	populate(dbp, idnum, balance, accounts, "account");
	idnum += accounts;
	end_anum = idnum - 1;
	if ((ret = dbp->close(dbp, 0)) != 0) {
		env->err(env, ret, "DB->close: account");
		return (1);
	}
	if (verbose)
		printf("Populated accounts: %ld - %ld\n",
		    (long)start_anum, (long)end_anum);

	/*
	 * Since the number of branches is very small, we want to use very
	 * small pages and only 1 key per page, i.e., key-locking instead
	 * of page locking.
	 */
	if ((ret = db_create(&dbp, env, 0)) != 0) {
		env->err(env, ret, "db_create");
		return (1);
	}
	(void)dbp->set_h_ffactor(dbp, 1);
	(void)dbp->set_h_nelem(dbp, (u_int32_t)branches);
	(void)dbp->set_pagesize(dbp, 512);
	if ((ret = dbp->open(dbp, NULL, "branch", NULL,
	    DB_HASH, oflags, 0644)) != 0) {
		env->err(env, ret, "DB->open: branch");
		return (1);
	}
	start_bnum = idnum;
	populate(dbp, idnum, balance, branches, "branch");
	idnum += branches;
	end_bnum = idnum - 1;
	if ((ret = dbp->close(dbp, 0)) != 0) {
		env->err(env, ret, "DB->close: branch");
		return (1);
	}
	if (verbose)
		printf("Populated branches: %ld - %ld\n",
		    (long)start_bnum, (long)end_bnum);

	/*
	 * In the case of tellers, we also want small pages, but we'll let
	 * the fill factor dynamically adjust itself.
	 */
	if ((ret = db_create(&dbp, env, 0)) != 0) {
		env->err(env, ret, "db_create");
		return (1);
	}
	(void)dbp->set_h_ffactor(dbp, 0);
	(void)dbp->set_h_nelem(dbp, (u_int32_t)tellers);
	(void)dbp->set_pagesize(dbp, 512);
	if ((ret = dbp->open(dbp, NULL, "teller", NULL,
	    DB_HASH, oflags, 0644)) != 0) {
		env->err(env, ret, "DB->open: teller");
		return (1);
	}

	start_tnum = idnum;
	populate(dbp, idnum, balance, tellers, "teller");
	idnum += tellers;
	end_tnum = idnum - 1;
	if ((ret = dbp->close(dbp, 0)) != 0) {
		env->err(env, ret, "DB->close: teller");
		return (1);
	}
	if (verbose)
		printf("Populated tellers: %ld - %ld\n",
		    (long)start_tnum, (long)end_tnum);

	if ((ret = db_create(&dbp, env, 0)) != 0) {
		env->err(env, ret, "db_create");
		return (1);
	}
	(void)dbp->set_re_len(dbp, HISTORY_LEN);
	if ((ret = dbp->open(dbp, NULL, "history", NULL,
	    DB_RECNO, oflags, 0644)) != 0) {
		env->err(env, ret, "DB->open: history");
		return (1);
	}

	hpopulate(dbp, history, accounts, branches, tellers);
	if ((ret = dbp->close(dbp, 0)) != 0) {
		env->err(env, ret, "DB->close: history");
		return (1);
	}
	return (0);
}

int
populate(dbp, start_id, balance, nrecs, msg)
	DB *dbp;
	u_int32_t start_id, balance;
	int nrecs;
	const char *msg;
{
	DBT kdbt, ddbt;
	defrec drec;
	int i, ret;

	kdbt.flags = 0;
	kdbt.data = &drec.id;
	kdbt.size = sizeof(u_int32_t);
	ddbt.flags = 0;
	ddbt.data = &drec;
	ddbt.size = sizeof(drec);
	memset(&drec.pad[0], 1, sizeof(drec.pad));

	for (i = 0; i < nrecs; i++) {
		drec.id = start_id + (u_int32_t)i;
		drec.balance = balance;
		if ((ret =
		    (dbp->put)(dbp, NULL, &kdbt, &ddbt, DB_NOOVERWRITE)) != 0) {
			dbp->err(dbp,
			    ret, "Failure initializing %s file\n", msg);
			return (1);
		}
	}
	return (0);
}

int
hpopulate(dbp, history, accounts, branches, tellers)
	DB *dbp;
	int history, accounts, branches, tellers;
{
	DBT kdbt, ddbt;
	histrec hrec;
	db_recno_t key;
	int i, ret;

	memset(&kdbt, 0, sizeof(kdbt));
	memset(&ddbt, 0, sizeof(ddbt));
	ddbt.data = &hrec;
	ddbt.size = sizeof(hrec);
	kdbt.data = &key;
	kdbt.size = sizeof(key);
	memset(&hrec.pad[0], 1, sizeof(hrec.pad));
	hrec.amount = 10;

	for (i = 1; i <= history; i++) {
		hrec.aid = random_id(ACCOUNT, accounts, branches, tellers);
		hrec.bid = random_id(BRANCH, accounts, branches, tellers);
		hrec.tid = random_id(TELLER, accounts, branches, tellers);
		if ((ret = dbp->put(dbp, NULL, &kdbt, &ddbt, DB_APPEND)) != 0) {
			dbp->err(dbp, ret, "dbp->put");
			return (1);
		}
	}
	return (0);
}

u_int32_t
random_int(lo, hi)
	u_int32_t lo, hi;
{
	u_int32_t ret;
	int t;

#ifndef RAND_MAX
#define	RAND_MAX	0x7fffffff
#endif
	t = rand();
	ret = (u_int32_t)(((double)t / ((double)(RAND_MAX) + 1)) *
	    (hi - lo + 1));
	ret += lo;
	return (ret);
}

u_int32_t
random_id(type, accounts, branches, tellers)
	FTYPE type;
	int accounts, branches, tellers;
{
	u_int32_t min, max, num;

	max = min = BEGID;
	num = accounts;
	switch (type) {
	case TELLER:
		min += branches;
		num = tellers;
		/* FALLTHROUGH */
	case BRANCH:
		if (type == BRANCH)
			num = branches;
		min += accounts;
		/* FALLTHROUGH */
	case ACCOUNT:
		max = min + num - 1;
	}
	return (random_int(min, max));
}

int
tp_run(dbenv, n, accounts, branches, tellers, verbose)
	DB_ENV *dbenv;
	int n, accounts, branches, tellers, verbose;
{
	DB *adb, *bdb, *hdb, *tdb;
	int failed, ret, txns;
	struct timeval start_tv, end_tv;
	double start_time, end_time;

	adb = bdb = hdb = tdb = NULL;

	/*
	 * Open the database files.
	 */
	if ((ret = db_create(&adb, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		goto err;
	}
	if ((ret = adb->open(adb, NULL, "account", NULL, DB_UNKNOWN,
	    DB_AUTO_COMMIT, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open: account");
		goto err;
	}
	if ((ret = db_create(&bdb, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		goto err;
	}
	if ((ret = bdb->open(bdb, NULL, "branch", NULL, DB_UNKNOWN,
	    DB_AUTO_COMMIT, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open: branch");
		goto err;
	}
	if ((ret = db_create(&hdb, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		goto err;
	}
	if ((ret = hdb->open(hdb, NULL, "history", NULL, DB_UNKNOWN,
	    DB_AUTO_COMMIT, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open: history");
		goto err;
	}
	if ((ret = db_create(&tdb, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		goto err;
	}
	if ((ret = tdb->open(tdb, NULL, "teller", NULL, DB_UNKNOWN,
	    DB_AUTO_COMMIT, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open: teller");
		goto err;
	}

	(void)gettimeofday(&start_tv, NULL);

	for (txns = n, failed = 0; n-- > 0;)
		if ((ret = tp_txn(dbenv, adb, bdb, tdb, hdb,
		    accounts, branches, tellers, verbose)) != 0)
			++failed;

	(void)gettimeofday(&end_tv, NULL);

	start_time = start_tv.tv_sec + ((start_tv.tv_usec + 0.0)/NS_PER_MS);
	end_time = end_tv.tv_sec + ((end_tv.tv_usec + 0.0)/NS_PER_MS);
	if (end_time == start_time)
		end_time += 1/NS_PER_MS;

	printf("%s: %d txns: %d failed, %.3f sec, %.2f TPS\n", progname,
	    txns, failed, (end_time - start_time),
	    (txns - failed) / (double)(end_time - start_time));

err:	if (adb != NULL)
		(void)adb->close(adb, 0);
	if (bdb != NULL)
		(void)bdb->close(bdb, 0);
	if (tdb != NULL)
		(void)tdb->close(tdb, 0);
	if (hdb != NULL)
		(void)hdb->close(hdb, 0);
	return (ret == 0 ? 0 : 1);
}

/*
 * XXX Figure out the appropriate way to pick out IDs.
 */
int
tp_txn(dbenv, adb, bdb, tdb, hdb, accounts, branches, tellers, verbose)
	DB_ENV *dbenv;
	DB *adb, *bdb, *tdb, *hdb;
	int accounts, branches, tellers, verbose;
{
	DBC *acurs, *bcurs, *tcurs;
	DBT d_dbt, d_histdbt, k_dbt, k_histdbt;
	DB_TXN *t;
	db_recno_t key;
	defrec rec;
	histrec hrec;
	int account, branch, teller, ret;

	t = NULL;
	acurs = bcurs = tcurs = NULL;

	/*
	 * !!!
	 * This is sample code -- we could move a lot of this into the driver
	 * to make it faster.
	 */
	account = random_id(ACCOUNT, accounts, branches, tellers);
	branch = random_id(BRANCH, accounts, branches, tellers);
	teller = random_id(TELLER, accounts, branches, tellers);

	memset(&d_histdbt, 0, sizeof(d_histdbt));

	memset(&k_histdbt, 0, sizeof(k_histdbt));
	k_histdbt.data = &key;
	k_histdbt.size = sizeof(key);

	memset(&k_dbt, 0, sizeof(k_dbt));
	k_dbt.size = sizeof(int);

	memset(&d_dbt, 0, sizeof(d_dbt));
	d_dbt.flags = DB_DBT_USERMEM;
	d_dbt.data = &rec;
	d_dbt.ulen = sizeof(rec);

	hrec.aid = account;
	hrec.bid = branch;
	hrec.tid = teller;
	hrec.amount = 10;
	/* Request 0 bytes since we're just positioning. */
	d_histdbt.flags = DB_DBT_PARTIAL;

	/*
	 * START PER-TRANSACTION TIMING.
	 *
	 * Technically, TPCB requires a limit on response time, you only get
	 * to count transactions that complete within 2 seconds.  That's not
	 * an issue for this sample application -- regardless, here's where
	 * the transaction begins.
	 */
	if (dbenv->txn_begin(dbenv, NULL, &t, 0) != 0)
		goto err;

	if (adb->cursor(adb, t, &acurs, 0) != 0 ||
	    bdb->cursor(bdb, t, &bcurs, 0) != 0 ||
	    tdb->cursor(tdb, t, &tcurs, 0) != 0)
		goto err;

	/* Account record */
	k_dbt.data = &account;
	if (acurs->get(acurs, &k_dbt, &d_dbt, DB_SET) != 0)
		goto err;
	rec.balance += 10;
	if (acurs->put(acurs, &k_dbt, &d_dbt, DB_CURRENT) != 0)
		goto err;

	/* Branch record */
	k_dbt.data = &branch;
	if (bcurs->get(bcurs, &k_dbt, &d_dbt, DB_SET) != 0)
		goto err;
	rec.balance += 10;
	if (bcurs->put(bcurs, &k_dbt, &d_dbt, DB_CURRENT) != 0)
		goto err;

	/* Teller record */
	k_dbt.data = &teller;
	if (tcurs->get(tcurs, &k_dbt, &d_dbt, DB_SET) != 0)
		goto err;
	rec.balance += 10;
	if (tcurs->put(tcurs, &k_dbt, &d_dbt, DB_CURRENT) != 0)
		goto err;

	/* History record */
	d_histdbt.flags = 0;
	d_histdbt.data = &hrec;
	d_histdbt.ulen = sizeof(hrec);
	if (hdb->put(hdb, t, &k_histdbt, &d_histdbt, DB_APPEND) != 0)
		goto err;

	if (acurs->close(acurs) != 0 || bcurs->close(bcurs) != 0 ||
	    tcurs->close(tcurs) != 0)
		goto err;

	ret = t->commit(t, 0);
	t = NULL;
	if (ret != 0)
		goto err;
	/* END PER-TRANSACTION TIMING. */

	return (0);

err:	if (acurs != NULL)
		(void)acurs->close(acurs);
	if (bcurs != NULL)
		(void)bcurs->close(bcurs);
	if (tcurs != NULL)
		(void)tcurs->close(tcurs);
	if (t != NULL)
		(void)t->abort(t);

	if (verbose)
		printf("Transaction A=%ld B=%ld T=%ld failed\n",
		    (long)account, (long)branch, (long)teller);
	return (-1);
}
