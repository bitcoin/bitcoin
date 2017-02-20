/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * bench_001 - time bulk fetch interface.
 *	Without -R builds a btree acording to the arguments.
 *	With -R runs and times bulk fetches.  If -d is specified
 *	during reads the DB_MULTIPLE interface is used
 *	otherwise the DB_MULTIPLE_KEY interface is used.
 *
 * ARGUMENTS:
 *	-c	cachesize [1000 * pagesize]
 *	-d	number of duplicates [none]
 *	-E	don't use environment
 *	-I	Just initialize the environment
 *	-i	number of read iterations [1000000]
 *	-l	length of data item [20]
 *	-n	number of keys [1000000]
 *	-p	pagesize [65536]
 *	-R	perform read test.
 *	-T	incorporate transactions.
 *
 * COMPILE:
 *	cc -I /usr/local/BerkeleyDB/include \
 *	    -o bench_001 -O2 bench_001.c /usr/local/BerkeleyDB/lib/libdb.so
 */
#include <sys/types.h>
#include <sys/time.h>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

#define	DATABASE	"bench_001.db"

int	compare_int(DB *, const DBT *, const DBT *);
DB_ENV *db_init(char *, char *, u_int, int);
int	fill(DB_ENV *, DB *, int, u_int, int, int);
int	get(DB *, int, u_int, int, int, int, int *);
int	main(int, char *[]);
void	usage(void);

const char
	*progname = "bench_001";		/* Program name. */
/*
 * db_init --
 *	Initialize the environment.
 */
