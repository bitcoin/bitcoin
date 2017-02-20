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
#include "dbinc/db_am.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996-2009 Oracle.  All rights reserved.\n";
#endif

typedef struct {			/* XXX: Globals. */
	const char *progname;		/* Program name. */
	char	*hdrbuf;		/* Input file header. */
	u_long	lineno;			/* Input file line number. */
	u_long	origline;		/* Original file line number. */
	int	endodata;		/* Reached the end of a database. */
	int	endofile;		/* Reached the end of the input. */
	int	version;		/* Input version. */
	char	*home;			/* Env home. */
	char	*passwd;		/* Env passwd. */
	int	private;		/* Private env. */
	u_int32_t cache;		/* Env cache size. */
} LDG;

int	db_load_badend __P((DB_ENV *));
void	db_load_badnum __P((DB_ENV *));
int	db_load_configure __P((DB_ENV *, DB *, char **, char **, int *));
int	db_load_convprintable __P((DB_ENV *, char *, char **));
int	db_load_db_init __P((DB_ENV *, char *, u_int32_t, int *));
int	db_load_dbt_rdump __P((DB_ENV *, DBT *));
int	db_load_dbt_rprint __P((DB_ENV *, DBT *));
int	db_load_dbt_rrecno __P((DB_ENV *, DBT *, int));
int	db_load_dbt_to_recno __P((DB_ENV *, DBT *, db_recno_t *));
int	db_load_env_create __P((DB_ENV **, LDG *));
int	db_load_load __P((DB_ENV *, char *, DBTYPE, char **, u_int, LDG *, int *));
int	db_load_main __P((int, char *[]));
int	db_load_rheader __P((DB_ENV *, DB *, DBTYPE *, char **, int *, int *));
int	db_load_usage __P((void));
int	db_load_version_check __P((void));

const char *progname;

#define	G(f)	((LDG *)dbenv->app_private)->f

					/* Flags to the load function. */
#define	LDF_NOHEADER	0x01		/* No dump header. */
#define	LDF_NOOVERWRITE	0x02		/* Don't overwrite existing rows. */
#define	LDF_PASSWORD	0x04		/* Encrypt created databases. */

