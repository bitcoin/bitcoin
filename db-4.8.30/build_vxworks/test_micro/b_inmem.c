/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "bench.h"

#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 0
/*
 * The in-memory tests don't run on early releases of Berkeley DB.
 */
#undef	MEGABYTE
#define	MEGABYTE	(1024 * 1024)

u_int32_t bulkbufsize = 4 * MEGABYTE;
u_int32_t cachesize = 32 * MEGABYTE;
u_int32_t datasize = 32;
u_int32_t keysize = 8;
u_int32_t logbufsize = 8 * MEGABYTE;
u_int32_t numitems;
u_int32_t pagesize = 32 * 1024;

FILE *fp;

static void b_inmem_op_ds __P((u_int, int));
static void b_inmem_op_ds_bulk __P((u_int, u_int *));
static void b_inmem_op_tds __P((u_int, int, u_int32_t, u_int32_t));
static int  b_inmem_usage __P((void));

static void
b_inmem_op_ds(u_int ops, int update)
{
	DB_ENV *dbenv;
	char *letters = "abcdefghijklmnopqrstuvwxuz";
	DB *dbp;
	DBT key, data;
	char *keybuf, *databuf;
	DB_MPOOL_STAT  *gsp;

	DB_BENCH_ASSERT((keybuf = malloc(keysize)) != NULL);
	DB_BENCH_ASSERT((databuf = malloc(datasize)) != NULL);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = keybuf;
	key.size = keysize;
	memset(keybuf, 'a', keysize);

	data.data = databuf;
	data.size = datasize;
	memset(databuf, 'b', datasize);

	DB_BENCH_ASSERT(db_create(&dbp, NULL, 0) == 0);
	dbenv = dbp->dbenv;
	dbp->set_errfile(dbp, stderr);

	DB_BENCH_ASSERT(dbp->set_pagesize(dbp, pagesize) == 0);
	DB_BENCH_ASSERT(dbp->open(
	    dbp, NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0666) == 0);

	(void)dbenv->memp_stat(dbenv, &gsp, NULL, DB_STAT_CLEAR);

	if (update) {
		TIMER_START;
		for (; ops > 0; --ops) {
			keybuf[(ops % keysize)] = letters[(ops % 26)];
			DB_BENCH_ASSERT(
			    dbp->put(dbp, NULL, &key, &data, 0) == 0);
		}
		TIMER_STOP;
	} else {
		DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);
		TIMER_START;
		for (; ops > 0; --ops)
			DB_BENCH_ASSERT(
			    dbp->get(dbp, NULL, &key, &data, 0) == 0);
		TIMER_STOP;
	}

	if (dbenv->memp_stat(dbenv, &gsp, NULL, 0) == 0)
		DB_BENCH_ASSERT(gsp->st_cache_miss == 0);

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);
}

static void
b_inmem_op_ds_bulk(u_int ops, u_int *totalp)
{
	DB_ENV *dbenv;
	DB *dbp;
	DBC *dbc;
	DBT key, data;
	u_int32_t len, klen;
	u_int i, total;
	char *keybuf, *databuf;
	void *pointer, *dp, *kp;
	DB_MPOOL_STAT  *gsp;

	DB_BENCH_ASSERT((keybuf = malloc(keysize)) != NULL);
	DB_BENCH_ASSERT((databuf = malloc(bulkbufsize)) != NULL);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = keybuf;
	key.size = keysize;

	data.data = databuf;
	data.size = datasize;
	memset(databuf, 'b', datasize);

	DB_BENCH_ASSERT(db_create(&dbp, NULL, 0) == 0);
	dbenv = dbp->dbenv;
	dbp->set_errfile(dbp, stderr);

	DB_BENCH_ASSERT(dbp->set_pagesize(dbp, pagesize) == 0);
	DB_BENCH_ASSERT(dbp->set_cachesize(dbp, 0, cachesize, 1) == 0);
	DB_BENCH_ASSERT(
	    dbp->open(dbp, NULL, NULL, NULL, DB_BTREE, DB_CREATE, 0666) == 0);

	for (i = 1; i <= numitems; ++i) {
		(void)snprintf(keybuf, keysize, "%7d", i);
		DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);
	}

#if 0
	fp = fopen("before", "w");
	dbp->set_msgfile(dbp, fp);
	DB_BENCH_ASSERT (dbp->stat_print(dbp, DB_STAT_ALL) == 0);
#endif

	DB_BENCH_ASSERT(dbp->cursor(dbp, NULL, &dbc, 0) == 0);

	data.ulen = bulkbufsize;
	data.flags = DB_DBT_USERMEM;

	(void)dbenv->memp_stat(dbenv, &gsp, NULL, DB_STAT_CLEAR);

	TIMER_START;
	for (total = 0; ops > 0; --ops) {
		DB_BENCH_ASSERT(dbc->c_get(
		    dbc, &key, &data, DB_FIRST | DB_MULTIPLE_KEY) == 0);
		DB_MULTIPLE_INIT(pointer, &data);
		while (pointer != NULL) {
			DB_MULTIPLE_KEY_NEXT(pointer, &data, kp, klen, dp, len);
			if (kp != NULL)
				++total;
		}
	}
	TIMER_STOP;
	*totalp = total;

	if (dbenv->memp_stat(dbenv, &gsp, NULL, 0) == 0)
	    DB_BENCH_ASSERT(gsp->st_cache_miss == 0);

