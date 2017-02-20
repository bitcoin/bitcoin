/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/fop.h"
#include "dbinc/hash.h"
#include "dbinc/log.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996-2009 Oracle.  All rights reserved.\n";
#endif

int db_printlog_print_app_record __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
int env_init_print __P((ENV *, u_int32_t, DB_DISTAB *));
int env_init_print_42 __P((ENV *, DB_DISTAB *));
int env_init_print_43 __P((ENV *, DB_DISTAB *));
int env_init_print_47 __P((ENV *, DB_DISTAB *));
int lsn_arg __P((char *, DB_LSN *));
int main __P((int, char *[]));
int open_rep_db __P((DB_ENV *, DB **, DBC **));
int usage __P((void));
int version_check __P((void));

const char *progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DBC *dbc;
	DBT data, keydbt;
	DB_DISTAB dtab;
	DB_ENV	*dbenv;
	DB_LOGC *logc;
	DB_LSN key, start, stop, verslsn;
	ENV *env;
	u_int32_t logcflag, newversion, version;
	int ch, cmp, exitval, nflag, rflag, ret, repflag;
	char *home, *passwd;

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = version_check()) != 0)
		return (ret);

	dbp = NULL;
	dbc = NULL;
	dbenv = NULL;
	logc = NULL;
	ZERO_LSN(start);
	ZERO_LSN(stop);
	exitval = nflag = rflag = repflag = 0;
	home = passwd = NULL;

	memset(&dtab, 0, sizeof(dtab));

	while ((ch = getopt(argc, argv, "b:e:h:NP:rRV")) != EOF)
		switch (ch) {
		case 'b':
			/* Don't use getsubopt(3), not all systems have it. */
			if (lsn_arg(optarg, &start))
				return (usage());
			break;
		case 'e':
			/* Don't use getsubopt(3), not all systems have it. */
			if (lsn_arg(optarg, &stop))
				return (usage());
			break;
		case 'h':
			home = optarg;
			break;
		case 'N':
			nflag = 1;
			break;
		case 'P':
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, "%s: strdup: %s\n",
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 'r':
			rflag = 1;
			break;
		case 'R':		/* Undocumented */
			repflag = 1;
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		return (usage());

	/* Handle possible interruptions. */
	__db_util_siginit();

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		goto shutdown;
	}

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	if (nflag) {
		if ((ret = dbenv->set_flags(dbenv, DB_NOLOCKING, 1)) != 0) {
			dbenv->err(dbenv, ret, "set_flags: DB_NOLOCKING");
			goto shutdown;
		}
		if ((ret = dbenv->set_flags(dbenv, DB_NOPANIC, 1)) != 0) {
			dbenv->err(dbenv, ret, "set_flags: DB_NOPANIC");
			goto shutdown;
		}
	}

	if (passwd != NULL && (ret = dbenv->set_encrypt(dbenv,
	    passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		goto shutdown;
	}

	/*
	 * Set up an app-specific dispatch function so that we can gracefully
	 * handle app-specific log records.
	 */
	if ((ret = dbenv->set_app_dispatch(
	    dbenv, db_printlog_print_app_record)) != 0) {
		dbenv->err(dbenv, ret, "app_dispatch");
		goto shutdown;
	}

	/*
	 * An environment is required, but as all we're doing is reading log
	 * files, we create one if it doesn't already exist.  If we create
	 * it, create it private so it automatically goes away when we're done.
	 * If we are reading the replication database, do not open the env
	 * with logging, because we don't want to log the opens.
	 */
	if (repflag) {
		if ((ret = dbenv->open(dbenv, home,
		    DB_INIT_MPOOL | DB_USE_ENVIRON, 0)) != 0 &&
		    (ret == DB_VERSION_MISMATCH ||
		    (ret = dbenv->open(dbenv, home,
		    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE | DB_USE_ENVIRON, 0))
		    != 0)) {
			dbenv->err(dbenv, ret, "DB_ENV->open");
			goto shutdown;
		}
	} else if ((ret = dbenv->open(dbenv, home, DB_USE_ENVIRON, 0)) != 0 &&
	    (ret == DB_VERSION_MISMATCH ||
	    (ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_LOG | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0)) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto shutdown;
	}
	env = dbenv->env;

	/* Allocate a log cursor. */
	if (repflag) {
		if ((ret = open_rep_db(dbenv, &dbp, &dbc)) != 0)
			goto shutdown;
	} else if ((ret = dbenv->log_cursor(dbenv, &logc, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->log_cursor");
		goto shutdown;
	}

	if (IS_ZERO_LSN(start)) {
		memset(&keydbt, 0, sizeof(keydbt));
		logcflag = rflag ? DB_PREV : DB_NEXT;
	} else {
		key = start;
		logcflag = DB_SET;
	}
	memset(&data, 0, sizeof(data));

	/*
	 * If we're using the repflag, we're immediately initializing
	 * the print table.  Use the current version.  If we're printing
	 * the log then initialize version to 0 so that we get the
	 * correct version right away.
	 */
	if (repflag)
		version = DB_LOGVERSION;
	else
		version = 0;
	ZERO_LSN(verslsn);

	/* Initialize print callbacks if repflag. */
	if (repflag &&
	    (ret = env_init_print(env, version, &dtab)) != 0) {
		dbenv->err(dbenv, ret, "callback: initialization");
		goto shutdown;
	}
	for (; !__db_util_interrupted(); logcflag = rflag ? DB_PREV : DB_NEXT) {
		if (repflag) {
			ret = dbc->get(dbc, &keydbt, &data, logcflag);
			if (ret == 0)
				key = ((__rep_control_args *)keydbt.data)->lsn;
		} else
			ret = logc->get(logc, &key, &data, logcflag);
		if (ret != 0) {
			if (ret == DB_NOTFOUND)
				break;
			dbenv->err(dbenv,
			    ret, repflag ? "DBC->get" : "DB_LOGC->get");
			goto shutdown;
		}

		/*
		 * We may have reached the end of the range we're displaying.
		 */
		if (!IS_ZERO_LSN(stop)) {
			cmp = LOG_COMPARE(&key, &stop);
			if ((rflag && cmp < 0) || (!rflag && cmp > 0))
				break;
		}
		if (!repflag && key.file != verslsn.file) {
			/*
			 * If our log file changed, we need to see if the
			 * version of the log file changed as well.
			 * If it changed, reset the print table.
			 */
			if ((ret = logc->version(logc, &newversion, 0)) != 0) {
				dbenv->err(dbenv, ret, "DB_LOGC->version");
				goto shutdown;
			}
			if (version != newversion) {
				version = newversion;
				if ((ret = env_init_print(env, version,
				    &dtab)) != 0) {
					dbenv->err(dbenv, ret,
					    "callback: initialization");
					goto shutdown;
				}
			}
		}

		ret = __db_dispatch(dbenv->env,
		    &dtab, &data, &key, DB_TXN_PRINT, NULL);

		/*
		 * XXX
		 * Just in case the underlying routines don't flush.
		 */
		(void)fflush(stdout);

		if (ret != 0) {
			dbenv->err(dbenv, ret, "tx: dispatch");
			goto shutdown;
		}
	}

	if (0) {
shutdown:	exitval = 1;
	}
	if (logc != NULL && (ret = logc->close(logc, 0)) != 0)
		exitval = 1;

	if (dbc != NULL && (ret = dbc->close(dbc)) != 0)
		exitval = 1;

	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0)
		exitval = 1;

	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}

	if (passwd != NULL)
		free(passwd);

	/* Resend any caught signal. */
	__db_util_sigresend();

	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * env_init_print --
 */
int
env_init_print(env, version, dtabp)
	ENV *env;
	u_int32_t version;
	DB_DISTAB *dtabp;
{
	int ret;

	/*
	 * We need to prime the print table with the current print
	 * functions.  Then we overwrite only specific entries based on
	 * each previous version we support.
	 */
	if ((ret = __bam_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __crdel_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __db_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __dbreg_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __fop_init_print(env, dtabp)) != 0)
		goto err;
#ifdef HAVE_HASH
	if ((ret = __ham_init_print(env, dtabp)) != 0)
		goto err;
#endif
#ifdef HAVE_QUEUE
	if ((ret = __qam_init_print(env, dtabp)) != 0)
		goto err;
#endif
	if ((ret = __txn_init_print(env, dtabp)) != 0)
		goto err;

	switch (version) {
	case DB_LOGVERSION:
		ret = 0;
		break;

	/*
	 * There are no log record/recovery differences between
	 * 4.4 and 4.5.  The log version changed due to checksum.
	 * There are no log recovery differences between
	 * 4.5 and 4.6.  The name of the rep_gen in txn_checkpoint
	 * changed (to spare, since we don't use it anymore).
	 */
	case DB_LOGVERSION_48:
		if ((ret = __db_add_recovery_int(env, dtabp,
		    __db_pg_sort_44_print, DB___db_pg_sort_44)) != 0)
			goto err;
		break;
	case DB_LOGVERSION_47:
	case DB_LOGVERSION_46:
	case DB_LOGVERSION_45:
	case DB_LOGVERSION_44:
		ret = env_init_print_47(env, dtabp);
		break;
	case DB_LOGVERSION_43:
		ret = env_init_print_43(env, dtabp);
		break;
	case DB_LOGVERSION_42:
		ret = env_init_print_42(env, dtabp);
		break;
	default:
		env->dbenv->errx(env->dbenv,
		    "Unknown version %lu", (u_long)version);
		ret = EINVAL;
		break;
	}
err:	return (ret);
}

int
env_init_print_42(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	   __bam_split_42_print, DB___bam_split_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_relink_42_print, DB___db_relink_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_alloc_42_print, DB___db_pg_alloc_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_free_42_print, DB___db_pg_free_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_freedata_42_print, DB___db_pg_freedata_42)) != 0)
		goto err;