int
db_load(args)
	char *args;
{
	int argc;
	char **argv;

	__db_util_arg("db_load", args, &argc, &argv);
	return (db_load_main(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS);
}

#include <stdio.h>
#define	ERROR_RETURN	ERROR

int
db_load_main(argc, argv)
	int argc;
	char *argv[];
{
	enum { NOTSET, FILEID_RESET, LSN_RESET, STANDARD_LOAD } mode;
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DBTYPE dbtype;
	DB_ENV	*dbenv;
	LDG ldg;
	u_int ldf;
	int ch, existed, exitval, ret;
	char **clist, **clp;

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((exitval = db_load_version_check()) != 0)
		goto done;

	ldg.progname = progname;
	ldg.lineno = 0;
	ldg.endodata = ldg.endofile = 0;
	ldg.version = 1;
	ldg.cache = MEGABYTE;
	ldg.hdrbuf = NULL;
	ldg.home = NULL;
	ldg.passwd = NULL;

	mode = NOTSET;
	ldf = 0;
	exitval = existed = 0;
	dbtype = DB_UNKNOWN;

	/* Allocate enough room for configuration arguments. */
	if ((clp = clist =
	    (char **)calloc((size_t)argc + 1, sizeof(char *))) == NULL) {
		fprintf(stderr, "%s: %s\n", ldg.progname, strerror(ENOMEM));
		exitval = 1;
		goto done;
	}

	/*
	 * There are two modes for db_load: -r and everything else.  The -r
	 * option zeroes out the database LSN's or resets the file ID, it
	 * doesn't really "load" a new database.  The functionality is in
	 * db_load because we don't have a better place to put it, and we
	 * don't want to create a new utility for just that functionality.
	 */
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "c:f:h:nP:r:Tt:V")) != EOF)
		switch (ch) {
		case 'c':
			if (mode != NOTSET && mode != STANDARD_LOAD) {
				exitval = db_load_usage();
				goto done;
			}
			mode = STANDARD_LOAD;

			*clp++ = optarg;
			break;
		case 'f':
			if (mode != NOTSET && mode != STANDARD_LOAD) {
				exitval = db_load_usage();
				goto done;
			}
			mode = STANDARD_LOAD;

			if (freopen(optarg, "r", stdin) == NULL) {
				fprintf(stderr, "%s: %s: reopen: %s\n",
				    ldg.progname, optarg, strerror(errno));
				exitval = db_load_usage();
				goto done;
			}
			break;
		case 'h':
			ldg.home = optarg;
			break;
		case 'n':
			if (mode != NOTSET && mode != STANDARD_LOAD) {
				exitval = db_load_usage();
				goto done;
			}
			mode = STANDARD_LOAD;

			ldf |= LDF_NOOVERWRITE;
			break;
		case 'P':
			ldg.passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (ldg.passwd == NULL) {
				fprintf(stderr, "%s: strdup: %s\n",
				    ldg.progname, strerror(errno));
				exitval = db_load_usage();
				goto done;
			}
			ldf |= LDF_PASSWORD;
			break;
		case 'r':
			if (mode == STANDARD_LOAD) {
				exitval = db_load_usage();
				goto done;
			}
			if (strcmp(optarg, "lsn") == 0)
				mode = LSN_RESET;
			else if (strcmp(optarg, "fileid") == 0)
				mode = FILEID_RESET;
			else {
				exitval = db_load_usage();
				goto done;
			}
			break;
		case 'T':
			if (mode != NOTSET && mode != STANDARD_LOAD) {
				exitval = db_load_usage();
				goto done;
			}
			mode = STANDARD_LOAD;

			ldf |= LDF_NOHEADER;
			break;
		case 't':
			if (mode != NOTSET && mode != STANDARD_LOAD) {
				exitval = db_load_usage();
				goto done;
			}
			mode = STANDARD_LOAD;

			if (strcmp(optarg, "btree") == 0) {
				dbtype = DB_BTREE;
				break;
			}
			if (strcmp(optarg, "hash") == 0) {
				dbtype = DB_HASH;
				break;
			}
			if (strcmp(optarg, "recno") == 0) {
				dbtype = DB_RECNO;
				break;
			}
			if (strcmp(optarg, "queue") == 0) {
				dbtype = DB_QUEUE;
				break;
			}
			exitval = db_load_usage();
			goto done;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case '?':
		default:
			exitval = db_load_usage();
			goto done;
		}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		exitval = db_load_usage();
		goto done;
	}

	/* Handle possible interruptions. */
	__db_util_siginit();

	/*
	 * Create an environment object initialized for error reporting, and
	 * then open it.
	 */
	if (db_load_env_create(&dbenv, &ldg) != 0)
		goto shutdown;

	/* If we're resetting the LSNs, that's an entirely separate path. */
	switch (mode) {
	case FILEID_RESET:
		exitval = dbenv->fileid_reset(
		    dbenv, argv[0], ldf & LDF_PASSWORD ? DB_ENCRYPT : 0);
		break;
	case LSN_RESET:
		exitval = dbenv->lsn_reset(
		    dbenv, argv[0], ldf & LDF_PASSWORD ? DB_ENCRYPT : 0);
		break;
	case NOTSET:
	case STANDARD_LOAD:
		while (!ldg.endofile)
			if (db_load_load(dbenv, argv[0], dbtype, clist, ldf,
			    &ldg, &existed) != 0)
				goto shutdown;
		break;
	}

	if (0) {
shutdown:	exitval = 1;
	}
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", ldg.progname, db_strerror(ret));
	}

	/* Resend any caught signal. */
	__db_util_sigresend();
	free(clist);
	if (ldg.passwd != NULL)
		free(ldg.passwd);

	/*
	 * Return 0 on success, 1 if keys existed already, and 2 on failure.
	 *
	 * Technically, this is wrong, because exit of anything other than
	 * 0 is implementation-defined by the ANSI C standard.  I don't see
	 * any good solutions that don't involve API changes.
	 */
done:
	return (exitval == 0 ? (existed == 0 ? 0 : 1) : 2);
}

/*
 * load --
 *	Load a database.
 */
int
db_load_load(dbenv, name, argtype, clist, flags, ldg, existedp)
	DB_ENV *dbenv;
	char *name, **clist;
	DBTYPE argtype;
	u_int flags;
	LDG *ldg;
	int *existedp;
{
	DB *dbp;
	DBC *dbc;
	DBT key, rkey, data, *readp, *writep;
	DBTYPE dbtype;
	DB_TXN *ctxn, *txn;
	db_recno_t recno, datarecno;
	u_int32_t put_flags;
	int ascii_recno, checkprint, hexkeys, keyflag, keys, resize, ret, rval;
	char *subdb;

	put_flags = LF_ISSET(LDF_NOOVERWRITE) ? DB_NOOVERWRITE : 0;
	G(endodata) = 0;

	dbc = NULL;
	subdb = NULL;
	ctxn = txn = NULL;
	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	memset(&rkey, 0, sizeof(DBT));

retry_db:
	dbtype = DB_UNKNOWN;
	keys = -1;
	hexkeys = -1;
	keyflag = -1;

	/* Create the DB object. */
	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		goto err;
	}

	/* Read the header -- if there's no header, we expect flat text. */
	if (LF_ISSET(LDF_NOHEADER)) {
		checkprint = 1;
		dbtype = argtype;
	} else {
		if (db_load_rheader(dbenv,
		    dbp, &dbtype, &subdb, &checkprint, &keys) != 0)
			goto err;
		if (G(endofile))
			goto done;
	}

	/*
	 * Apply command-line configuration changes.  (We apply command-line
	 * configuration changes to all databases that are loaded, e.g., all
	 * subdatabases.)
	 */
	if (db_load_configure(dbenv, dbp, clist, &subdb, &keyflag))
		goto err;

	if (keys != 1) {
		if (keyflag == 1) {
			dbp->err(dbp, EINVAL, "No keys specified in file");
			goto err;
		}
	}
	else if (keyflag == 0) {
		dbp->err(dbp, EINVAL, "Keys specified in file");
		goto err;
	}
	else
		keyflag = 1;

	if (dbtype == DB_BTREE || dbtype == DB_HASH) {
		if (keyflag == 0)
			dbp->err(dbp,
			    EINVAL, "Btree and Hash must specify keys");
		else
			keyflag = 1;
	}

	if (argtype != DB_UNKNOWN) {

		if (dbtype == DB_RECNO || dbtype == DB_QUEUE)
			if (keyflag != 1 && argtype != DB_RECNO &&
			    argtype != DB_QUEUE) {
				dbenv->errx(dbenv,
			   "improper database type conversion specified");
				goto err;
			}
		dbtype = argtype;
	}

	if (dbtype == DB_UNKNOWN) {
		dbenv->errx(dbenv, "no database type specified");
		goto err;
	}

	if (keyflag == -1)
		keyflag = 0;

	/*
	 * Recno keys have only been printed in hexadecimal starting
	 * with db_dump format version 3 (DB 3.2).
	 *
	 * !!!
	 * Note that version is set in db_load_rheader(), which must be called before
	 * this assignment.
	 */
	hexkeys = (G(version) >= 3 && keyflag == 1 && checkprint == 0);

	if (keyflag == 1 && (dbtype == DB_RECNO || dbtype == DB_QUEUE))
		ascii_recno = 1;
	else
		ascii_recno = 0;

	/* If configured with a password, encrypt databases we create. */
	if (LF_ISSET(LDF_PASSWORD) &&
	    (ret = dbp->set_flags(dbp, DB_ENCRYPT)) != 0) {
		dbp->err(dbp, ret, "DB->set_flags: DB_ENCRYPT");
		goto err;
	}

