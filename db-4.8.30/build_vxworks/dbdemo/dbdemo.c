/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db_config.h>
#include <db_int.h>

#define	DATABASE	"access.db"
int dbdemo_main __P((int, char *[]));
int dbdemo_usage __P((void));

int
dbdemo(args)
	char *args;
{
	int argc;
	char **argv;

	__db_util_arg("dbdemo", args, &argc, &argv);
	return (dbdemo_main(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS);
}

#include <stdio.h>
#define	ERROR_RETURN	ERROR

int
dbdemo_main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind, __db_getopt_reset;
	DB *dbp;
	DBC *dbcp;
	DBT key, data;
	size_t len;
	int ch, ret, rflag;
	char *database, *p, *t, buf[1024], rbuf[1024];
	const char *progname = "dbdemo";		/* Program name. */

	rflag = 0;
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "r")) != EOF)
		switch (ch) {
		case 'r':
			rflag = 1;
			break;
		case '?':
		default:
			return (dbdemo_usage());
		}
	argc -= optind;
	argv += optind;

	/* Accept optional database name. */
	database = *argv == NULL ? DATABASE : argv[0];

	/* Optionally discard the database. */
	if (rflag)
		(void)remove(database);

	/* Create and initialize database object, open the database. */
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_create: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, progname);
	if ((ret = dbp->set_pagesize(dbp, 1024)) != 0) {
		dbp->err(dbp, ret, "set_pagesize");
		goto err1;
	}
	if ((ret = dbp->set_cachesize(dbp, 0, 32 * 1024, 0)) != 0) {
		dbp->err(dbp, ret, "set_cachesize");
		goto err1;
	}
	if ((ret = dbp->open(dbp,
	    NULL, database, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
		dbp->err(dbp, ret, "%s: open", database);
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
		if (strcmp(buf, "exit\n") == 0 || strcmp(buf, "quit\n") == 0)
			break;
		if ((len = strlen(buf)) <= 1)
			continue;
		for (t = rbuf, p = buf + (len - 2); p >= buf;)
			*t++ = *p--;
		*t++ = '\0';

		key.data = buf;
		data.data = rbuf;
		data.size = key.size = (u_int32_t)len - 1;

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
	}
	printf("\n");

	/* Acquire a cursor for the database. */
	if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
		dbp->err(dbp, ret, "DB->cursor");
		goto err1;
	}

	/* Initialize the key/data pair so the flags aren't set. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	/* Walk through the database and print out the key/data pairs. */
	while ((ret = dbcp->get(dbcp, &key, &data, DB_NEXT)) == 0)
		printf("%.*s : %.*s\n",
		    (int)key.size, (char *)key.data,
		    (int)data.size, (char *)data.data);
	if (ret != DB_NOTFOUND) {
		dbp->err(dbp, ret, "DBcursor->get");
		goto err2;
	}

	/* Close everything down. */
	if ((ret = dbcp->close(dbcp)) != 0) {
		dbp->err(dbp, ret, "DBcursor->close");
		goto err1;
	}
	if ((ret = dbp->close(dbp, 0)) != 0) {
		fprintf(stderr,
		    "%s: DB->close: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);

err2:	(void)dbcp->close(dbcp);
err1:	(void)dbp->close(dbp, 0);
	return (EXIT_FAILURE);
}

int
dbdemo_usage()
{
	(void)fprintf(stderr, "usage: ex_access [-r] [database]\n");
	return (EXIT_FAILURE);
}