#if HAVE_HASH
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_metagroup_42_print, DB___ham_metagroup_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_groupalloc_42_print, DB___ham_groupalloc_42)) != 0)
		goto err;
#endif
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_ckp_42_print, DB___txn_ckp_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_regop_42_print, DB___txn_regop_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_create_42_print, DB___fop_create_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_write_42_print, DB___fop_write_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_noundo_46)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_xa_regop_42_print, DB___txn_xa_regop_42)) != 0)
		goto err;
err:
	return (ret);
}

int
env_init_print_43(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_relink_43_print, DB___bam_relink_43)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	   __bam_split_42_print, DB___bam_split_42)) != 0)
		goto err;
	/*
	 * We want to use the 4.2-based txn_regop record.
	 */
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_regop_42_print, DB___txn_regop_42)) != 0)
		goto err;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_create_42_print, DB___fop_create_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_write_42_print, DB___fop_write_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_noundo_46)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_xa_regop_42_print, DB___txn_xa_regop_42)) != 0)
		goto err;
err:
	return (ret);
}

/*
 * env_init_print_47 --
 *
 */
int
env_init_print_47(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	   __bam_split_42_print, DB___bam_split_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_sort_44_print, DB___db_pg_sort_44)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_sort_44_print, DB___db_pg_sort_44)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_create_42_print, DB___fop_create_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_write_42_print, DB___fop_write_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_noundo_46)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_xa_regop_42_print, DB___txn_xa_regop_42)) != 0)
		goto err;

