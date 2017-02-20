/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * This file is based on the template file ex_apprec_template.  Note that
 * because ex_apprec_mkdir, like most application-specific recovery functions,
 * does not make use of DB-private structures, it has actually been simplified
 * significantly.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include <db.h>

#include "ex_apprec.h"

/*
 * ex_apprec_mkdir_recover --
 *	Recovery function for mkdir.
 *
 * PUBLIC: int ex_apprec_mkdir_recover
 * PUBLIC:   __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
 */
int
ex_apprec_mkdir_recover(dbenv, dbtp, lsnp, op)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
{
	ex_apprec_mkdir_args *argp;
	int ret;

	argp = NULL;

#ifdef DEBUG_RECOVER
	ex_apprec_mkdir_print(dbenv, dbtp, lsnp, op);
#endif
	if ((ret = ex_apprec_mkdir_read(dbenv, dbtp->data, &argp)) != 0)
		goto out;

	switch (op) {
	case DB_TXN_ABORT:
	case DB_TXN_BACKWARD_ROLL:
		/*
		 * If we're aborting, we need to remove the directory if it
		 * exists.  We log the trailing zero in pathnames, so we can
		 * simply pass the data part of the DBT into rmdir as a string.
		 * (Note that we don't have any alignment guarantees, but for
		 * a char * this doesn't matter.)
		 *
		 * Ignore all errors other than ENOENT;  DB may attempt to undo
		 * or redo operations without knowing whether they have already
		 * been done or undone, so we should never assume in a recovery
		 * function that the task definitely needs doing or undoing.
		 */
		ret = rmdir(argp->dirname.data);
		if (ret != 0 && errno != ENOENT)
			dbenv->err(dbenv, ret, "Error in abort of mkdir");
		else
			ret = 0;
		break;
	case DB_TXN_FORWARD_ROLL:
		/*
		 * The forward direction is just the opposite;  here, we ignore
		 * EEXIST, because the directory may already exist.
		 */
		ret = mkdir(argp->dirname.data, 0755);
		if (ret != 0 && errno != EEXIST)
			dbenv->err(dbenv,
			    ret, "Error in roll-forward of mkdir");
		else
			ret = 0;
		break;
	default:
		/*
		 * We might want to handle DB_TXN_PRINT or DB_TXN_APPLY here,
		 * too, but we don't try to print the log records and aren't
		 * using replication, so there's no need to in this example.
		 */
		dbenv->errx(dbenv, "Unexpected operation type\n");
		return (EINVAL);
	}

	/*
	 * The recovery function is responsible for returning the LSN of the
	 * previous log record in this transaction, so that transaction aborts
	 * can follow the chain backwards.
	 *
	 * (If we'd wanted the LSN of this record earlier, we could have
	 * read it from lsnp, as well--but because we weren't working with
	 * pages or other objects that store their LSN and base recovery
	 * decisions on it, we didn't need to.)
	 */
	*lsnp = argp->prev_lsn;

out:	if (argp != NULL)
		free(argp);
	return (ret);
}
