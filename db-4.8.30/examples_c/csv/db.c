/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "csv.h"
#include "csv_local.h"
#include "csv_extern.h"

static int compare_uint32(DB *, const DBT *, const DBT *);

/*
 * csv_env_init --
 *	Initialize the database environment.
 */
int
csv_env_open(const char *home, int is_rdonly)
{
	int ret;

	dbenv = NULL;
	db = NULL;

	/* Create a database environment handle. */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (1);
	}

	/*
	 * Configure Berkeley DB error reporting to stderr, with our program
	 * name as the prefix.
	 */
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	/*
	 * The default Berkeley DB cache size is fairly small; configure a
	 * 1MB cache for now.  This value will require tuning in the future.
	 */
	if ((ret = dbenv->set_cachesize(dbenv, 0, 1048576, 1)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_cachesize");
		return (1);
	}

	/*
	 * We may be working with an existing environment -- try and join it.
	 * If that fails, create a new database environment; for now, we only
	 * need a cache, no logging, locking, or transactions.
	 */
	if ((ret = dbenv->open(dbenv, home,
	    DB_JOINENV | DB_USE_ENVIRON, 0)) != 0 &&
	    (ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		return (1);
	}

	/* Create the primary database handle. */
	if ((ret = db_create(&db, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		return (1);
	}

	/*
	 * Records may be relatively large -- use a large page size.
	 */
	if ((ret = db->set_pagesize(db, 32 * 1024)) != 0) {
		dbenv->err(dbenv, ret, "DB->set_pagesize");
		return (1);
	}

	/*
	 * The primary database uses an integer as its key; on little-endian
	 * machines, integers sort badly using the default Berkeley DB sort
	 * function (which is lexicographic).  Specify a comparison function
	 * for the database.
	 */
	if ((ret = db->set_bt_compare(db, compare_uint32)) != 0) {
		dbenv->err(dbenv, ret, "DB->set_bt_compare");
		return (1);
	}

	/* Open the primary database. */
	if ((ret = db->open(db, NULL,
	    "primary", NULL, DB_BTREE, is_rdonly ? 0 : DB_CREATE, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open: primary");
		return (1);
	}

	/* Open the secondaries.  */
	if ((ret = csv_secondary_open()) != 0)
		return (1);

	return (0);
}

/*
 * csv_env_close --
 *	Discard the database environment.
 */
int
csv_env_close()
{
	int ret, t_ret;

	ret = 0;

	/* Close the secondaries. */
	ret = csv_secondary_close();

	/* Close the primary handle. */
	if (db != NULL && (t_ret = db->close(db, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->close");
		if (ret == 0)
			ret = t_ret;
	}
	if ((t_ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: DB_ENV->close: %s\n", progname, db_strerror(ret));
		if (ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * csv_secondary_open --
 *	Open any secondary indices.
 */
int
csv_secondary_open()
{
	DB *sdb;
	DbField *f;
	int ret, (*fcmp)(DB *, const DBT *, const DBT *);

	/*
	 * Create secondary database handles.
	 */
	for (f = fieldlist; f->name != NULL; ++f) {
		if (f->indx == 0)
			continue;

		if ((ret = db_create(&sdb, dbenv, 0)) != 0) {
			dbenv->err(dbenv, ret, "db_create");
			return (1);
		}
		sdb->app_private = f;

		/* Keys are small, use a relatively small page size. */
		if ((ret = sdb->set_pagesize(sdb, 8 * 1024)) != 0) {
			dbenv->err(dbenv, ret, "DB->set_pagesize");
			return (1);
		}

		/*
		 * Sort the database based on the underlying type.  Skip
		 * strings, Berkeley DB defaults to lexicographic sort.
		 */
		switch (f->type) {
		case DOUBLE:
			fcmp = compare_double;
			break;
		case UNSIGNED_LONG:
			fcmp = compare_ulong;
			break;
		case NOTSET:
		case STRING:
		default:
			fcmp = NULL;
			break;
		}
		if (fcmp != NULL &&
		    (ret = sdb->set_bt_compare(sdb, fcmp)) != 0) {
			dbenv->err(dbenv, ret, "DB->set_bt_compare");
			return (1);
		}

		/* Always configure secondaries for sorted duplicates. */
		if ((ret = sdb->set_flags(sdb, DB_DUPSORT)) != 0) {
			dbenv->err(dbenv, ret, "DB->set_flags");
			return (1);
		}
		if ((ret = sdb->set_dup_compare(sdb, compare_ulong)) != 0) {
			dbenv->err(dbenv, ret, "DB->set_dup_compare");
			return (1);
		}

		if ((ret = sdb->open(
		    sdb, NULL, f->name, NULL, DB_BTREE, DB_CREATE, 0)) != 0) {
			dbenv->err(dbenv, ret, "DB->open: %s", f->name);
			return (1);
		}
		if ((ret = sdb->associate(
		    db, NULL, sdb, secondary_callback, DB_CREATE)) != 0) {
			dbenv->err(dbenv, ret, "DB->set_associate");
			return (1);
		}
		f->secondary = sdb;
	}

	return (0);
}

/*
 * csv_secondary_close --
 *	Close any secondary indices.
 */
int
csv_secondary_close()
{
	DbField *f;
	int ret, t_ret;

	ret = 0;
	for (f = fieldlist; f->name != NULL; ++f)
		if (f->secondary != NULL && (t_ret =
		    f->secondary->close(f->secondary, 0)) != 0 && ret == 0)
			ret = t_ret;

	return (ret);
}

/*
 * compare_uint32 --
 *	Compare two keys.
 */
static int
compare_uint32(DB *db_arg, const DBT *a_arg, const DBT *b_arg)
{
	u_int32_t a, b;

	db_arg = db_arg;			/* Quiet compiler. */

	memcpy(&a, a_arg->data, sizeof(u_int32_t));
	memcpy(&b, b_arg->data, sizeof(u_int32_t));
	return (a > b ? 1 : ((a < b) ? -1 : 0));
}