err:
	return (ret);
}

int
usage()
{
	fprintf(stderr, "usage: %s %s\n", progname,
	    "[-NrV] [-b file/offset] [-e file/offset] [-h home] [-P password]");
	return (EXIT_FAILURE);
}

int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr,
	"%s: version %d.%d doesn't match library version %d.%d\n",
		    progname, DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}

/* Print an unknown, application-specific log record as best we can. */
int
db_printlog_print_app_record(dbenv, dbt, lsnp, op)
	DB_ENV *dbenv;
	DBT *dbt;
	DB_LSN *lsnp;
	db_recops op;
{
	u_int32_t i, rectype;
	int ch;

	DB_ASSERT(dbenv->env, op == DB_TXN_PRINT);

	COMPQUIET(dbenv, NULL);
	COMPQUIET(op, DB_TXN_PRINT);

	/*
	 * Fetch the rectype, which always must be at the beginning of the
	 * record (if dispatching is to work at all).
	 */
	memcpy(&rectype, dbt->data, sizeof(rectype));

	/*
	 * Applications may wish to customize the output here based on the
	 * rectype.  We just print the entire log record in the generic
	 * mixed-hex-and-printable format we use for binary data.
	 */
	printf("[%lu][%lu]application specific record: rec: %lu\n",
	    (u_long)lsnp->file, (u_long)lsnp->offset, (u_long)rectype);
	printf("\tdata: ");
	for (i = 0; i < dbt->size; i++) {
		ch = ((u_int8_t *)dbt->data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	printf("\n\n");

	return (0);
}

int
open_rep_db(dbenv, dbpp, dbcp)
	DB_ENV *dbenv;
	DB **dbpp;
	DBC **dbcp;
{
	int ret;

	DB *dbp;
	*dbpp = NULL;
	*dbcp = NULL;

	if ((ret = db_create(dbpp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		return (ret);
	}

	dbp = *dbpp;
	if ((ret =
	    dbp->open(dbp, NULL, REPDBNAME, NULL, DB_BTREE, 0, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open");
		goto err;
	}

	if ((ret = dbp->cursor(dbp, NULL, dbcp, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->cursor");
		goto err;
	}

	return (0);

err:	if (*dbpp != NULL)
		(void)(*dbpp)->close(*dbpp, 0);
	return (ret);
}

/*
 * lsn_arg --
 *	Parse a LSN argument.
 */
int
lsn_arg(arg, lsnp)
	char *arg;
	DB_LSN *lsnp;
{
	u_long uval;
	char *p;

	/*
	 * Expected format is: lsn.file/lsn.offset.
	 */
	if ((p = strchr(arg, '/')) == NULL)
		return (1);
	*p = '\0';

	if (__db_getulong(NULL, progname, arg, 0, UINT32_MAX, &uval))
		return (1);
	lsnp->file = uval;
	if (__db_getulong(NULL, progname, p + 1, 0, UINT32_MAX, &uval))
		return (1);
	lsnp->offset = uval;
	return (0);
}