#if 0
	Set application-specific btree comparison, compression, or hash
	functions here. For example:

	if ((ret = dbp->set_bt_compare(dbp, local_comparison_func)) != 0) {
		dbp->err(dbp, ret, "DB->set_bt_compare");
		goto err;
	}
	if ((ret = dbp->set_bt_compress(dbp, local_compress_func,
	    local_decompress_func)) != 0) {
		dbp->err(dbp, ret, "DB->set_bt_compress");
		goto err;
	}
	if ((ret = dbp->set_h_hash(dbp, local_hash_func)) != 0) {
		dbp->err(dbp, ret, "DB->set_h_hash");
		goto err;
	}
#endif

	/* Open the DB file. */
	if ((ret = dbp->open(dbp, NULL, name, subdb, dbtype,
	    DB_CREATE | (TXN_ON(dbenv->env) ? DB_AUTO_COMMIT : 0),
	    DB_MODE_666)) != 0) {
		dbp->err(dbp, ret, "DB->open: %s", name);
		goto err;
	}
	if (ldg->private != 0) {
		if ((ret = __db_util_cache(dbp, &ldg->cache, &resize)) != 0)
			goto err;
		if (resize) {
			if ((ret = dbp->close(dbp, 0)) != 0)
				goto err;
			dbp = NULL;
			if ((ret = dbenv->close(dbenv, 0)) != 0)
				goto err;
			if ((ret = db_load_env_create(&dbenv, ldg)) != 0)
				goto err;
			goto retry_db;
		}
	}

	/* Initialize the key/data pair. */
	readp = writep = &key;
	if (dbtype == DB_RECNO || dbtype == DB_QUEUE) {
		key.size = sizeof(recno);
		if (keyflag) {
			key.data = &datarecno;
			if (checkprint) {
				readp = &rkey;
				goto key_data;
			}
		} else
			key.data = &recno;
	} else
