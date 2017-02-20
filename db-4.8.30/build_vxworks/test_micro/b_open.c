/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#include "bench.h"

static int b_open_usage(void);

int
b_open(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB_ENV *dbenv;
	DB *dbp;
	DBTYPE type;
	int ch, i, count;
	char *fname, *dbname, *ts;

	type = DB_BTREE;
	count = 1000;
	fname = dbname = NULL;
	ts = "Btree";
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "c:dft:")) != EOF)
		switch (ch) {
		case 'c':
			count = atoi(optarg);
			break;
		case 'd':
			dbname = "dbname";
			break;
		case 'f':
			fname = "filename";
			break;
		case 't':
			switch (optarg[0]) {
			case 'B': case 'b':
				ts = "Btree";
				type = DB_BTREE;
				break;
			case 'H': case 'h':
				if (b_util_have_hash())
					return (0);
				ts = "Hash";
				type = DB_HASH;
				break;
			case 'Q': case 'q':
				if (b_util_have_queue())
					return (0);
				ts = "Queue";
				type = DB_QUEUE;
				break;
			case 'R': case 'r':
				ts = "Recno";
				type = DB_RECNO;
				break;
			default:
				return (b_open_usage());
			}
			break;
		case '?':
		default:
			return (b_open_usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (b_open_usage());

#if DB_VERSION_MAJOR < 4
	/*
	 * Don't run in-memory database tests on versions less than 3, it
	 * takes forever and eats memory.
	 */
	if (fname == NULL && dbname == NULL)
		return (0);
#endif
#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 4
	/*
	 * Named in-memory databases weren't available until 4.4.
	 */
	if (fname == NULL && dbname != NULL)
		return (0);
#endif

	/* Create the environment. */
	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);
	dbenv->set_errfile(dbenv, stderr);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR == 0
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR,
	    NULL, DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR,
	    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0666) == 0);
#endif

	/* Create the database. */
	DB_BENCH_ASSERT(db_create(&dbp, dbenv, 0) == 0);

#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
	DB_BENCH_ASSERT(dbp->open(
	    dbp, NULL, fname, dbname, type, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbp->open(
	    dbp, fname, dbname, type, DB_CREATE, 0666) == 0);
#endif
	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);

	/* Open the database count times. */
	TIMER_START;
	for (i = 0; i < count; ++i) {
		DB_BENCH_ASSERT(db_create(&dbp, dbenv, 0) == 0);
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
		DB_BENCH_ASSERT(dbp->open(
		    dbp, NULL, fname, dbname, type, DB_CREATE, 0666) == 0);
#else
		DB_BENCH_ASSERT(dbp->open(
		    dbp, fname, dbname, type, DB_CREATE, 0666) == 0);
#endif
		DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);
	}
	TIMER_STOP;

	printf("# %d %s %sdatabase open/close pairs\n",
	    count, ts,
	    fname == NULL ?
		(dbname == NULL ? "in-memory " : "named in-memory ") :
		(dbname == NULL ? "" : "sub-"));
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);

	return (0);
}

static int
b_open_usage()
{
	(void)fprintf(stderr, "usage: b_open [-df] [-c count] [-t type]\n");
	return (EXIT_FAILURE);
}