#if 0
	fp = fopen("before", "w");
	dbp->set_msgfile(dbp, fp);
	DB_BENCH_ASSERT (dbp->stat_print(dbp, DB_STAT_ALL) == 0);
#endif

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);

	COMPQUIET(dp, NULL);
	COMPQUIET(klen, 0);
	COMPQUIET(len, 0);
}

static void
b_inmem_op_tds(u_int ops, int update, u_int32_t env_flags, u_int32_t log_flags)
{
	DB *dbp;
	DBT key, data;
	DB_ENV *dbenv;
	DB_MPOOL_STAT  *gsp;
	DB_TXN *txn;
	char *keybuf, *databuf;

	DB_BENCH_ASSERT((keybuf = malloc(keysize)) != NULL);
	DB_BENCH_ASSERT((databuf = malloc(datasize)) != NULL);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = keybuf;
	key.size = keysize;
	memset(keybuf, 'a', keysize);

	data.data = databuf;
	data.size = datasize;
	memset(databuf, 'b', datasize);

	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);

	dbenv->set_errfile(dbenv, stderr);

	/* General environment configuration. */
#ifdef DB_AUTO_COMMIT
	DB_BENCH_ASSERT(dbenv->set_flags(dbenv, DB_AUTO_COMMIT, 1) == 0);
#endif
	if (env_flags != 0)
		DB_BENCH_ASSERT(dbenv->set_flags(dbenv, env_flags, 1) == 0);

	/* Logging configuration. */
	if (log_flags != 0)
#if DB_VERSION_MINOR >= 7
		DB_BENCH_ASSERT(
		    dbenv->log_set_config(dbenv, log_flags, 1) == 0);
#else
		DB_BENCH_ASSERT(dbenv->set_flags(dbenv, log_flags, 1) == 0);
#endif
#ifdef DB_LOG_INMEMORY
	if (!(log_flags & DB_LOG_INMEMORY))
#endif
#ifdef DB_LOG_IN_MEMORY
	if (!(log_flags & DB_LOG_IN_MEMORY))
#endif
		DB_BENCH_ASSERT(dbenv->set_lg_max(dbenv, logbufsize * 10) == 0);
	DB_BENCH_ASSERT(dbenv->set_lg_bsize(dbenv, logbufsize) == 0);

	DB_BENCH_ASSERT(dbenv->open(dbenv, "TESTDIR",
	    DB_CREATE | DB_PRIVATE | DB_INIT_LOCK |
	    DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN, 0666) == 0);

	DB_BENCH_ASSERT(db_create(&dbp, dbenv, 0) == 0);
	DB_BENCH_ASSERT(dbp->set_pagesize(dbp, pagesize) == 0);
	DB_BENCH_ASSERT(dbp->open(
	    dbp, NULL, TESTFILE, NULL, DB_BTREE, DB_CREATE, 0666) == 0);

	if (update) {
		(void)dbenv->memp_stat(dbenv, &gsp, NULL, DB_STAT_CLEAR);

		TIMER_START;
		for (; ops > 0; --ops)
			DB_BENCH_ASSERT(
			    dbp->put(dbp, NULL, &key, &data, 0) == 0);
		TIMER_STOP;

		if (dbenv->memp_stat(dbenv, &gsp, NULL, 0) == 0)
			DB_BENCH_ASSERT(gsp->st_page_out == 0);
	} else {
		DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);
		(void)dbenv->memp_stat(dbenv, &gsp, NULL, DB_STAT_CLEAR);

		TIMER_START;
		for (; ops > 0; --ops) {
			DB_BENCH_ASSERT(
			    dbenv->txn_begin(dbenv, NULL, &txn, 0) == 0);
			DB_BENCH_ASSERT(
			    dbp->get(dbp, NULL, &key, &data, 0) == 0);
			DB_BENCH_ASSERT(txn->commit(txn, 0) == 0);
		}
		TIMER_STOP;

		if (dbenv->memp_stat(dbenv, &gsp, NULL, 0) == 0)
			DB_BENCH_ASSERT(gsp->st_cache_miss == 0);
	}

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);
	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);
}

#define	DEFAULT_OPS	1000000