key_data:	if ((readp->data = malloc(readp->ulen = 1024)) == NULL) {
			dbenv->err(dbenv, ENOMEM, NULL);
			goto err;
		}
	if ((data.data = malloc(data.ulen = 1024)) == NULL) {
		dbenv->err(dbenv, ENOMEM, NULL);
		goto err;
	}

	if (TXN_ON(dbenv->env) &&
	    (ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0)
		goto err;

	if (put_flags == 0 && (ret = dbp->cursor(dbp,
	    txn, &dbc, DB_CURSOR_BULK)) != 0)
		goto err;

	/* Get each key/data pair and add them to the database. */
	for (recno = 1; !__db_util_interrupted(); ++recno) {
		if (!keyflag) {
			if (checkprint) {
				if (db_load_dbt_rprint(dbenv, &data))
					goto err;
			} else {
				if (db_load_dbt_rdump(dbenv, &data))
					goto err;
			}
		} else {
			if (checkprint) {
				if (db_load_dbt_rprint(dbenv, readp))
					goto err;
				if (ascii_recno &&
				    db_load_dbt_to_recno(dbenv, readp, &datarecno) != 0)
					goto err;

				if (!G(endodata) && db_load_dbt_rprint(dbenv, &data))
					goto odd_count;
			} else {
				if (ascii_recno) {
					if (db_load_dbt_rrecno(dbenv, readp, hexkeys))
						goto err;
				} else
					if (db_load_dbt_rdump(dbenv, readp))
						goto err;

				if (!G(endodata) && db_load_dbt_rdump(dbenv, &data)) {
odd_count:				dbenv->errx(dbenv,
					    "odd number of key/data pairs");
					goto err;
				}
			}
		}
		if (G(endodata))
			break;
retry:
		if (put_flags != 0 && txn != NULL)
			if ((ret = dbenv->txn_begin(dbenv, txn, &ctxn, 0)) != 0)
				goto err;
		switch (ret = ((put_flags == 0) ?
		    dbc->put(dbc, writep, &data, DB_KEYLAST) :
		    dbp->put(dbp, ctxn, writep, &data, put_flags))) {
		case 0:
			if (ctxn != NULL) {
				if ((ret =
				    ctxn->commit(ctxn, DB_TXN_NOSYNC)) != 0)
					goto err;
				ctxn = NULL;
			}
			break;
		case DB_KEYEXIST:
			*existedp = 1;
			dbenv->errx(dbenv,
			    "%s: line %d: key already exists, not loaded:",
			    name,
			    !keyflag ? recno : recno * 2 - 1);

			(void)dbenv->prdbt(&key,
			    checkprint, 0, stderr, __db_pr_callback, 0);
			break;
		case DB_LOCK_DEADLOCK:
			/* If we have a child txn, retry--else it's fatal. */
			if (ctxn != NULL) {
				if ((ret = ctxn->abort(ctxn)) != 0)
					goto err;
				ctxn = NULL;
				goto retry;
			}
			/* FALLTHROUGH */
		default:
			dbenv->err(dbenv, ret, NULL);
			if (ctxn != NULL) {
				(void)ctxn->abort(ctxn);
				ctxn = NULL;
			}
			goto err;
		}
		if (ctxn != NULL) {
			if ((ret = ctxn->abort(ctxn)) != 0)
				goto err;
			ctxn = NULL;
		}
	}
done:	rval = 0;
	if (dbc != NULL && (ret = dbc->close(dbc)) != 0) {
		dbc = NULL;
		goto err;
	}
	if (txn != NULL && (ret = txn->commit(txn, 0)) != 0) {
		txn = NULL;
		goto err;
	}

	if (0) {
err:		rval = 1;
		if (dbc != NULL)
			(void)dbc->close(dbc);
		if (txn != NULL)
			(void)txn->abort(txn);
	}

	/* Close the database. */
	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->close");
		rval = 1;
	}

	if (G(hdrbuf) != NULL)
		free(G(hdrbuf));
	G(hdrbuf) = NULL;
	/* Free allocated memory. */
	if (subdb != NULL)
		free(subdb);
	if (dbtype != DB_RECNO && dbtype != DB_QUEUE && key.data != NULL)
		free(key.data);
	if (rkey.data != NULL)
		free(rkey.data);
	free(data.data);

	return (rval);
}

/*
 * env_create --
 *	Create the environment and initialize it for error reporting.
 */
