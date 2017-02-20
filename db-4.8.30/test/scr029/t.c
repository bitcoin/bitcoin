/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

#include <sys/types.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

#define	ENV {								\
	if (dbenv != NULL)						\
		assert(dbenv->close(dbenv, 0) == 0);			\
	assert(db_env_create(&dbenv, 0) == 0);				\
	dbenv->set_errfile(dbenv, stderr);				\
}

int
main()
{
	const u_int8_t *lk_conflicts;
	DB_ENV *dbenv;
	db_timeout_t timeout;
	u_int32_t a, b, c, v;
	int nmodes, lk_modes;
	u_int8_t conflicts[40];

	dbenv = NULL;

	/* tx_max: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_tx_max(dbenv, 37) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_TXN, 0666) == 0);
	assert(dbenv->get_tx_max(dbenv, &v) == 0);
	assert(v == 37);
	ENV
	assert(dbenv->set_tx_max(dbenv, 63) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_tx_max(dbenv, &v) == 0);
	assert(v == 37);

	/* lg_max: reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lg_max(dbenv, 37 * 1024 * 1024) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOG, 0666) == 0);
	assert(dbenv->get_lg_max(dbenv, &v) == 0);
	assert(v == 37 * 1024 * 1024);
	ENV
	assert(dbenv->set_lg_max(dbenv, 63 * 1024 * 1024) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lg_max(dbenv, &v) == 0);
	assert(v == 63 * 1024 * 1024);

	/* lg_bsize: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lg_bsize(dbenv, 37 * 1024) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOG, 0666) == 0);
	assert(dbenv->get_lg_bsize(dbenv, &v) == 0);
	assert(v == 37 * 1024);
	ENV
	assert(dbenv->set_lg_bsize(dbenv, 63 * 1024) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lg_bsize(dbenv, &v) == 0);
	assert(v == 37 * 1024);

	/* lg_regionmax: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lg_regionmax(dbenv, 137 * 1024) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOG, 0666) == 0);
	assert(dbenv->get_lg_regionmax(dbenv, &v) == 0);
	assert(v == 137 * 1024);
	ENV
	assert(dbenv->set_lg_regionmax(dbenv, 163 * 1024) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lg_regionmax(dbenv, &v) == 0);
	assert(v == 137 * 1024);

	/* lk_get_lk_conflicts: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	memset(conflicts, 'a', sizeof(conflicts));
	nmodes = 6;
	assert(dbenv->set_lk_conflicts(dbenv, conflicts, nmodes) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_lk_conflicts(dbenv, &lk_conflicts, &lk_modes) == 0);
	assert(lk_conflicts[0] == 'a');
	assert(lk_modes == 6);
	ENV
	memset(conflicts, 'b', sizeof(conflicts));
	nmodes = 8;
	assert(dbenv->set_lk_conflicts(dbenv, conflicts, nmodes) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lk_conflicts(dbenv, &lk_conflicts, &lk_modes) == 0);
	assert(lk_conflicts[0] == 'a');
	assert(lk_modes == 6);

	/* lk_detect: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lk_detect(dbenv, DB_LOCK_MAXLOCKS) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_lk_detect(dbenv, &v) == 0);
	assert(v == DB_LOCK_MAXLOCKS);
	ENV
	assert(dbenv->set_lk_detect(dbenv, DB_LOCK_DEFAULT) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lk_detect(dbenv, &v) == 0);
	assert(v == DB_LOCK_MAXLOCKS);

	/* lk_max_locks: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lk_max_locks(dbenv, 37) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_lk_max_locks(dbenv, &v) == 0);
	assert(v == 37);
	ENV
	assert(dbenv->set_lk_max_locks(dbenv, 63) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lk_max_locks(dbenv, &v) == 0);
	assert(v == 37);

	/* lk_max_lockers: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lk_max_lockers(dbenv, 37) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_lk_max_lockers(dbenv, &v) == 0);
	assert(v == 37);
	ENV
	assert(dbenv->set_lk_max_lockers(dbenv, 63) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lk_max_lockers(dbenv, &v) == 0);
	assert(v == 37);

	/* lk_max_objects: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_lk_max_objects(dbenv, 37) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_lk_max_objects(dbenv, &v) == 0);
	assert(v == 37);
	ENV
	assert(dbenv->set_lk_max_objects(dbenv, 63) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_lk_max_objects(dbenv, &v) == 0);
	assert(v == 37);

	/* lock timeout: reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_timeout(dbenv, 37, DB_SET_LOCK_TIMEOUT) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_timeout(dbenv, &timeout, DB_SET_LOCK_TIMEOUT) == 0);
	assert(timeout == 37);
	ENV
	assert(dbenv->set_timeout(dbenv, 63, DB_SET_LOCK_TIMEOUT) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_timeout(dbenv, &timeout, DB_SET_LOCK_TIMEOUT) == 0);
	assert(timeout == 63);

	/* txn timeout: reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_timeout(dbenv, 37, DB_SET_TXN_TIMEOUT) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_LOCK, 0666) == 0);
	assert(dbenv->get_timeout(dbenv, &timeout, DB_SET_TXN_TIMEOUT) == 0);
	assert(timeout == 37);
	ENV
	assert(dbenv->set_timeout(dbenv, 63, DB_SET_TXN_TIMEOUT) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_timeout(dbenv, &timeout, DB_SET_TXN_TIMEOUT) == 0);
	assert(timeout == 63);

	/* cache size: NOT reset at run-time. */
	system("rm -rf TESTDIR; mkdir TESTDIR");
	ENV
	assert(dbenv->set_cachesize(dbenv, 1, 37, 3) == 0);
	assert(dbenv->open(dbenv,
	    "TESTDIR", DB_CREATE | DB_INIT_MPOOL, 0666) == 0);
	assert(dbenv->get_cachesize(dbenv, &a, &b, &c) == 0);
	assert(a == 1 && b == 37 && c == 3);
	ENV
	assert(dbenv->set_cachesize(dbenv, 2, 63, 1) == 0);
	assert(dbenv->open(dbenv, "TESTDIR", DB_JOINENV, 0666) == 0);
	assert(dbenv->get_cachesize(dbenv, &a, &b, &c) == 0);
	assert(a == 1 && b == 37 && c == 3);

	return (0);
}
