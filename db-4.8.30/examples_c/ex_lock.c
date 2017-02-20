/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

int db_init __P((const char *, u_int32_t, int));
int main __P((int, char *[]));
int usage __P((void));

DB_ENV	 *dbenv;
const char
	*progname = "ex_lock";				/* Program name. */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DBT lock_dbt;
	DB_LOCK lock;
	DB_LOCK *locks;
	db_lockmode_t lock_type;
	long held;
	size_t len;
	u_int32_t locker, maxlocks;
	int ch, do_unlink, did_get, i, lockid, lockcount, ret;
	const char *home;
	char opbuf[16], objbuf[1024], lockbuf[16];

	home = "TESTDIR";
	maxlocks = 0;
	do_unlink = 0;
	while ((ch = getopt(argc, argv, "h:m:u")) != EOF)
		switch (ch) {
		case 'h':
			home = optarg;
			break;
		case 'm':
			if ((i = atoi(optarg)) <= 0)
				return (usage());
			maxlocks = (u_int32_t)i;  /* XXX: possible overflow. */
			break;
		case 'u':
			do_unlink = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	/* Initialize the database environment. */
	if ((ret = db_init(home, maxlocks, do_unlink)) != 0)
		return (ret);

	locks = 0;
	lockcount = 0;

	/*
	 * Accept lock requests.
	 */
	if ((ret = dbenv->lock_id(dbenv, &locker)) != 0) {
		dbenv->err(dbenv, ret, "unable to get locker id");
		(void)dbenv->close(dbenv, 0);
		return (EXIT_FAILURE);
	}
	lockid = -1;

	memset(&lock_dbt, 0, sizeof(lock_dbt));
	for (held = 0, did_get = 0;;) {
		printf("Operation get/release [get]> ");
		fflush(stdout);
		if (fgets(opbuf, sizeof(opbuf), stdin) == NULL)
			break;
		if ((len = strlen(opbuf)) <= 1 || strcmp(opbuf, "get\n") == 0) {
			/* Acquire a lock. */
			printf("input object (text string) to lock> ");
			fflush(stdout);
			if (fgets(objbuf, sizeof(objbuf), stdin) == NULL)
				break;
			if ((len = strlen(objbuf)) <= 1)
				continue;

			do {
				printf("lock type read/write [read]> ");
				fflush(stdout);
				if (fgets(lockbuf,
				    sizeof(lockbuf), stdin) == NULL)
					break;
				len = strlen(lockbuf);
			} while (len > 1 &&
			    strcmp(lockbuf, "read\n") != 0 &&
			    strcmp(lockbuf, "write\n") != 0);
			if (len == 1 || strcmp(lockbuf, "read\n") == 0)
				lock_type = DB_LOCK_READ;
			else
				lock_type = DB_LOCK_WRITE;

			lock_dbt.data = objbuf;
			lock_dbt.size = (u_int32_t)strlen(objbuf);
			ret = dbenv->lock_get(dbenv, locker,
			    DB_LOCK_NOWAIT, &lock_dbt, lock_type, &lock);
			if (ret == 0) {
				did_get = 1;
				lockid = lockcount++;
				if (locks == NULL)
					locks =
					    (DB_LOCK *)malloc(sizeof(DB_LOCK));
				else
					locks = (DB_LOCK *)realloc(locks,
					    lockcount * sizeof(DB_LOCK));
				locks[lockid] = lock;
			}
		} else {
			/* Release a lock. */
			do {
				printf("input lock to release> ");
				fflush(stdout);
				if (fgets(objbuf,
				    sizeof(objbuf), stdin) == NULL)
					break;
			} while ((len = strlen(objbuf)) <= 1);
			lockid = strtol(objbuf, NULL, 16);
			if (lockid < 0 || lockid >= lockcount) {
				printf("Lock #%d out of range\n", lockid);
				continue;
			}
			lock = locks[lockid];
			ret = dbenv->lock_put(dbenv, &lock);
			did_get = 0;
		}
		switch (ret) {
		case 0:
			printf("Lock #%d %s\n", lockid,
			    did_get ? "granted" : "released");
			held += did_get ? 1 : -1;
			break;
		case DB_LOCK_NOTGRANTED:
			dbenv->err(dbenv, ret, NULL);
			break;
		case DB_LOCK_DEADLOCK:
			dbenv->err(dbenv, ret,
			    "lock_%s", did_get ? "get" : "put");
			break;
		default:
			dbenv->err(dbenv, ret,
			    "lock_%s", did_get ? "get" : "put");
			(void)dbenv->close(dbenv, 0);
			return (EXIT_FAILURE);
		}
	}

	printf("\nClosing lock region %ld locks held\n", held);

	if (locks != NULL)
		free(locks);

	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

/*
 * db_init --
 *	Initialize the environment.
 */
int
db_init(home, maxlocks, do_unlink)
	const char *home;
	u_int32_t maxlocks;
	int do_unlink;
{
	int ret;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "%s: db_env_create: %s\n",
		    progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}

	if (do_unlink) {
		if ((ret = dbenv->remove(dbenv, home, DB_FORCE)) != 0) {
			fprintf(stderr, "%s: dbenv->remove: %s\n",
			    progname, db_strerror(ret));
			return (EXIT_FAILURE);
		}
		if ((ret = db_env_create(&dbenv, 0)) != 0) {
			fprintf(stderr, "%s: db_env_create: %s\n",
			    progname, db_strerror(ret));
			return (EXIT_FAILURE);
		}
	}

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);
	if (maxlocks != 0)
		dbenv->set_lk_max_locks(dbenv, maxlocks);

	if ((ret =
	    dbenv->open(dbenv, home, DB_CREATE | DB_INIT_LOCK, 0)) != 0) {
		dbenv->err(dbenv, ret, NULL);
		(void)dbenv->close(dbenv, 0);
		return (EXIT_FAILURE);
	}
	return (0);
}

int
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-u] [-h home] [-m maxlocks]\n", progname);
	return (EXIT_FAILURE);
}
