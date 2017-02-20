/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

int db_setup __P((const char *, const char *, FILE *, const char *));
int db_teardown __P((const char *, const char *, FILE *, const char *));
static int usage __P((void));

const char *progname = "ex_env";		/* Program name. */

/*
 * An example of a program creating/configuring a Berkeley DB environment.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	const char *data_dir, *home;

	int ch;
	/*
	 * All of the shared database files live in home, but
	 * data files will live in data_dir.
	 */
	home = "TESTDIR";
	data_dir = "data";
	while ((ch = getopt(argc, argv, "h:d:")) != EOF)
		switch (ch) {
		case 'h':
			home = optarg;
			break;
		case 'd':
			data_dir = optarg;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	printf("Setup env\n");
	if (db_setup(home, data_dir, stderr, progname) != 0)
		return (EXIT_FAILURE);

	printf("Teardown env\n");
	if (db_teardown(home, data_dir, stderr, progname) != 0)
		return (EXIT_FAILURE);

	return (EXIT_SUCCESS);
}

int
db_setup(home, data_dir, errfp, progname)
	const char *home, *data_dir, *progname;
	FILE *errfp;
{
	DB_ENV *dbenv;
	DB *dbp;
	int ret;

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		return (1);
	}
	dbenv->set_errfile(dbenv, errfp);
	dbenv->set_errpfx(dbenv, progname);

	/*
	 * We want to specify the shared memory buffer pool cachesize,
	 * but everything else is the default.
	 */
	if ((ret = dbenv->set_cachesize(dbenv, 0, 64 * 1024, 0)) != 0) {
		dbenv->err(dbenv, ret, "set_cachesize");
		dbenv->close(dbenv, 0);
		return (1);
	}

	/* Databases are in a subdirectory. */
	(void)dbenv->set_data_dir(dbenv, data_dir);

	/* Open the environment with full transactional support. */
	if ((ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | 
	    DB_INIT_TXN, 0644)) != 0) {
		dbenv->err(dbenv, ret, "environment open: %s", home);
		dbenv->close(dbenv, 0);
		return (1);
	}

	/* 
	 * Open a database in the environment to verify the data_dir
	 * has been set correctly.
	 * Create a database object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0){
		fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		return (1);
	}

	/* Open a database with DB_BTREE access method. */
	if ((ret = dbp->open(dbp, NULL, "exenv_db1.db", NULL, 
	    DB_BTREE, DB_CREATE,0644)) != 0){
		fprintf(stderr, "database open: %s\n", db_strerror(ret));
		return (1);
	}

	/* Close the database handle. */
	if ((ret = dbp->close(dbp, 0)) != 0) {
		fprintf(stderr, "database close: %s\n", db_strerror(ret));
		return (1);
	}

	/* Close the environment handle. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
		return (1);
	}
	return (0);
}

int
db_teardown(home, data_dir, errfp, progname)
	const char *home, *data_dir, *progname;
	FILE *errfp;
{
	DB_ENV *dbenv;
	int ret;

	/* Remove the shared database regions. */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		return (1);
	}
	dbenv->set_errfile(dbenv, errfp);
	dbenv->set_errpfx(dbenv, progname);

	(void)dbenv->set_data_dir(dbenv, data_dir);

	/* Remove the environment. */
	if ((ret = dbenv->remove(dbenv, home, 0)) != 0) {
		fprintf(stderr, "DB_ENV->remove: %s\n", db_strerror(ret));
		return (1);
	}
	return (0);
}

static int
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-h home] [-d data_dir]\n", progname);
	return (EXIT_FAILURE);
}
