/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#include "bench.h"

static int b_put_usage(void);
static int b_put_secondary(DB *, const DBT *, const DBT *, DBT *);

int
b_put(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB_ENV *dbenv;
	DB *dbp, **second;
	DBTYPE type;
	DBT key, data;
	db_recno_t recno;
	u_int32_t cachesize, dsize;
	int ch, i, count, secondaries;
	char *ts, buf[64];

	second = NULL;
	type = DB_BTREE;
	cachesize = MEGABYTE;
	dsize = 20;
	count = 100000;
	secondaries = 0;
	ts = "Btree";
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "C:c:d:s:t:")) != EOF)
		switch (ch) {
		case 'C':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'd':
			dsize = (u_int32_t)atoi(optarg);
			break;
		case 's':
			secondaries = atoi(optarg);
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
				return (b_put_usage());
			}
			break;
		case '?':
		default:
			return (b_put_usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (b_put_usage());

#if DB_VERSION_MAJOR < 3 || DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 3
	/*
	 * Secondaries were added after DB 3.2.9.
	 */
	if (secondaries)
		return (0);
#endif

	/* Create the environment. */
	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);
	dbenv->set_errfile(dbenv, stderr);
	DB_BENCH_ASSERT(dbenv->set_cachesize(dbenv, 0, cachesize, 0) == 0);
#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR,
	    NULL, DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbenv->open(dbenv, TESTDIR,
	    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0666) == 0);
#endif

	/*
	 * Create the database.
	 * Optionally set the record length for Queue.
	 */
	DB_BENCH_ASSERT(db_create(&dbp, dbenv, 0) == 0);
	if (type == DB_QUEUE)
		DB_BENCH_ASSERT(dbp->set_re_len(dbp, dsize) == 0);
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
	DB_BENCH_ASSERT(
	    dbp->open(dbp, NULL, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(
	    dbp->open(dbp, TESTFILE, NULL, type, DB_CREATE, 0666) == 0);
#endif

	/* Optionally create the secondaries. */
	if (secondaries != 0) {
		DB_BENCH_ASSERT((second =
		    calloc(sizeof(DB *), (size_t)secondaries)) != NULL);
		for (i = 0; i < secondaries; ++i) {
			DB_BENCH_ASSERT(db_create(&second[i], dbenv, 0) == 0);
			(void)snprintf(buf, sizeof(buf), "%d.db", i);
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
			DB_BENCH_ASSERT(second[i]->open(second[i], NULL,
			    buf, NULL, DB_BTREE, DB_CREATE, 0600) == 0);
#else
			DB_BENCH_ASSERT(second[i]->open(second[i],
			    buf, NULL, DB_BTREE, DB_CREATE, 0600) == 0);
#endif
#if DB_VERSION_MAJOR > 3 || DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR >= 3
#if DB_VERSION_MAJOR > 3 && DB_VERSION_MINOR > 0
			/*
			 * The DB_TXN argument to Db.associate was added in
			 * 4.1.25.
			 */
			DB_BENCH_ASSERT(dbp->associate(
			    dbp, NULL, second[i], b_put_secondary, 0) == 0);
#else
			DB_BENCH_ASSERT(dbp->associate(
			    dbp, second[i], b_put_secondary, 0) == 0);
#endif
#endif
		}
	}

	/* Store a key/data pair. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	switch (type) {
	case DB_BTREE:
	case DB_HASH:
		key.data = "01234567890123456789";
		key.size = 20;
		break;
	case DB_QUEUE:
	case DB_RECNO:
		recno = 1;
		key.data = &recno;
		key.size = sizeof(recno);
		break;
	case DB_UNKNOWN:
		b_util_abort();
		break;
	}

	data.size = dsize;
	DB_BENCH_ASSERT(
	    (data.data = malloc((size_t)dsize)) != NULL);

	/* Store the key/data pair count times. */
	TIMER_START;
	for (i = 0; i < count; ++i) {
		/* Change data value so the secondaries are updated. */
		(void)snprintf(data.data, data.size, "%10lu", (u_long)i);
		DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);
	}
	TIMER_STOP;

	if (type == DB_BTREE || type == DB_HASH)
		printf(
		    "# %d %s database put of 10 byte key, %lu byte data",
		    count, ts, (u_long)dsize);
	else
		printf("# %d %s database put of key, %lu byte data",
		    count, ts, (u_long)dsize);
	if (secondaries)
		printf(" with %d secondaries", secondaries);
	printf("\n");
	TIMER_DISPLAY(count);

	if (second != NULL) {
		for (i = 0; i < secondaries; ++i)
			DB_BENCH_ASSERT(second[i]->close(second[i], 0) == 0);
		free(second);
	}

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);
	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);

	return (0);
}

static int
b_put_secondary(dbp, pkey, pdata, skey)
	DB *dbp;
	const DBT *pkey, *pdata;
	DBT *skey;
{
	skey->data = pdata->data;
	skey->size = pdata->size;

	COMPQUIET(dbp, NULL);
	COMPQUIET(pkey, NULL);
	return (0);
}

static int
b_put_usage()
{
	(void)fprintf(stderr, "usage: b_put %s\n",
	    "[-C cachesz] [-c count] [-d bytes] [-s secondaries] [-t type]");
	return (EXIT_FAILURE);
}