int
b_inmem(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	u_int ops, total;
	int ch;

	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		++progname;

	ops = 0;
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "b:C:d:k:l:o:P:")) != EOF)
		switch (ch) {
		case 'b':
			bulkbufsize = (u_int32_t)atoi(optarg);
			break;
		case 'C':
			cachesize = (u_int32_t)atoi(optarg);
			break;
		case 'd':
			datasize = (u_int)atoi(optarg);
			break;
		case 'k':
			keysize = (u_int)atoi(optarg);
			break;
		case 'l':
			logbufsize = (u_int32_t)atoi(optarg);
			break;
		case 'o':
			ops = (u_int)atoi(optarg);
			break;
		case 'P':
			pagesize = (u_int32_t)atoi(optarg);
			break;
		case '?':
		default:
			return (b_inmem_usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		return (b_inmem_usage());

	numitems = (cachesize / (keysize + datasize - 1)) / 2;

	if (strcasecmp(argv[0], "read") == 0) {
		if (ops == 0)
			ops = DEFAULT_OPS;
		b_inmem_op_ds(ops, 0);
		printf(
	"# %u in-memory Btree database reads of %u/%u byte key/data pairs\n",
		    ops, keysize, datasize);
	} else if (strcasecmp(argv[0], "bulk") == 0) {
		if (keysize < 8) {
			fprintf(stderr,
		    "%s: bulk read requires a key size >= 10\n", progname);
			return (EXIT_FAILURE);
		}
		/*
		 * The ops value is the number of bulk operations, not key get
		 * operations.  Reduce the value so the test doesn't take so
		 * long, and use the returned number of retrievals as the ops
		 * value for timing purposes.
		 */
		if (ops == 0)
			ops = 100000;
		b_inmem_op_ds_bulk(ops, &total);
		ops = total;
		printf(
    "# %u bulk in-memory Btree database reads of %u/%u byte key/data pairs\n",
		    ops, keysize, datasize);
	} else if (strcasecmp(argv[0], "write") == 0) {
		if (ops == 0)
			ops = DEFAULT_OPS;
		b_inmem_op_ds(ops, 1);
		printf(
	"# %u in-memory Btree database writes of %u/%u byte key/data pairs\n",
		    ops, keysize, datasize);
	} else if (strcasecmp(argv[0], "txn-read") == 0) {
		if (ops == 0)
			ops = DEFAULT_OPS;
		b_inmem_op_tds(ops, 0, 0, 0);
		printf(
		"# %u transactional in-memory Btree database reads of %u/%u %s",
		    ops, keysize, datasize, "byte key/data pairs\n");
	} else if (strcasecmp(argv[0], "txn-write") == 0) {
		if (ops == 0)
			ops = DEFAULT_OPS;
#if defined(DB_LOG_INMEMORY) || defined(DB_LOG_IN_MEMORY)
#if defined(DB_LOG_INMEMORY)
		b_inmem_op_tds(ops, 1, 0, DB_LOG_INMEMORY);
#else
		b_inmem_op_tds(ops, 1, 0, DB_LOG_IN_MEMORY);
#endif
		printf(
	"# %u transactional in-memory logging Btree database writes of %u/%u%s",
		    ops, keysize, datasize, " byte key/data pairs\n");
#else
		return (EXIT_SUCCESS);
#endif
	} else if (strcasecmp(argv[0], "txn-nosync") == 0) {
		if (ops == 0)
			ops = DEFAULT_OPS;
		b_inmem_op_tds(ops, 1, DB_TXN_NOSYNC, 0);
		printf(
	"# %u transactional nosync logging Btree database writes of %u/%u %s",
		    ops, keysize, datasize, "byte key/data pairs\n");
	} else if (strcasecmp(argv[0], "txn-write-nosync") == 0) {
		if (ops == 0)
			ops = DEFAULT_OPS;
#ifdef DB_TXN_WRITE_NOSYNC
		b_inmem_op_tds(ops, 1, DB_TXN_WRITE_NOSYNC, 0);
		printf(
  "# %u transactional OS-write/nosync logging Btree database writes of %u/%u%s",
		    ops, keysize, datasize, " byte key/data pairs\n");
#else
		return (EXIT_SUCCESS);
#endif
	} else if (strcasecmp(argv[0], "txn-sync") == 0) {
		/*
		 * Flushing to disk takes a long time, reduce the number of
		 * default ops.
		 */
		if (ops == 0)
			ops = 100000;
		b_inmem_op_tds(ops, 1, 0, 0);
		printf(
	"# %u transactional logging Btree database writes of %u/%u %s",
		    ops, keysize, datasize, "byte key/data pairs\n");
	} else {
		fprintf(stderr, "%s: unknown keyword %s\n", progname, argv[0]);
		return (EXIT_FAILURE);
	}

	TIMER_DISPLAY(ops);
	return (EXIT_SUCCESS);
}

static int
b_inmem_usage()
{
	fprintf(stderr, "usage: %s %s%s%s%s",
	    progname, "[-b bulkbufsz] [-C cachesz]\n\t",
	    "[-d datasize] [-k keysize] [-l logbufsz] [-o ops] [-P pagesz]\n\t",
	    "[read | bulk | write | txn-read |\n\t",
	    "txn-write | txn-nosync | txn-write-nosync | txn-sync]\n");
	return (EXIT_FAILURE);
}
#else
int
b_inmem(int argc, char *argv[])
{
	COMPQUIET(argc, 0);
	COMPQUIET(argv, NULL);
	return (0);
}
#endif
