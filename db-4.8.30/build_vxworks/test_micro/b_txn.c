/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#include "bench.h"

static int b_txn_usage(void);

int
b_txn(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB_ENV *dbenv;
	DB_TXN *txn;
	int tabort, ch, i, count;

	count = 1000;
	tabort = 0;
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "ac:")) != EOF)
		switch (ch) {
		case 'a':
			tabort = 1;
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case '?':
		default:
			return (b_txn_usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (b_txn_usage());

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

	/* Start and commit/abort a transaction count times. */
	TIMER_START;
	if (tabort)
		for (i = 0; i < count; ++i) {
#if DB_VERSION_MAJOR < 4
			DB_BENCH_ASSERT(txn_begin(dbenv, NULL, &txn, 0) == 0);
			DB_BENCH_ASSERT(txn_abort(txn) == 0);
#else
			DB_BENCH_ASSERT(
			    dbenv->txn_begin(dbenv, NULL, &txn, 0) == 0);
			DB_BENCH_ASSERT(txn->abort(txn) == 0);
#endif
		}
	else
		for (i = 0; i < count; ++i) {
#if DB_VERSION_MAJOR < 4
			DB_BENCH_ASSERT(txn_begin(dbenv, NULL, &txn, 0) == 0);
			DB_BENCH_ASSERT(txn_commit(txn, 0) == 0);
#else
			DB_BENCH_ASSERT(
			    dbenv->txn_begin(dbenv, NULL, &txn, 0) == 0);
			DB_BENCH_ASSERT(txn->commit(txn, 0) == 0);
#endif
		}
	TIMER_STOP;

	printf("# %d empty transaction start/%s pairs\n",
	    count, tabort ? "abort" : "commit");
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);

	return (0);
}

static int
b_txn_usage()
{
	(void)fprintf(stderr, "usage: b_txn [-a] [-c count]\n");
	return (EXIT_FAILURE);
}