int
db_load_env_create(dbenvp, ldg)
	DB_ENV **dbenvp;
	LDG *ldg;
{
	DB_ENV *dbenv;
	int ret;

	if ((ret = db_env_create(dbenvp, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", ldg->progname, db_strerror(ret));
		return (ret);
	}
	dbenv = *dbenvp;
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, ldg->progname);
	if (ldg->passwd != NULL && (ret = dbenv->set_encrypt(dbenv,
	    ldg->passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		return (ret);
	}
	if ((ret = db_load_db_init(dbenv, ldg->home, ldg->cache, &ldg->private)) != 0)
		return (ret);
	dbenv->app_private = ldg;

	return (0);
}

/*
 * db_init --
 *	Initialize the environment.
 */
int
db_load_db_init(dbenv, home, cache, is_private)
	DB_ENV *dbenv;
	char *home;
	u_int32_t cache;
	int *is_private;
{
	u_int32_t flags;
	int ret;

	*is_private = 0;
	/* We may be loading into a live environment.  Try and join. */
	flags = DB_USE_ENVIRON |
	    DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN;
	if ((ret = dbenv->open(dbenv, home, flags, 0)) == 0)
		return (0);
	if (ret == DB_VERSION_MISMATCH)
		goto err;

	/*
	 * We're trying to load a database.
	 *
	 * An environment is required because we may be trying to look at
	 * databases in directories other than the current one.  We could
	 * avoid using an environment iff the -h option wasn't specified,
	 * but that seems like more work than it's worth.
	 *
	 * No environment exists (or, at least no environment that includes
	 * an mpool region exists).  Create one, but make it private so that
	 * no files are actually created.
	 */
	LF_CLR(DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_TXN);
	LF_SET(DB_CREATE | DB_PRIVATE);
	*is_private = 1;
	if ((ret = dbenv->set_cachesize(dbenv, 0, cache, 1)) != 0) {
		dbenv->err(dbenv, ret, "set_cachesize");
		return (1);
	}
	if ((ret = dbenv->open(dbenv, home, flags, 0)) == 0)
		return (0);

	/* An environment is required. */
err:	dbenv->err(dbenv, ret, "DB_ENV->open");
	return (1);
}

#define	FLAG(name, value, keyword, flag)				\
	if (strcmp(name, keyword) == 0) {				\
		switch (*value) {					\
		case '1':						\
			if ((ret = dbp->set_flags(dbp, flag)) != 0) {	\
				dbp->err(dbp, ret, "%s: set_flags: %s",	\
				    G(progname), name);			\
				goto err;				\
			}						\
			break;						\
		case '0':						\
			break;						\
		default:						\
			db_load_badnum(dbenv);					\
			goto err;					\
		}							\
		continue;						\
	}
#define	NUMBER(name, value, keyword, func, t)				\
	if (strcmp(name, keyword) == 0) {				\
		if ((ret = __db_getlong(dbenv,				\
		    NULL, value, 0, LONG_MAX, &val)) != 0 ||		\
		    (ret = dbp->func(dbp, (t)val)) != 0)		\
			goto nameerr;					\
		continue;						\
	}
#define	STRING(name, value, keyword, func)				\
	if (strcmp(name, keyword) == 0) {				\
		if ((ret = dbp->func(dbp, value[0])) != 0)		\
			goto nameerr;					\
		continue;						\
	}

/*
 * The code to check a command-line or input header argument against a list
 * of configuration options.  It's #defined because it's used in two places
 * and the two places have gotten out of sync more than once.
 */
#define	CONFIGURATION_LIST_COMPARE					\
	NUMBER(name, value, "bt_minkey", set_bt_minkey, u_int32_t);	\
	  FLAG(name, value, "chksum", DB_CHKSUM);			\
	NUMBER(name, value, "db_lorder", set_lorder, int);		\
	NUMBER(name, value, "db_pagesize", set_pagesize, u_int32_t);	\
	  FLAG(name, value, "duplicates", DB_DUP);			\
	  FLAG(name, value, "dupsort", DB_DUPSORT);			\
	NUMBER(name, value, "extentsize", set_q_extentsize, u_int32_t);	\
	NUMBER(name, value, "h_ffactor", set_h_ffactor, u_int32_t);	\
	NUMBER(name, value, "h_nelem", set_h_nelem, u_int32_t);		\
	NUMBER(name, value, "re_len", set_re_len, u_int32_t);		\
	STRING(name, value, "re_pad", set_re_pad);			\
	  FLAG(name, value, "recnum", DB_RECNUM);			\
	  FLAG(name, value, "renumber", DB_RENUMBER);			\
	if (strcmp(name, "compressed") == 0) {				\
		switch (*value) {					\
		case '1':						\
			if ((ret = dbp->set_bt_compress(dbp, NULL,	\
				NULL)) != 0)				\
				goto nameerr;				\
			break;						\
		case '0':						\
			break;						\
		default:						\
			db_load_badnum(dbenv);					\
			goto err;					\
		}							\
		continue;						\
	}

/*
 * configure --
 *	Handle command-line configuration options.
 */
int
db_load_configure(dbenv, dbp, clp, subdbp, keysp)
	DB_ENV *dbenv;
	DB *dbp;
	char **clp, **subdbp;
	int *keysp;
{
	long val;
	int ret, savech;
	char *name, *value;

	for (; (name = *clp) != NULL; *--value = savech, ++clp) {
		if ((value = strchr(name, '=')) == NULL) {
			dbp->errx(dbp,
		    "command-line configuration uses name=value format");
			return (1);
		}
		savech = *value;
		*value++ = '\0';

		if (strcmp(name, "database") == 0 ||
		    strcmp(name, "subdatabase") == 0) {
			if (*subdbp != NULL)
				free(*subdbp);
			if ((*subdbp = strdup(value)) == NULL) {
				dbp->err(dbp, ENOMEM, NULL);
				return (1);
			}
			continue;
		}
		if (strcmp(name, "keys") == 0) {
			if (strcmp(value, "1") == 0)
				*keysp = 1;
			else if (strcmp(value, "0") == 0)
				*keysp = 0;
			else {
				db_load_badnum(dbenv);
				return (1);
			}
			continue;
		}

		CONFIGURATION_LIST_COMPARE;

		dbp->errx(dbp,
		    "unknown command-line configuration keyword \"%s\"", name);
		return (1);
	}
	return (0);

nameerr:
	dbp->err(dbp, ret, "%s: %s=%s", G(progname), name, value);
err:	return (1);
}

/*
 * rheader --
 *	Read the header message.
 */
int
db_load_rheader(dbenv, dbp, dbtypep, subdbp, checkprintp, keysp)
	DB_ENV *dbenv;
	DB *dbp;
	DBTYPE *dbtypep;
	char **subdbp;
	int *checkprintp, *keysp;
{
	DBT *keys, *kp;
	size_t buflen, linelen, start;
	long val;
	int ch, first, hdr, ret;
	char *buf, *name, *p, *value;
	u_int32_t i, nparts;

	*dbtypep = DB_UNKNOWN;
	*checkprintp = 0;
	name = NULL;

	/*
	 * We start with a smallish buffer;  most headers are small.
	 * We may need to realloc it for a large subdatabase name.
	 */
	buflen = 4096;
	if (G(hdrbuf) == NULL) {
		hdr = 0;
		if ((buf = malloc(buflen)) == NULL)
			goto memerr;
		G(hdrbuf) = buf;
		G(origline) = G(lineno);
	} else {
		hdr = 1;
		buf = G(hdrbuf);
		G(lineno) = G(origline);
	}

	start = 0;
	for (first = 1;; first = 0) {
		++G(lineno);

		/* Read a line, which may be of arbitrary length, into buf. */
		linelen = 0;
		buf = &G(hdrbuf)[start];
		if (hdr == 0) {
			for (;;) {
				if ((ch = getchar()) == EOF) {
					if (!first || ferror(stdin))
						goto badfmt;
					G(endofile) = 1;
					break;
				}

				/*
				 * If the buffer is too small, double it.
				 */
				if (linelen + start == buflen) {
					G(hdrbuf) =
					    realloc(G(hdrbuf), buflen *= 2);
					if (G(hdrbuf) == NULL)
						goto memerr;
					buf = &G(hdrbuf)[start];
				}

				if (ch == '\n')
					break;

				buf[linelen++] = ch;
			}
			if (G(endofile) == 1)
				break;
			buf[linelen++] = '\0';
		} else
			linelen = strlen(buf) + 1;
		start += linelen;

		if (name != NULL) {
			free(name);
			name = NULL;
		}
		/* If we don't see the expected information, it's an error. */
		if ((name = strdup(buf)) == NULL)
			goto memerr;
		if ((p = strchr(name, '=')) == NULL)
			goto badfmt;
		*p++ = '\0';

		value = p--;

		if (name[0] == '\0')
			goto badfmt;

		/*
		 * The only values that may be zero-length are database names.
		 * In the original Berkeley DB code it was possible to create
		 * zero-length database names, and the db_load code was then
		 * changed to allow such databases to be be dumped and loaded.
		 * [#8204]
		 */
		if (strcmp(name, "database") == 0 ||
		    strcmp(name, "subdatabase") == 0) {
			if ((ret = db_load_convprintable(dbenv, value, subdbp)) != 0) {
				dbp->err(dbp, ret, "error reading db name");
				goto err;
			}
			continue;
		}

		/* No other values may be zero-length. */
		if (value[0] == '\0')
			goto badfmt;

		if (strcmp(name, "HEADER") == 0)
			break;
		if (strcmp(name, "VERSION") == 0) {
			/*
			 * Version 1 didn't have a "VERSION" header line.  We
			 * only support versions 1, 2, and 3 of the dump format.
			 */
			G(version) = atoi(value);

			if (G(version) > 3) {
				dbp->errx(dbp,
				    "line %lu: VERSION %d is unsupported",
				    G(lineno), G(version));
				goto err;
			}
			continue;
		}
		if (strcmp(name, "format") == 0) {
			if (strcmp(value, "bytevalue") == 0) {
				*checkprintp = 0;
				continue;
			}
			if (strcmp(value, "print") == 0) {
				*checkprintp = 1;
				continue;
			}
			goto badfmt;
		}
		if (strcmp(name, "type") == 0) {
			if (strcmp(value, "btree") == 0) {
				*dbtypep = DB_BTREE;
				continue;
			}
			if (strcmp(value, "hash") == 0) {
				*dbtypep = DB_HASH;
				continue;
			}
			if (strcmp(value, "recno") == 0) {
				*dbtypep = DB_RECNO;
				continue;
			}
			if (strcmp(value, "queue") == 0) {
				*dbtypep = DB_QUEUE;
				continue;
			}
			dbp->errx(dbp, "line %lu: unknown type", G(lineno));
			goto err;
		}
		if (strcmp(name, "keys") == 0) {
			if (strcmp(value, "1") == 0)
				*keysp = 1;
			else if (strcmp(value, "0") == 0)
				*keysp = 0;
			else {
				db_load_badnum(dbenv);
				goto err;
			}
			continue;
		}
		if (strcmp(name, "nparts") == 0) {
			if ((ret = __db_getlong(dbenv,
			    NULL, value, 0, LONG_MAX, &val)) != 0) {
				db_load_badnum(dbenv);
				goto err;
			}
			nparts = (u_int32_t) val;
			if ((keys =
			    malloc((nparts - 1) * sizeof(DBT))) == NULL) {
				dbenv->err(dbenv, ENOMEM, NULL);
				goto err;
			}
			kp = keys;
			for (i = 1; i < nparts; kp++, i++) {
				if ((kp->data =
				     malloc(kp->ulen = 1024)) == NULL) {
					dbenv->err(dbenv, ENOMEM, NULL);
					goto err;
				}
				if (*checkprintp) {
					if (db_load_dbt_rprint(dbenv, kp))
						goto err;
				} else {
					if (db_load_dbt_rdump(dbenv, kp))
						goto err;
				}
			}
			if ((ret = dbp->set_partition(
			     dbp, nparts, keys, NULL)) != 0)
				goto err;

			continue;
		}

		CONFIGURATION_LIST_COMPARE;

		dbp->errx(dbp,
		    "unknown input-file header configuration keyword \"%s\"",
		    name);
		goto err;
	}
	ret = 0;

	if (0) {
nameerr:	dbp->err(dbp, ret, "%s: %s=%s", G(progname), name, value);
		ret = 1;
	}
	if (0) {
badfmt:		dbp->errx(dbp, "line %lu: unexpected format", G(lineno));
		ret = 1;
	}
	if (0) {
memerr:		dbp->errx(dbp, "unable to allocate memory");
err:		ret = 1;
	}
	if (name != NULL)
		free(name);
	return (ret);
}

/*
 * Macro to convert a pair of hex bytes to a decimal value.
 *
 * !!!
 * Note that this macro is side-effect safe.  This was done deliberately,
 * callers depend on it.
 */
#define	DIGITIZE(store, v1, v2) {					\
	char _v1, _v2;							\
	_v1 = (v1);							\
	_v2 = (v2);							\
	if ((_v1) > 'f' || (_v2) > 'f')					\
		return (db_load_badend(dbenv));					\
	(store) =							\
	((_v1) == '0' ? 0 :						\
	((_v1) == '1' ? 1 :						\
	((_v1) == '2' ? 2 :						\
	((_v1) == '3' ? 3 :						\
	((_v1) == '4' ? 4 :						\
	((_v1) == '5' ? 5 :						\
	((_v1) == '6' ? 6 :						\
	((_v1) == '7' ? 7 :						\
	((_v1) == '8' ? 8 :						\
	((_v1) == '9' ? 9 :						\
	((_v1) == 'a' ? 10 :						\
	((_v1) == 'b' ? 11 :						\
	((_v1) == 'c' ? 12 :						\
	((_v1) == 'd' ? 13 :						\
	((_v1) == 'e' ? 14 : 15))))))))))))))) << 4 |			\
	((_v2) == '0' ? 0 :						\
	((_v2) == '1' ? 1 :						\
	((_v2) == '2' ? 2 :						\
	((_v2) == '3' ? 3 :						\
	((_v2) == '4' ? 4 :						\
	((_v2) == '5' ? 5 :						\
	((_v2) == '6' ? 6 :						\
	((_v2) == '7' ? 7 :						\
	((_v2) == '8' ? 8 :						\
	((_v2) == '9' ? 9 :						\
	((_v2) == 'a' ? 10 :						\
	((_v2) == 'b' ? 11 :						\
	((_v2) == 'c' ? 12 :						\
	((_v2) == 'd' ? 13 :						\
	((_v2) == 'e' ? 14 : 15)))))))))))))));				\
}

/*
 * convprintable --
 *	Convert a printable-encoded string into a newly allocated string.
 *
 * In an ideal world, this would probably share code with dbt_rprint, but
 * that's set up to read character-by-character (to avoid large memory
 * allocations that aren't likely to be a problem here), and this has fewer
 * special cases to deal with.
 *
 * Note that despite the printable encoding, the char * interface to this
 * function (which is, not coincidentally, also used for database naming)
 * means that outstr cannot contain any nuls.
 */
int
db_load_convprintable(dbenv, instr, outstrp)
	DB_ENV *dbenv;
	char *instr, **outstrp;
{
	char *outstr;

	/*
	 * Just malloc a string big enough for the whole input string;
	 * the output string will be smaller (or of equal length).
	 *
	 * Note that we may be passed a zero-length string and need to
	 * be able to duplicate it.
	 */
	if ((outstr = malloc(strlen(instr) + 1)) == NULL)
		return (ENOMEM);

	*outstrp = outstr;

	for ( ; *instr != '\0'; instr++)
		if (*instr == '\\') {
			if (*++instr == '\\') {
				*outstr++ = '\\';
				continue;
			}
			DIGITIZE(*outstr++, *instr, *++instr);
		} else
			*outstr++ = *instr;

	*outstr = '\0';

	return (0);
}

/*
 * dbt_rprint --
 *	Read a printable line into a DBT structure.
 */
int
db_load_dbt_rprint(dbenv, dbtp)
	DB_ENV *dbenv;
	DBT *dbtp;
{
	u_int32_t len;
	u_int8_t *p;
	int c1, c2, escape, first;
	char buf[32];

	++G(lineno);

	first = 1;
	escape = 0;
	for (p = dbtp->data, len = 0; (c1 = getchar()) != '\n';) {
		if (c1 == EOF) {
			if (len == 0) {
				G(endofile) = G(endodata) = 1;
				return (0);
			}
			return (db_load_badend(dbenv));
		}
		if (first) {
			first = 0;
			if (G(version) > 1) {
				if (c1 != ' ') {
					buf[0] = c1;
					if (fgets(buf + 1,
					    sizeof(buf) - 1, stdin) == NULL ||
					    strcmp(buf, "DATA=END\n") != 0)
						return (db_load_badend(dbenv));
					G(endodata) = 1;
					return (0);
				}
				continue;
			}
		}
		if (escape) {
			if (c1 != '\\') {
				if ((c2 = getchar()) == EOF)
					return (db_load_badend(dbenv));
				DIGITIZE(c1, c1, c2);
			}
			escape = 0;
		} else
			if (c1 == '\\') {
				escape = 1;
				continue;
			}
		if (len >= dbtp->ulen - 10) {
			dbtp->ulen *= 2;
			if ((dbtp->data =
			    realloc(dbtp->data, dbtp->ulen)) == NULL) {
				dbenv->err(dbenv, ENOMEM, NULL);
				return (1);
			}
			p = (u_int8_t *)dbtp->data + len;
		}
		++len;
		*p++ = c1;
	}
	dbtp->size = len;

	return (0);
}

/*
 * dbt_rdump --
 *	Read a byte dump line into a DBT structure.
 */
int
db_load_dbt_rdump(dbenv, dbtp)
	DB_ENV *dbenv;
	DBT *dbtp;
{
	u_int32_t len;
	u_int8_t *p;
	int c1, c2, first;
	char buf[32];

	++G(lineno);

	first = 1;
	for (p = dbtp->data, len = 0; (c1 = getchar()) != '\n';) {
		if (c1 == EOF) {
			if (len == 0) {
				G(endofile) = G(endodata) = 1;
				return (0);
			}
			return (db_load_badend(dbenv));
		}
		if (first) {
			first = 0;
			if (G(version) > 1) {
				if (c1 != ' ') {
					buf[0] = c1;
					if (fgets(buf + 1,
					    sizeof(buf) - 1, stdin) == NULL ||
					    strcmp(buf, "DATA=END\n") != 0)
						return (db_load_badend(dbenv));
					G(endodata) = 1;
					return (0);
				}
				continue;
			}
		}
		if ((c2 = getchar()) == EOF)
			return (db_load_badend(dbenv));
		if (len >= dbtp->ulen - 10) {
			dbtp->ulen *= 2;
			if ((dbtp->data =
			    realloc(dbtp->data, dbtp->ulen)) == NULL) {
				dbenv->err(dbenv, ENOMEM, NULL);
				return (1);
			}
			p = (u_int8_t *)dbtp->data + len;
		}
		++len;
		DIGITIZE(*p++, c1, c2);
	}
	dbtp->size = len;

	return (0);
}

/*
 * dbt_rrecno --
 *	Read a record number dump line into a DBT structure.
 */
int
db_load_dbt_rrecno(dbenv, dbtp, ishex)
	DB_ENV *dbenv;
	DBT *dbtp;
	int ishex;
{
	char buf[32], *p, *q;
	u_long recno;

	++G(lineno);

	if (fgets(buf, sizeof(buf), stdin) == NULL) {
		G(endofile) = G(endodata) = 1;
		return (0);
	}

	if (strcmp(buf, "DATA=END\n") == 0) {
		G(endodata) = 1;
		return (0);
	}

	if (buf[0] != ' ')
		goto err;

	/*
	 * If we're expecting a hex key, do an in-place conversion
	 * of hex to straight ASCII before calling __db_getulong().
	 */
	if (ishex) {
		for (p = q = buf + 1; *q != '\0' && *q != '\n';) {
			/*
			 * 0-9 in hex are 0x30-0x39, so this is easy.
			 * We should alternate between 3's and [0-9], and
			 * if the [0-9] are something unexpected,
			 * __db_getulong will fail, so we only need to catch
			 * end-of-string conditions.
			 */
			if (*q++ != '3')
				goto err;
			if (*q == '\n' || *q == '\0')
				goto err;
			*p++ = *q++;
		}
		*p = '\0';
	}

	if (__db_getulong(dbenv, G(progname), buf + 1, 0, 0, &recno))
		goto err;

	*((db_recno_t *)dbtp->data) = recno;
	dbtp->size = sizeof(db_recno_t);
	return (0);

err:	return (db_load_badend(dbenv));
}

int
db_load_dbt_to_recno(dbenv, dbt, recnop)
	DB_ENV *dbenv;
	DBT *dbt;
	db_recno_t *recnop;
{
	char buf[32];				/* Large enough for 2^64. */

	memcpy(buf, dbt->data, dbt->size);
	buf[dbt->size] = '\0';

	return (__db_getulong(dbenv, G(progname), buf, 0, 0, (u_long *)recnop));
}

/*
 * badnum --
 *	Display the bad number message.
 */
void
db_load_badnum(dbenv)
	DB_ENV *dbenv;
{
	dbenv->errx(dbenv,
	    "boolean name=value pairs require a value of 0 or 1");
}

/*
 * badend --
 *	Display the bad end to input message.
 */
int
db_load_badend(dbenv)
	DB_ENV *dbenv;
{
	dbenv->errx(dbenv, "unexpected end of input data or key/data pair");
	return (1);
}

/*
 * usage --
 *	Display the usage message.
 */
int
db_load_usage()
{
	(void)fprintf(stderr, "usage: %s %s\n\t%s\n", progname,
	    "[-nTV] [-c name=value] [-f file]",
    "[-h home] [-P password] [-t btree | hash | recno | queue] db_file");
	(void)fprintf(stderr, "usage: %s %s\n",
	    progname, "-r lsn | fileid [-h home] [-P password] db_file");
	return (EXIT_FAILURE);
}

int
db_load_version_check()
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
