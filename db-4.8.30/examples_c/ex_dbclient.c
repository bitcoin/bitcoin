/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <db.h>

#define	DATABASE_HOME	"database"

#define	DATABASE	"access.db"

int	db_clientrun __P((DB_ENV *, const char *));
int	ex_dbclient __P((const char *));
int	ex_dbclient_run __P((const char *, FILE *, const char *, const char *));
int	main __P((int, char *[]));

/*
 * An example of a program creating/configuring a Berkeley DB environment.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	const char *home;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s hostname\n", argv[0]);
		return (EXIT_FAILURE);
	}

	/*
	 * All of the shared database files live in DATABASE_HOME, but
	 * data files will live in CONFIG_DATA_DIR.
	 */
	home = DATABASE_HOME;
	return (ex_dbclient_run(home,
	    stderr, argv[1], argv[0]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
ex_dbclient(host)
	const char *host;
{
	const char *home;
	const char *progname = "ex_dbclient";		/* Program name. */
	int ret;

	/*
	 * All of the shared database files live in DATABASE_HOME, but
	 * data files will live in CONFIG_DATA_DIR.
	 */
	home = DATABASE_HOME;

	if ((ret = ex_dbclient_run(home, stderr, host, progname)) != 0)
		return (ret);

	return (0);
}

int
ex_dbclient_run(home, errfp, host, progname)
	const char *home, *host, *progname;
	FILE *errfp;
{
	DB_ENV *dbenv;
	int ret, retry;

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, DB_RPCCLIENT)) != 0) {
		fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		return (1);
	}
	retry = 0;
loop:
	while (retry < 5) {
		/*
		 * Set the server host we are talking to.
		 */
		if ((ret = dbenv->set_rpc_server(dbenv, NULL, host, 10000,
		    10000, 0)) != 0) {
			fprintf(stderr, "Try %d: DB_ENV->set_rpc_server: %s\n",
			    retry, db_strerror(ret));
			retry++;
			sleep(15);
		} else
			break;
	}

	if (retry >= 5) {
		fprintf(stderr,
		    "DB_ENV->set_rpc_server: %s\n", db_strerror(ret));
		dbenv->close(dbenv, 0);
		return (1);
	}
	/*
	 * We want to specify the shared memory buffer pool cachesize,
	 * but everything else is the default.
	 */
	if ((ret = dbenv->set_cachesize(dbenv, 0, 64 * 1024, 0)) != 0) {
		dbenv->err(dbenv, ret, "set_cachesize");
		dbenv->close(dbenv, 0);
		return (1);
	}
	/*
	 * We have multiple processes reading/writing these files, so
	 * we need concurrency control and a shared buffer pool, but
	 * not logging or transactions.
	 */
	if ((ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL, 0)) != 0) {
		dbenv->err(dbenv, ret, "environment open: %s", home);
		dbenv->close(dbenv, 0);
		if (ret == DB_NOSERVER)
			goto loop;
		return (1);
	}

	ret = db_clientrun(dbenv, progname);
	printf("db_clientrun returned %d\n", ret);
	if (ret == DB_NOSERVER)
		goto loop;

	/* Close the handle. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
		return (1);
	}
	return (0);
}

int
db_clientrun(dbenv, progname)
	DB_ENV *dbenv;
	const char *progname;
{
	DB *dbp;
	DBT key, data;
	u_int32_t len;
	int ret;
	char *p, *t, buf[1024], rbuf[1024];

	/* Remove the previous database. */

	/* Create and initialize database object, open the database. */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_create: %s\n", progname, db_strerror(ret));
		return (ret);
	}
	if ((ret = dbp->set_pagesize(dbp, 1024)) != 0) {
		dbp->err(dbp, ret, "set_pagesize");
		goto err1;
	}
	if ((ret = dbp->open(dbp,
	    NULL, DATABASE, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
		dbp->err(dbp, ret, "%s: open", DATABASE);
		goto err1;
	}

	/*
	 * Insert records into the database, where the key is the user
	 * input and the data is the user input in reverse order.
	 */
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	for (;;) {
		printf("input> ");
		fflush(stdout);
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if ((len = strlen(buf)) <= 1)
			continue;
		for (t = rbuf, p = buf + (len - 2); p >= buf;)
			*t++ = *p--;
		*t++ = '\0';

		key.data = buf;
		data.data = rbuf;
		data.size = key.size = len - 1;

		switch (ret =
		    dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE)) {
		case 0:
			break;
		default:
			dbp->err(dbp, ret, "DB->put");
			if (ret != DB_KEYEXIST)
				goto err1;
			break;
		}
		memset(&data, 0, sizeof(DBT));
		switch (ret = dbp->get(dbp, NULL, &key, &data, 0)) {
		case 0:
			printf("%.*s : %.*s\n",
			    (int)key.size, (char *)key.data,
			    (int)data.size, (char *)data.data);
			break;
		default:
			dbp->err(dbp, ret, "DB->get");
			break;
		}
	}
	if ((ret = dbp->close(dbp, 0)) != 0) {
		fprintf(stderr,
		    "%s: DB->close: %s\n", progname, db_strerror(ret));
		return (1);
	}
	return (0);

err1:	(void)dbp->close(dbp, 0);
	return (ret);
}
