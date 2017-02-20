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
#include "dbinc/hash.h"
#include "dbinc/db_upgrade.h"

/*
 * __ham_30_hashmeta --
 *	Upgrade the database from version 4/5 to version 6.
 *
 * PUBLIC: int __ham_30_hashmeta __P((DB *, char *, u_int8_t *));
 */
int
__ham_30_hashmeta(dbp, real_name, obuf)
	DB *dbp;
	char *real_name;
	u_int8_t *obuf;
{
	ENV *env;
	HASHHDR *oldmeta;
	HMETA30 newmeta;
	u_int32_t *o_spares, *n_spares;
	u_int32_t fillf, i, maxb, max_entry, nelem;
	int ret;

	env = dbp->env;
	memset(&newmeta, 0, sizeof(newmeta));

	oldmeta = (HASHHDR *)obuf;

	/*
	 * The first 32 bytes are similar.  The only change is the version
	 * and that we removed the ovfl_point and have the page type now.
	 */

	newmeta.dbmeta.lsn = oldmeta->lsn;
	newmeta.dbmeta.pgno = oldmeta->pgno;
	newmeta.dbmeta.magic = oldmeta->magic;
	newmeta.dbmeta.version = 6;
	newmeta.dbmeta.pagesize = oldmeta->pagesize;
	newmeta.dbmeta.type = P_HASHMETA;

	/* Move flags */
	newmeta.dbmeta.flags = oldmeta->flags;

	/* Copy the free list, which has changed its name but works the same. */
	newmeta.dbmeta.free = oldmeta->last_freed;

	/* Copy: max_bucket, high_mask, low-mask, ffactor, nelem, h_charkey */
	newmeta.max_bucket = oldmeta->max_bucket;
	newmeta.high_mask = oldmeta->high_mask;
	newmeta.low_mask = oldmeta->low_mask;
	newmeta.ffactor = oldmeta->ffactor;
	newmeta.nelem = oldmeta->nelem;
	newmeta.h_charkey = oldmeta->h_charkey;

	/*
	 * There was a bug in 2.X versions where the nelem could go negative.
	 * In general, this is considered "bad."  If it does go negative
	 * (that is, very large and positive), we'll die trying to dump and
	 * load this database.  So, let's see if we can fix it here.
	 */
	nelem = newmeta.nelem;
	fillf = newmeta.ffactor;
	maxb = newmeta.max_bucket;

	if ((fillf != 0 && fillf * maxb < 2 * nelem) ||
	    (fillf == 0 && nelem > 0x8000000))
		newmeta.nelem = 0;

	/*
	 * We now have to convert the spares array.  The old spares array
	 * contained the total number of extra pages allocated prior to
	 * the bucket that begins the next doubling.  The new spares array
	 * contains the page number of the first bucket in the next doubling
	 * MINUS the bucket number of that bucket.
	 */
	o_spares = oldmeta->spares;
	n_spares = newmeta.spares;
	max_entry = __db_log2(maxb + 1);   /* highest spares entry in use */
	n_spares[0] = 1;
	for (i = 1; i < NCACHED && i <= max_entry; i++)
		n_spares[i] = 1 + o_spares[i - 1];

					/* Replace the unique ID. */
	if ((ret = __os_fileid(env, real_name, 1, newmeta.dbmeta.uid)) != 0)
		return (ret);

	/* Overwrite the original. */
	memcpy(oldmeta, &newmeta, sizeof(newmeta));

	return (0);
}

/*
 * __ham_30_sizefix --
 *	Make sure that all hash pages belonging to the current
 *	hash doubling are within the bounds of the file.
 *
 * PUBLIC: int __ham_30_sizefix __P((DB *, DB_FH *, char *, u_int8_t *));
 */
