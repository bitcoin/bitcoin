/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "bench.h"
#include "b_workload.h"

static int   dump_verbose_stats __P((DB *, CONFIG *));
static int   is_del_workload __P((int));
static int   is_get_workload __P((int));
static int   is_put_workload __P((int));
static int   run_mixed_workload __P((DB *, CONFIG *));
static int   run_std_workload __P((DB *, CONFIG *));
static int   usage __P((void));
static char *workload_str __P((int));

/*
 * General TODO list:
 * * The workload type. Might work better as a bitmask than the current enum.
 * * Improve the verbose stats, so they can be easily parsed.
 * * Think about doing automatic btree/hash comparison in here.
 */
int
b_workload(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	CONFIG conf;
	DB *dbp;
	DB_ENV *dbenv;
	int ch, ffactor, ksz;

	dbenv = NULL;
	memset(&conf, 0, sizeof(conf));
	conf.seed = 124087;
	srand(conf.seed);

	conf.pcount = 100000;
	conf.ts = "Btree";
	conf.type = DB_BTREE;
	conf.dsize = 20;
	conf.presize = 0;
	conf.workload = T_PUT_GET_DELETE;

	while ((ch = getopt(argc, argv, "b:c:d:e:g:ik:m:op:r:t:vw:")) != EOF)
		switch (ch) {
		case 'b':
			conf.cachesz = atoi(optarg);
			break;
		case 'c':
			conf.pcount = atoi(optarg);
			break;
		case 'd':
			conf.dsize = atoi(optarg);
			break;
		case 'e':
			conf.cursor_del = atoi(optarg);
			break;
		case 'g':
			conf.gcount = atoi(optarg);
			break;
		case 'i':
			conf.presize = 1;
			break;
		case 'k':
			conf.ksize = atoi(optarg);
			break;
		case 'm':
			conf.message = optarg;
			break;
		case 'o':
			conf.orderedkeys = 1;
			break;
		case 'p':
			conf.pagesz = atoi(optarg);
			break;
		case 'r':
			conf.num_dups = atoi(optarg);
			break;
		case 't':
			switch (optarg[0]) {
			case 'B': case 'b':
				conf.ts = "Btree";
				conf.type = DB_BTREE;
				break;
			case 'H': case 'h':
				if (b_util_have_hash())
					return (0);
				conf.ts = "Hash";
				conf.type = DB_HASH;
				break;
			default:
				return (usage());
			}
			break;
		case 'v':
			conf.verbose = 1;
			break;
		case 'w':
			switch (optarg[0]) {
			case 'A':
				conf.workload = T_PUT_GET_DELETE;
				break;
			case 'B':
				conf.workload = T_GET;
				break;
			case 'C':
				conf.workload = T_PUT;
				break;
			case 'D':
				conf.workload = T_DELETE;
				break;
			case 'E':
				conf.workload = T_PUT_GET;
				break;
			case 'F':
				conf.workload = T_PUT_DELETE;
				break;
			case 'G':
				conf.workload = T_GET_DELETE;
				break;
			case 'H':
				conf.workload = T_MIXED;
				break;
			default:
				return (usage());
			}
			break;
		case '?':
		default:
			fprintf(stderr, "Invalid option: %c\n", ch);
			return (usage());
		}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		return (usage());

	/*
	 * Validate the input parameters if specified.
	 */
	if (conf.pagesz != 0)
		DB_BENCH_ASSERT(conf.pagesz >= 512 && conf.pagesz <= 65536 &&
		   ((conf.pagesz & (conf.pagesz - 1)) == 0));

	if (conf.cachesz != 0)
		DB_BENCH_ASSERT(conf.cachesz > 20480);
	DB_BENCH_ASSERT(conf.ksize == 0 || conf.orderedkeys == 0);

	/* Create the environment. */
	DB_BENCH_ASSERT(db_env_create(&dbenv, 0) == 0);
	dbenv->set_errfile(dbenv, stderr);
	if (conf.cachesz != 0)
		DB_BENCH_ASSERT(
		    dbenv->set_cachesize(dbenv, 0, conf.cachesz, 0) == 0);

#if DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR < 1
	DB_BENCH_ASSERT(dbenv->open(dbenv, "TESTDIR",
	    NULL, DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbenv->open(dbenv, "TESTDIR",
	    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE, 0666) == 0);
#endif

	DB_BENCH_ASSERT(db_create(&dbp, dbenv, 0) == 0);
	if (conf.pagesz != 0)
		DB_BENCH_ASSERT(
		    dbp->set_pagesize(dbp, conf.pagesz) == 0);
	if (conf.presize != 0 && conf.type == DB_HASH) {
		ksz = (conf.orderedkeys != 0) ? sizeof(u_int32_t) : conf.ksize;
		if (ksz == 0)
			ksz = 10;
		ffactor = (conf.pagesz - 32)/(ksz + conf.dsize + 8);
		fprintf(stderr, "ffactor: %d\n", ffactor);
		DB_BENCH_ASSERT(
		    dbp->set_h_ffactor(dbp, ffactor) == 0);
		DB_BENCH_ASSERT(
		    dbp->set_h_nelem(dbp, conf.pcount*10) == 0);
	}
#if DB_VERSION_MAJOR >= 4 && DB_VERSION_MINOR >= 1
	DB_BENCH_ASSERT(dbp->open(
	    dbp, NULL, TESTFILE, NULL, conf.type, DB_CREATE, 0666) == 0);
#else
	DB_BENCH_ASSERT(dbp->open(
	    dbp, TESTFILE, NULL, conf.type, DB_CREATE, 0666) == 0);
#endif

	if (conf.workload == T_MIXED)
		 run_mixed_workload(dbp, &conf);
	else
		run_std_workload(dbp, &conf);

	if (is_put_workload(conf.workload) == 0)
		timespecadd(&conf.tot_time, &conf.put_time);
	if (is_get_workload(conf.workload) == 0)
		timespecadd(&conf.tot_time, &conf.get_time);
	if (is_del_workload(conf.workload) == 0)
		timespecadd(&conf.tot_time, &conf.del_time);

	/* Ensure data is flushed for following measurements. */
	DB_BENCH_ASSERT(dbp->sync(dbp, 0) == 0);

	if (conf.verbose != 0)
		dump_verbose_stats(dbp, &conf);

	DB_BENCH_ASSERT(dbp->close(dbp, 0) == 0);
	DB_BENCH_ASSERT(dbenv->close(dbenv, 0) == 0);

	/*
	 * Construct a string for benchmark output.
	 *
	 * Insert HTML in-line to make the output prettier -- ugly, but easy.
	 */
	printf("# workload test: %s: %s<br>%lu ops",
	    conf.ts, workload_str(conf.workload), (u_long)conf.pcount);
	if (conf.ksize != 0)
		printf(", key size: %lu", (u_long)conf.ksize);
	if (conf.dsize != 0)
		printf(", data size: %lu", (u_long)conf.dsize);
	if (conf.pagesz != 0)
		printf(", page size: %lu", (u_long)conf.pagesz);
	else
		printf(", page size: default");
	if (conf.cachesz != 0)
		printf(", cache size: %lu", (u_long)conf.cachesz);
	else
		printf(", cache size: default");
	printf(", %s keys", conf.orderedkeys == 1 ? "ordered" : "unordered");
	printf(", num dups: %lu", (u_long)conf.num_dups);
	printf("\n");

	if (conf.workload != T_MIXED) {
		if (conf.message != NULL)
			printf("%s %s ", conf.message, conf.ts);
		TIME_DISPLAY(conf.pcount, conf.tot_time);
	} else
		TIMER_DISPLAY(conf.pcount);

	return (0);
}

/*
 * The mixed workload is designed to simulate a somewhat real
 * usage scenario.
 * NOTES: * rand is used to decide on the current operation. This will
 *        be repeatable, since the same seed is always used.
 *        * All added keys are stored in a FIFO queue, this is not very
 *        space efficient, but is the best way I could come up with to
 *        insert random key values, and be able to retrieve/delete them.
 *        * TODO: the workload will currently only work with unordered
 *        fixed length keys.
 */
#define	GET_PROPORTION 90
#define	PUT_PROPORTION 7
#define	DEL_PROPORTION 3

static int
run_mixed_workload(dbp, config)
	DB *dbp;
	CONFIG *config;
{
	DBT key, data;
	size_t next_op, i, ioff, inscount;
	char kbuf[KBUF_LEN];
	struct bench_q operation_queue;

	/* Having ordered insertion does not make sense here */
	DB_BENCH_ASSERT(config->orderedkeys == 0);

	srand(config->seed);
	memset(&operation_queue, 0, sizeof(struct bench_q));

	ioff = 0;
	INIT_KEY(key, config);
	memset(&data, 0, sizeof(data));
	DB_BENCH_ASSERT(
	    (data.data = malloc(data.size = config->dsize)) != NULL);

	/*
	 * Add an initial sample set of data to the DB.
	 * This should add some stability, and reduce the likelihood
	 * of deleting all of the entries in the DB.
	 */
	inscount = 2 * config->pcount;
	if (inscount > 100000)
		inscount = 100000;

	for (i = 0; i < inscount; ++i) {
		GET_KEY_NEXT(key, config, kbuf, i);
		BENCH_Q_TAIL_INSERT(operation_queue, kbuf);
		DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);
	}

	TIMER_START;
	for (i = 0; i < config->pcount; ++i) {
		next_op = rand()%100;

		if (next_op < GET_PROPORTION ) {
			BENCH_Q_POP_PUSH(operation_queue, kbuf);
			key.data = kbuf;
			key.size = sizeof(kbuf);
			dbp->get(dbp, NULL, &key, &data, 0);
		} else if (next_op < GET_PROPORTION+PUT_PROPORTION) {
			GET_KEY_NEXT(key, config, kbuf, i);
			BENCH_Q_TAIL_INSERT(operation_queue, kbuf);
			dbp->put(dbp, NULL, &key, &data, 0);
		} else {
			BENCH_Q_POP(operation_queue, kbuf);
			key.data = kbuf;
			key.size = sizeof(kbuf);
			dbp->del(dbp, NULL, &key, 0);
		}
	}
	TIMER_STOP;
	TIMER_GET(config->tot_time);

	return (0);
}

