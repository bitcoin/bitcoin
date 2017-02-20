/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#include "bench.h"

static int usage(void);

int
b_recover(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DBT key, data;
	DB_ENV *dbenv;
	DB_TXN *txn;
	u_int32_t cachesize;
	int ch, i, count;

	/*
	 * Recover was too slow before release 4.0 that it's not worth
	 * running the test.
	 */
#if DB_VERSION_MAJOR < 4
	return (0);
#endif
	cachesize = MEGABYTE;
	count = 1000;
	while ((ch = getopt(argc, argv, "C:c:")) != EOF)
		switch (ch) {
		case 'C':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (usage());

	/* Create the environment. */
	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);
	dbenv->set_errfile(dbenv, stderr);
	DB_BENCH_ASSERT(dbenv->set_cachesize(dbenv, 0, cachesize, 0) == 0);

#define	OFLAGS								\
	(DB_CREATE | DB_INIT_LOCK |					\
	DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE)
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 0
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR, NULL, OFLAGS, 0666) == 0);
#endif
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 1
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR, OFLAGS, 0666) == 0);
#endif
#if DB_VERSION_MAJOR > 3 || DB_VERSION_MINOR > 1
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR, OFLAGS, 0666) == 0);
#endif

	/* Create the database. */
	DB_BENCH_ASSERT(db_create(&dbp, dbenv, 0) == 0);
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
	DB_BENCH_ASSERT(dbp->open(dbp, NULL,
	    TESTFILE, NULL, DB_BTREE, DB_CREATE | DB_AUTO_COMMIT, 0666) == 0);
#else
	DB_BENCH_ASSERT(
	    dbp->open(dbp, TESTFILE, NULL, DB_BTREE, DB_CREATE, 0666) == 0);
#endif

	/* Initialize the data. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.size = data.size = 20;
	key.data = data.data = "01234567890123456789";

	/* Start/commit a transaction count times. */
	for (i = 0; i < count; ++i) {
#if DB_VERSION_MAJOR < 4
		DB_BENCH_ASSERT(
		    txn_begin(dbenv, NULL, &txn, DB_TXN_NOSYNC) == 0);
		DB_BENCH_ASSERT(dbp->put(dbp, txn, &key, &data, 0) == 0);
		DB_BENCH_ASSERT(txn_commit(txn, 0) == 0);
#else
		DB_BENCH_ASSERT(
		    dbenv->txn_begin(dbenv, NULL, &txn, DB_TXN_NOSYNC) == 0);
		DB_BENCH_ASSERT(dbp->put(dbp, txn, &key, &data, 0) == 0);
		DB_BENCH_ASSERT(txn->commit(txn, 0) == 0);
#endif
	}

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);
	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);

	/* Create a new DB_ENV handle. */
	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);
	dbenv->set_errfile(dbenv, stderr);
	DB_BENCH_ASSERT(
	    dbenv->set_cachesize(dbenv, 0, 1048576 /* 1MB */, 0) == 0);

	/* Now run recovery. */
	TIMER_START;
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 0
	DB_BENCH_ASSERT(dbenv->open(
	    dbenv, TESTDIR, NULL, OFLAGS | DB_RECOVER, 0666) == 0);
#endif
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 1
	DB_BENCH_ASSERT(
	    dbenv->open(dbenv, TESTDIR, OFLAGS | DB_RECOVER, 0666) == 0);
#endif
#if DB_VERSION_MAJOR > 3 || DB_VERSION_MINOR > 1
	DB_BENCH_ASSERT(
	    dbenv->open(dbenv, TESTDIR, OFLAGS | DB_RECOVER, 0666) == 0);
#endif
	TIMER_STOP;

	/*
	 * We divide the time by the number of transactions, so an "operation"
	 * is the recovery of a single transaction.
	 */
	printf("# recovery after %d transactions\n", count);
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);

	return (0);
}

static int
usage()
{
	(void)fprintf(stderr, "usage: b_recover [-C cachesz] [-c count]\n");
	return (EXIT_FAILURE);
}