int
__ham_30_sizefix(dbp, fhp, realname, metabuf)
	DB *dbp;
	DB_FH *fhp;
	char *realname;
	u_int8_t *metabuf;
{
	u_int8_t buf[DB_MAX_PGSIZE];
	ENV *env;
	HMETA30 *meta;
	db_pgno_t last_actual, last_desired;
	int ret;
	size_t nw;
	u_int32_t pagesize;

	env = dbp->env;
	memset(buf, 0, DB_MAX_PGSIZE);

	meta = (HMETA30 *)metabuf;
	pagesize = meta->dbmeta.pagesize;

	/*
	 * Get the last page number.  To do this, we'll need dbp->pgsize
	 * to be set right, so slam it into place.
	 */
	dbp->pgsize = pagesize;
	if ((ret = __db_lastpgno(dbp, realname, fhp, &last_actual)) != 0)
		return (ret);

	/*
	 * The last bucket in the doubling is equal to high_mask;  calculate
	 * the page number that implies.
	 */
	last_desired = BS_TO_PAGE(meta->high_mask, meta->spares);

	/*
	 * If last_desired > last_actual, we need to grow the file.  Write
	 * a zeroed page where last_desired would go.
	 */
	if (last_desired > last_actual) {
		if ((ret = __os_seek(
		    env, fhp, last_desired, pagesize, 0)) != 0)
			return (ret);
		if ((ret = __os_write(env, fhp, buf, pagesize, &nw)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __ham_31_hashmeta --
 *	Upgrade the database from version 6 to version 7.
 *
 * PUBLIC: int __ham_31_hashmeta
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__ham_31_hashmeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	HMETA30 *oldmeta;
	HMETA31 *newmeta;

	COMPQUIET(dbp, NULL);
	COMPQUIET(real_name, NULL);
	COMPQUIET(fhp, NULL);

	newmeta = (HMETA31 *)h;
	oldmeta = (HMETA30 *)h;

	/*
	 * Copy the fields down the page.
	 * The fields may overlap so start at the bottom and use memmove().
	 */
	memmove(newmeta->spares, oldmeta->spares, sizeof(oldmeta->spares));
	newmeta->h_charkey = oldmeta->h_charkey;
	newmeta->nelem = oldmeta->nelem;
	newmeta->ffactor = oldmeta->ffactor;
	newmeta->low_mask = oldmeta->low_mask;
	newmeta->high_mask = oldmeta->high_mask;
	newmeta->max_bucket = oldmeta->max_bucket;
	memmove(newmeta->dbmeta.uid,
	    oldmeta->dbmeta.uid, sizeof(oldmeta->dbmeta.uid));
	newmeta->dbmeta.flags = oldmeta->dbmeta.flags;
	newmeta->dbmeta.record_count = 0;
	newmeta->dbmeta.key_count = 0;
	ZERO_LSN(newmeta->dbmeta.unused3);

	/* Update the version. */
	newmeta->dbmeta.version = 7;

	/* Upgrade the flags. */
	if (LF_ISSET(DB_DUPSORT))
		F_SET(&newmeta->dbmeta, DB_HASH_DUPSORT);

	*dirtyp = 1;
	return (0);
}

/*
 * __ham_31_hash --
 *	Upgrade the database hash leaf pages.
 *
 * PUBLIC: int __ham_31_hash
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__ham_31_hash(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	HKEYDATA *hk;
	db_pgno_t pgno, tpgno;
	db_indx_t indx;
	int ret;

	COMPQUIET(flags, 0);

	ret = 0;
	for (indx = 0; indx < NUM_ENT(h); indx += 2) {
		hk = (HKEYDATA *)H_PAIRDATA(dbp, h, indx);
		if (HPAGE_PTYPE(hk) == H_OFFDUP) {
			memcpy(&pgno, HOFFDUP_PGNO(hk), sizeof(db_pgno_t));
			tpgno = pgno;
			if ((ret = __db_31_offdup(dbp, real_name, fhp,
			    LF_ISSET(DB_DUPSORT) ? 1 : 0, &tpgno)) != 0)
				break;
			if (pgno != tpgno) {
				*dirtyp = 1;
				memcpy(HOFFDUP_PGNO(hk),
				    &tpgno, sizeof(db_pgno_t));
			}
		}
	}

	return (ret);
}

/*
 * __ham_46_hashmeta --
 *	Upgrade the database from version 8 to version 9.
 *
 * PUBLIC: int __ham_46_hashmeta
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__ham_46_hashmeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	HMETA33 *newmeta;

	COMPQUIET(dbp, NULL);
	COMPQUIET(real_name, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(fhp, NULL);

	newmeta = (HMETA33 *)h;
	/* Update the version. */
	newmeta->dbmeta.version = 9;
	*dirtyp = 1;

	return (0);
}

/*
 * __ham_46_hash --
 *	Upgrade the database hash leaf pages.
 *	From version 8 databases to version 9.
 *	Involves sorting leaf pages, no format change.
 *
 * PUBLIC: int __ham_46_hash
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__ham_46_hash(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	DBC *dbc;
	int ret, t_ret;

	COMPQUIET(real_name, NULL);
	COMPQUIET(flags, 0);
	COMPQUIET(fhp, NULL);

	if ((ret = __db_cursor(dbp, NULL, NULL, &dbc, 0)) != 0)
		return (ret);
	*dirtyp = 1;
	ret = __ham_sort_page(dbc, NULL, h);
	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
