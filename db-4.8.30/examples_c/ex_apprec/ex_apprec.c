/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "db_int.h"
#include "dbinc/db_swap.h"

#include "ex_apprec.h"

int	apprec_dispatch __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
int	open_env __P((const char *, FILE *, const char *, DB_ENV **));
int	verify_absence __P((DB_ENV *, const char *));
int	verify_presence __P((DB_ENV *, const char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	DB_ENV *dbenv;
	DB_LSN lsn;
	DB_TXN *txn;
	DBT dirnamedbt;
	int ret;
	const char *home;
	char ch, dirname[256];
	const char *progname = "ex_apprec";		/* Program name. */

	/* Default home. */
	home = "TESTDIR";

	while ((ch = getopt(argc, argv, "h:")) != EOF)
		switch (ch) {
		case 'h':
			home = optarg;
			break;
		default:
			fprintf(stderr, "usage: %s [-h home]", progname);
			exit(EXIT_FAILURE);
		}

	printf("Set up environment.\n");
	if ((ret = open_env(home, stderr, progname, &dbenv)) != 0)
		return (EXIT_FAILURE);

	printf("Create a directory in a transaction.\n");
	/*
	 * This application's convention is to log the full directory name,
	 * including trailing nul.
	 */
	memset(&dirnamedbt, 0, sizeof(dirnamedbt));
	sprintf(dirname, "%s/MYDIRECTORY", home);
	dirnamedbt.data = dirname;
	dirnamedbt.size = strlen(dirname) + 1;

	if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
		dbenv->err(dbenv, ret, "txn_begin");
		return (EXIT_FAILURE);
	}

	/*
	 * Remember, always log actions before you execute them!
	 * Since this log record is describing a file system operation and
	 * we have no control over when file system operations go to disk,
	 * we need to flush the log record immediately to ensure that the
	 * log record is on disk before the operation it describes.  The
	 * flush would not be necessary were we doing an operation into the
	 * BDB mpool and using LSNs that mpool knew about.
	 */
	memset(&lsn, 0, sizeof(lsn));
	if ((ret =
	    ex_apprec_mkdir_log(dbenv,
		txn, &lsn, DB_FLUSH, &dirnamedbt)) != 0) {
		dbenv->err(dbenv, ret, "mkdir_log");
		return (EXIT_FAILURE);
	}
	if (mkdir(dirname, 0755) != 0) {
		dbenv->err(dbenv, errno, "mkdir");
		return (EXIT_FAILURE);
	}

	printf("Verify the directory's presence: ");
	verify_presence(dbenv, dirname);
	printf("check.\n");

	/* Now abort the transaction and verify that the directory goes away. */
	printf("Abort the transaction.\n");
	if ((ret = txn->abort(txn)) != 0) {
		dbenv->err(dbenv, ret, "txn_abort");
		return (EXIT_FAILURE);
	}

	printf("Verify the directory's absence: ");
	verify_absence(dbenv, dirname);
	printf("check.\n");

	/* Now do the same thing over again, only with a commit this time. */
	printf("Create a directory in a transaction.\n");
	memset(&dirnamedbt, 0, sizeof(dirnamedbt));
	sprintf(dirname, "%s/MYDIRECTORY", home);
	dirnamedbt.data = dirname;
	dirnamedbt.size = strlen(dirname) + 1;
	if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
		dbenv->err(dbenv, ret, "txn_begin");
		return (EXIT_FAILURE);
	}

	memset(&lsn, 0, sizeof(lsn));
	if ((ret =
	    ex_apprec_mkdir_log(dbenv, txn, &lsn, 0, &dirnamedbt)) != 0) {
		dbenv->err(dbenv, ret, "mkdir_log");
		return (EXIT_FAILURE);
	}
	if (mkdir(dirname, 0755) != 0) {
		dbenv->err(dbenv, errno, "mkdir");
		return (EXIT_FAILURE);
	}

	printf("Verify the directory's presence: ");
	verify_presence(dbenv, dirname);
	printf("check.\n");

	/* Now abort the transaction and verify that the directory goes away. */
	printf("Commit the transaction.\n");
	if ((ret = txn->commit(txn, 0)) != 0) {
		dbenv->err(dbenv, ret, "txn_commit");
		return (EXIT_FAILURE);
	}

	printf("Verify the directory's presence: ");
	verify_presence(dbenv, dirname);
	printf("check.\n");

	printf("Now remove the directory, then run recovery.\n");
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
		return (EXIT_FAILURE);
	}
	if (rmdir(dirname) != 0) {
		fprintf(stderr,
		    "%s: rmdir failed with error %s", progname,
		    strerror(errno));
	}
	verify_absence(dbenv, dirname);

	/* Opening with DB_RECOVER runs recovery. */
	if ((ret = open_env(home, stderr, progname, &dbenv)) != 0)
		return (EXIT_FAILURE);

	printf("Verify the directory's presence: ");
	verify_presence(dbenv, dirname);
	printf("check.\n");

	/* Close the handle. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "DB_ENV->close: %s\n", db_strerror(ret));
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

int
open_env(home, errfp, progname, dbenvp)
	const char *home, *progname;
	FILE *errfp;
	DB_ENV **dbenvp;
{
	DB_ENV *dbenv;
	int ret;

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(errfp, "%s: %s\n", progname, db_strerror(ret));
		return (ret);
	}
	dbenv->set_errfile(dbenv, errfp);
	dbenv->set_errpfx(dbenv, progname);

	/* Set up our custom recovery dispatch function. */
	if ((ret = dbenv->set_app_dispatch(dbenv, apprec_dispatch)) != 0) {
		dbenv->err(dbenv, ret, "set_app_dispatch");
		return (ret);
	}

	/*
	 * Open the environment with full transactional support, running
	 * recovery.
	 */
	if ((ret =
	    dbenv->open(dbenv, home, DB_CREATE | DB_RECOVER | DB_INIT_LOCK |
	    DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN, 0)) != 0) {
		dbenv->err(dbenv, ret, "environment open: %s", home);
		dbenv->close(dbenv, 0);
		return (ret);
	}

	*dbenvp = dbenv;
	return (0);
}

/*
 * Sample application dispatch function to handle user-specified log record
 * types.
 */
int
apprec_dispatch(dbenv, dbt, lsn, op)
	DB_ENV *dbenv;
	DBT *dbt;
	DB_LSN *lsn;
	db_recops op;
{
	u_int32_t rectype;

	/* Pull the record type out of the log record. */
	LOGCOPY_32(dbenv->env, &rectype, dbt->data);

	switch (rectype) {
	case DB_ex_apprec_mkdir:
		return (ex_apprec_mkdir_recover(dbenv, dbt, lsn, op));
	default:
		/*
		 * We've hit an unexpected, allegedly user-defined record
		 * type.
		 */
		dbenv->errx(dbenv, "Unexpected log record type encountered");
		return (EINVAL);
	}
}

int
verify_absence(dbenv, dirname)
	DB_ENV *dbenv;
	const char *dirname;
{

	if (access(dirname, F_OK) == 0) {
		dbenv->errx(dbenv, "Error--directory present!");
		exit(EXIT_FAILURE);
	}

	return (0);
}

int
verify_presence(dbenv, dirname)
	DB_ENV *dbenv;
	const char *dirname;
{

	if (access(dirname, F_OK) != 0) {
		dbenv->errx(dbenv, "Error--directory not present!");
		exit(EXIT_FAILURE);
	}

	return (0);
}