static int
run_std_workload(dbp, config)
	DB *dbp;
	CONFIG *config;
{
	DBT key, data;
	DBC *dbc;
	u_int32_t i;
	int ret;
	char kbuf[KBUF_LEN];

	/* Setup a key/data pair. */
	INIT_KEY(key, config);
	memset(&data, 0, sizeof(data));
	DB_BENCH_ASSERT(
	    (data.data = malloc(data.size = config->dsize)) != NULL);

	/* Store the key/data pair count times. */
	TIMER_START;
	for (i = 0; i < config->pcount; ++i) {
		GET_KEY_NEXT(key, config, kbuf, i);
		DB_BENCH_ASSERT(dbp->put(dbp, NULL, &key, &data, 0) == 0);
	}
	TIMER_STOP;
	TIMER_GET(config->put_time);

	if (is_get_workload(config->workload) == 0) {
		TIMER_START;
		for (i = 0; i <= config->gcount; ++i) {
			DB_BENCH_ASSERT(dbp->cursor(dbp, NULL, &dbc, 0) == 0);
			while ((dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0);
			DB_BENCH_ASSERT(dbc->c_close(dbc) == 0);
		}
		TIMER_STOP;
		TIMER_GET(config->get_time);
	}

	if (is_del_workload(config->workload) == 0) {
		/* reset rand to reproduce key sequence. */
		srand(config->seed);

		TIMER_START;
		if (config->cursor_del != 0) {
			DB_BENCH_ASSERT(dbp->cursor(dbp, NULL, &dbc, 0) == 0);
			while (
			    (ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0)
				DB_BENCH_ASSERT(dbc->c_del(dbc, 0) == 0);
			DB_BENCH_ASSERT (ret == DB_NOTFOUND);
		} else {
			INIT_KEY(key, config);
			for (i = 0; i < config->pcount; ++i) {
				GET_KEY_NEXT(key, config, kbuf, i);

				ret = dbp->del(dbp, NULL, &key, 0);
				/*
				 * Random key generation can cause dups,
				 * so NOTFOUND result is OK.
				 */
				if (config->ksize == 0)
					DB_BENCH_ASSERT
					    (ret == 0 || ret == DB_NOTFOUND);
				else
					DB_BENCH_ASSERT(ret == 0);
			}
		}
		TIMER_STOP;
		TIMER_GET(config->del_time);
	}
	return (0);
}

static int
dump_verbose_stats(dbp, config)
	DB *dbp;
	CONFIG *config;
{
/*
 * It would be nice to be able to define stat as _stat on
 * Windows, but that substitutes _stat for the db call as well.
 */
#ifdef DB_WIN32
	struct _stat fstat;
#else
	struct stat fstat;
#endif
	DB_HASH_STAT *hstat;
	DB_BTREE_STAT *bstat;
	double free_prop;
	char path[1024];

#ifdef DB_BENCH_INCLUDE_CONFIG_SUMMARY
	printf("Completed workload benchmark.\n");
	printf("Configuration summary:\n");
	printf("\tworkload type: %d\n", (int)config->workload);
	printf("\tdatabase type: %s\n", config->ts);
	if (config->cachesz != 0)
		printf("\tcache size: %lu\n", (u_long)config->cachesz);
	if (config->pagesz != 0)
		printf("\tdatabase page size: %lu\n", (u_long)config->pagesz);
	printf("\tput element count: %lu\n", (u_long)config->pcount);
	if ( is_get_workload(config->workload) == 0)
		printf("\tget element count: %lu\n", (u_long)config->gcount);
	if (config->orderedkeys)
		printf("\tInserting items in order\n");
	else if (config->ksize == 0)
		printf("\tInserting keys with size 10\n");
	else
		printf(
		    "\tInserting keys with size: %lu\n", (u_long)config->ksize);

	printf("\tInserting data elements size: %lu\n", (u_long)config->dsize);

	if (is_del_workload(config->workload) == 0) {
		if (config->cursor_del)
			printf("\tDeleting items using a cursor\n");
		else
			printf("\tDeleting items without a cursor\n");
	}
#endif /* DB_BENCH_INCLUDE_CONFIG_SUMMARY */

	if (is_put_workload(config->workload) == 0)
		printf("%s Time spent inserting (%lu) (%s) items: %lu/%lu\n",
		    config->message[0] == '\0' ? "" : config->message,
		    (u_long)config->pcount, config->ts,
		    (u_long)config->put_time.tv_sec, config->put_time.tv_nsec);

	if (is_get_workload(config->workload) == 0)
		printf("%s Time spent getting (%lu) (%s) items: %lu/%lu\n",
		    config->message[0] == '\0' ? "" : config->message,
		    (u_long)config->pcount * ((config->gcount == 0) ?
		    1 : config->gcount), config->ts,
		    (u_long)config->get_time.tv_sec, config->get_time.tv_nsec);

	if (is_del_workload(config->workload) == 0)
		printf("%s Time spent deleting (%lu) (%s) items: %lu/%lu\n",
		    config->message[0] == '\0' ? "" : config->message,
		    (u_long)config->pcount, config->ts,
		    (u_long)config->del_time.tv_sec, config->del_time.tv_nsec);

	(void)snprintf(path, sizeof(path),
	    "%s%c%s", TESTDIR, PATH_SEPARATOR[0], TESTFILE);
#ifdef DB_WIN32
	if (_stat(path, &fstat) == 0) {
#else
	if (stat(path, &fstat) == 0) {
#endif
		printf("%s Size of db file (%s): %lu K\n",
		    config->message[0] == '\0' ? "" : config->message,
		    config->ts, (u_long)fstat.st_size/1024);
	}

	if (config->type == DB_HASH) {
#if DB_VERSION_MAJOR < 3 || DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR <= 2
		DB_BENCH_ASSERT(dbp->stat(dbp, &hstat, NULL, 0) == 0);
#elif DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR <= 2
		DB_BENCH_ASSERT(dbp->stat(dbp, &hstat, 0) == 0);
#else
		DB_BENCH_ASSERT(dbp->stat(dbp, NULL, &hstat, 0) == 0);
#endif
		/*
		 * Hash fill factor is a bit tricky. Want to include
		 * both bucket and overflow buckets (not offpage).
		 */
		free_prop = hstat->hash_pagesize*hstat->hash_buckets;
		free_prop += hstat->hash_pagesize*hstat->hash_overflows;
		free_prop =
		    (free_prop - hstat->hash_bfree - hstat->hash_ovfl_free)/
		    free_prop;
		printf("%s db fill factor (%s): %.2f%%\n",
		    config->message[0] == '\0' ? "" : config->message,
		    config->ts, free_prop*100);
		free(hstat);
	} else { /* Btree */
#if DB_VERSION_MAJOR < 3 || DB_VERSION_MAJOR == 3 && DB_VERSION_MINOR <= 2
		DB_BENCH_ASSERT(dbp->stat(dbp, &bstat, NULL, 0) == 0);
#elif DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR <= 2
		DB_BENCH_ASSERT(dbp->stat(dbp, &bstat, 0) == 0);
#else
		DB_BENCH_ASSERT(dbp->stat(dbp, NULL, &bstat, 0) == 0);
#endif
		free_prop = bstat->bt_pagesize*bstat->bt_leaf_pg;
		free_prop = (free_prop-bstat->bt_leaf_pgfree)/free_prop;
		printf("%s db fill factor (%s): %.2f%%\n",
		    config->message[0] == '\0' ? "" : config->message,
		    config->ts, free_prop*100);
		free(bstat);
	}
	return (0);
}

static char *
workload_str(workload)
	int workload;
{
	static char buf[128];

	switch (workload) {
	case T_PUT_GET_DELETE:
		return ("PUT/GET/DELETE");
		/* NOTREACHED */
	case T_GET:
		return ("GET");
		/* NOTREACHED */
	case T_PUT:
		return ("PUT");
		/* NOTREACHED */
	case T_DELETE:
		return ("DELETE");
		/* NOTREACHED */
	case T_PUT_GET:
		return ("PUT/GET");
		/* NOTREACHED */
	case T_PUT_DELETE:
		return ("PUT/DELETE");
		/* NOTREACHED */
	case T_GET_DELETE:
		return ("GET/DELETE");
		/* NOTREACHED */
	case T_MIXED:
		snprintf(buf, sizeof(buf), "MIXED (get: %d, put: %d, del: %d)",
		    (int)GET_PROPORTION,
		    (int)PUT_PROPORTION, (int)DEL_PROPORTION);
		return (buf);
	default:
		break;
	}

	exit(usage());
	/* NOTREACHED */
}

static int
is_get_workload(workload)
	int workload;
{
	switch (workload) {
	case T_GET:
	case T_PUT_GET:
	case T_PUT_GET_DELETE:
	case T_GET_DELETE:
		return 0;
	}
	return 1;
}

static int
is_put_workload(workload)
	int workload;
{
	switch (workload) {
	case T_PUT:
	case T_PUT_GET:
	case T_PUT_GET_DELETE:
	case T_PUT_DELETE:
		return 0;
	}
	return 1;
}

static int
is_del_workload(workload)
	int workload;
{
	switch (workload) {
	case T_DELETE:
	case T_PUT_DELETE:
	case T_PUT_GET_DELETE:
	case T_GET_DELETE:
		return 0;
	}
	return 1;
}

static int
usage()
{
	(void)fprintf(stderr,
	    "usage: b_workload [-b cachesz] [-c count] [-d bytes] [-e]\n");
	(void)fprintf(stderr,
	    "\t[-g getitrs] [-i] [-k keysize] [-m message] [-o] [-p pagesz]\n");
	(void)fprintf(stderr, "\t[-r dup_count] [-t type] [-w type]\n");

	(void)fprintf(stderr, "Where:\n");
	(void)fprintf(stderr, "\t-b the size of the DB cache.\n");
	(void)fprintf(stderr, "\t-c the number of elements to be measured.\n");
	(void)fprintf(stderr, "\t-d the size of each data element.\n");
	(void)fprintf(stderr, "\t-e delete entries using a cursor.\n");
	(void)fprintf(stderr, "\t-g number of get cursor traverses.\n");
	(void)fprintf(stderr, "\t-i Pre-init hash DB bucket count.\n");
	(void)fprintf(stderr, "\t-k the size of each key inserted.\n");
	(void)fprintf(stderr, "\t-m message pre-pended to log output.\n");
	(void)fprintf(stderr, "\t-o keys should be ordered for insert.\n");
	(void)fprintf(stderr, "\t-p the page size for the database.\n");
	(void)fprintf(stderr, "\t-r the number of duplicates to insert\n");
	(void)fprintf(stderr, "\t-t type of the underlying database.\n");
	(void)fprintf(stderr, "\t-w the workload to measure, available:\n");
	(void)fprintf(stderr, "\t\tA - PUT_GET_DELETE\n");
	(void)fprintf(stderr, "\t\tB - GET\n");
	(void)fprintf(stderr, "\t\tC - PUT\n");
	(void)fprintf(stderr, "\t\tD - DELETE\n");
	(void)fprintf(stderr, "\t\tE - PUT_GET\n");
	(void)fprintf(stderr, "\t\tF - PUT_DELETE\n");
	(void)fprintf(stderr, "\t\tG - GET_DELETE\n");
	(void)fprintf(stderr, "\t\tH - MIXED\n");
	return (EXIT_FAILURE);
}