DB_ENV *
db_init(home, prefix, cachesize, txn)
	char *home, *prefix;
	u_int cachesize;
	int txn;
{
	DB_ENV *dbenv;
	u_int32_t flags;
	int ret;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_env_create");
		return (NULL);
	}
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, prefix);
	(void)dbenv->set_cachesize(dbenv, 0,
	    cachesize == 0 ? 50 * 1024 * 1024 : (u_int32_t)cachesize, 0);

	flags = DB_CREATE | DB_INIT_MPOOL;
	if (txn)
		flags |= DB_INIT_TXN | DB_INIT_LOCK;
	if ((ret = dbenv->open(dbenv, home, flags, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open: %s", home);
		(void)dbenv->close(dbenv, 0);
		return (NULL);
	}
	return (dbenv);
}

/*
 * get -- loop getting batches of records.
 *
 */
int
get(dbp, txn, datalen, num, dups, iter, countp)
	DB *dbp;
	u_int datalen;
	int txn, num, dups, iter, *countp;
{
	DBC *dbcp;
	DBT key, data;
	DB_ENV *dbenv;
	DB_TXN *txnp;
	u_int32_t flags, len, klen;
	int count, i, j, ret;
	void *pointer, *dp, *kp;

	dbenv = dbp->dbenv;

	klen = 0;				/* Lint. */
	klen = klen;

	memset(&key, 0, sizeof(key));
	key.data = &j;
	key.size = sizeof(j);
	memset(&data, 0, sizeof(data));
	data.flags = DB_DBT_USERMEM;
	data.data = malloc(datalen*1024*1024);
	data.ulen = data.size = datalen*1024*1024;

	count = 0;
	flags = DB_SET;
	if (!dups)
		flags |= DB_MULTIPLE_KEY;
	else
		flags |= DB_MULTIPLE;
	for (i = 0; i < iter; i++) {
		txnp = NULL;
		if (txn)
			if ((ret =
			    dbenv->txn_begin(dbenv, NULL, &txnp, 0)) != 0)
				goto err;
		if ((ret = dbp->cursor(dbp, txnp, &dbcp, 0)) != 0)
			goto err;

		j = random() % num;
		if ((ret = dbcp->get(dbcp, &key, &data, flags)) != 0)
			goto err;
		DB_MULTIPLE_INIT(pointer, &data);
		if (dups)
			while (pointer != NULL) {
				DB_MULTIPLE_NEXT(pointer, &data, dp, len);
				if (dp != NULL)
					count++;
			}
		else
			while (pointer != NULL) {
				DB_MULTIPLE_KEY_NEXT(pointer,
				    &data, kp, klen, dp, len);
				if (kp != NULL)
					count++;
			}
		if ((ret = dbcp->close(dbcp)) != 0)
			goto err;
		if (txn)
			if ((ret = txnp->commit(txnp, 0)) != 0)
				goto err;
	}

	*countp = count;
	return (0);

err:	dbp->err(dbp, ret, "get");
	return (ret);
}

/*
 * fill - fill a db
 *	Since we open/created the db with transactions (potentially),
 * we need to populate it with transactions.  We'll bundle the puts
 * 10 to a transaction.
 */
#define	PUTS_PER_TXN	10
int
fill(dbenv, dbp, txn, datalen, num, dups)
	DB_ENV *dbenv;
	DB *dbp;
	u_int datalen;
	int txn, num, dups;
{
	DBT key, data;
	DB_TXN *txnp;
	struct data {
		int id;
		char str[1];
	} *data_val;
	int count, i, ret;

	/*
	 * Insert records into the database, where the key is the user
	 * input and the data is the user input in reverse order.
	 */
	txnp = NULL;
	ret = 0;
	count = 0;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	key.data = &i;
	key.size = sizeof(i);
	data.data = data_val = malloc(datalen);
	memcpy(data_val->str, "0123456789012345678901234567890123456789",
	    datalen - sizeof(data_val->id));
	data.size = datalen;
	data.flags = DB_DBT_USERMEM;

	for (i = 0; i < num; i++) {
		if (txn != 0 && i % PUTS_PER_TXN == 0) {
			if (txnp != NULL) {
				ret = txnp->commit(txnp, 0);
				txnp = NULL;
				if (ret != 0)
					goto err;
			}
			if ((ret =
			    dbenv->txn_begin(dbenv, NULL, &txnp, 0)) != 0)
				goto err;
		}
		data_val->id = 0;
		do {
			switch (ret = dbp->put(dbp, txnp, &key, &data, 0)) {
			case 0:
				count++;
				break;
			default:
				dbp->err(dbp, ret, "DB->put");
				goto err;
			}
		} while (++data_val->id < dups);
	}
	if (txnp != NULL)
		ret = txnp->commit(txnp, 0);

	printf("%d\n", count);
	return (ret);

err:	if (txnp != NULL)
		(void)txnp->abort(txnp);
	return (ret);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DB_ENV *dbenv;
	DB_TXN *txnp;
	struct timeval start_time, end_time;
	double secs;
	u_int cache, datalen, pagesize;
	int ch, count, dups, env, init, iter, num;
	int ret, rflag, txn;

	txnp = NULL;
	datalen = 20;
	iter = num = 1000000;
	env = 1;
	dups = init = rflag = txn = 0;

	pagesize = 65536;
	cache = 1000 * pagesize;

	while ((ch = getopt(argc, argv, "c:d:EIi:l:n:p:RT")) != EOF)
		switch (ch) {
		case 'c':
			cache = (u_int)atoi(optarg);
			break;
		case 'd':
			dups = atoi(optarg);
			break;
		case 'E':
			env = 0;
			break;
		case 'I':
			init = 1;
			break;
		case 'i':
			iter = atoi(optarg);
			break;
		case 'l':
			datalen = (u_int)atoi(optarg);
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 'p':
			pagesize = (u_int)atoi(optarg);
			break;
		case 'R':
			rflag = 1;
			break;
		case 'T':
			txn = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Remove the previous database. */
	if (!rflag) {
		if (env)
			(void)system("rm -rf BENCH_001; mkdir BENCH_001");
		else
			(void)unlink(DATABASE);
	}

	dbenv = NULL;
	if (env == 1 &&
	    (dbenv = db_init("BENCH_001", "bench_001", cache, txn)) == NULL)
		return (-1);
	if (init)
		exit(0);
	/* Create and initialize database object, open the database. */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_create: %s\n", progname, db_strerror(ret));
		exit(EXIT_FAILURE);
	}
	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, progname);
	if ((ret = dbp->set_bt_compare(dbp, compare_int)) != 0) {
		dbp->err(dbp, ret, "set_bt_compare");
		goto err;
	}
	if ((ret = dbp->set_pagesize(dbp, pagesize)) != 0) {
		dbp->err(dbp, ret, "set_pagesize");
		goto err;
	}
	if (dups && (ret = dbp->set_flags(dbp, DB_DUP)) != 0) {
		dbp->err(dbp, ret, "set_flags");
		goto err;
	}

	if (env == 0 && (ret = dbp->set_cachesize(dbp, 0, cache, 0)) != 0) {
		dbp->err(dbp, ret, "set_cachesize");
		goto err;
	}

	if ((ret = dbp->set_flags(dbp, DB_DUP)) != 0) {
		dbp->err(dbp, ret, "set_flags");
		goto err;
	}

	if (txn != 0)
		if ((ret = dbenv->txn_begin(dbenv, NULL, &txnp, 0)) != 0)
			goto err;

	if ((ret = dbp->open(
	    dbp, txnp, DATABASE, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
		dbp->err(dbp, ret, "%s: open", DATABASE);
		if (txnp != NULL)
			(void)txnp->abort(txnp);
		goto err;
	}

	if (txnp != NULL)
		ret = txnp->commit(txnp, 0);
	txnp = NULL;
	if (ret != 0)
		goto err;

	if (rflag) {
		/* If no environment, fill the cache. */
		if (!env && (ret =
		    get(dbp, txn, datalen, num, dups, iter, &count)) != 0)
			goto err;

		/* Time the get loop. */
		(void)gettimeofday(&start_time, NULL);
		if ((ret =
		    get(dbp, txn, datalen, num, dups, iter, &count)) != 0)
			goto err;
		(void)gettimeofday(&end_time, NULL);
		secs =
		    (((double)end_time.tv_sec * 1000000 + end_time.tv_usec) -
		    ((double)start_time.tv_sec * 1000000 + start_time.tv_usec))
		    / 1000000;
		printf("%d records read using %d batches in %.2f seconds: ",
		    count, iter, secs);
		printf("%.0f records/second\n", (double)count / secs);

	} else if ((ret = fill(dbenv, dbp, txn, datalen, num, dups)) != 0)
		goto err;

	/* Close everything down. */
	if ((ret = dbp->close(dbp, rflag ? DB_NOSYNC : 0)) != 0) {
		fprintf(stderr,
		    "%s: DB->close: %s\n", progname, db_strerror(ret));
		return (1);
	}
	return (ret);

err:	(void)dbp->close(dbp, 0);
	return (1);
}

int
compare_int(dbp, a, b)
	DB *dbp;
	const DBT *a, *b;
{
	int ai, bi;

	dbp = dbp;				/* Lint. */

	/*
	 * Returns:
	 *	< 0 if a < b
	 *	= 0 if a = b
	 *	> 0 if a > b
	 */
	memcpy(&ai, a->data, sizeof(int));
	memcpy(&bi, b->data, sizeof(int));
	return (ai - bi);
}

void
usage()
{
	(void)fprintf(stderr, "usage: %s %s\n\t%s\n",
	    progname, "[-EIRT] [-c cachesize] [-d dups]",
	    "[-i iterations] [-l datalen] [-n keys] [-p pagesize]");
	exit(EXIT_FAILURE);
}
