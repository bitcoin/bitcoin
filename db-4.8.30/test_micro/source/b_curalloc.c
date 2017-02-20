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
b_curalloc(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DBC *curp;
	int ch, i, count;

	count = 100000;
	while ((ch = getopt(argc, argv, "c:")) != EOF)
		switch (ch) {
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

	/* Create the database. */
	DB_BENCH_ASSERT(db_create(&dbp, NULL, 0) == 0);
	dbp->set_errfile(dbp, stderr);

#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
	DB_BENCH_ASSERT(dbp->open(
	    dbp, NULL, TESTFILE, NULL, DB_BTREE, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(
	    dbp->open(dbp, TESTFILE, NULL, DB_BTREE, DB_CREATE, 0666) == 0);
#endif

	/* Allocate a cursor count times. */
	TIMER_START;
	for (i = 0; i < count; ++i) {
		DB_BENCH_ASSERT(dbp->cursor(dbp, NULL, &curp, 0) == 0);
		DB_BENCH_ASSERT(curp->c_close(curp) == 0);
	}
	TIMER_STOP;

	printf("# %d cursor allocations\n", count);
	TIMER_DISPLAY(count);

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);

	return (0);
}

static int
usage()
{
	(void)fprintf(stderr, "usage: b_curalloc [-c count]\n");
	return (EXIT_FAILURE);
}
