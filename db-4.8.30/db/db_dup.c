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
#include "dbinc/mp.h"
#include "dbinc/db_am.h"

/*
 * __db_ditem_nolog --
 *	Remove an item from a page without affecting its recoverability.
 *
 * PUBLIC:  int __db_ditem_nolog __P((DBC *, PAGE *, u_int32_t, u_int32_t));
 */
int
__db_ditem_nolog(dbc, pagep, indx, nbytes)
	DBC *dbc;
	PAGE *pagep;
	u_int32_t indx, nbytes;
{
	DB *dbp;
	db_indx_t cnt, *inp, offset;
	u_int8_t *from;

	dbp = dbc->dbp;
	DB_ASSERT(dbp->env, IS_DIRTY(pagep));
	DB_ASSERT(dbp->env, indx < NUM_ENT(pagep));

	/*
	 * If there's only a single item on the page, we don't have to
	 * work hard.
	 */
	if (NUM_ENT(pagep) == 1) {
		NUM_ENT(pagep) = 0;
		HOFFSET(pagep) = dbp->pgsize;
		return (0);
	}

	inp = P_INP(dbp, pagep);
	/*
	 * Pack the remaining key/data items at the end of the page.  Use
	 * memmove(3), the regions may overlap.
	 */
	from = (u_int8_t *)pagep + HOFFSET(pagep);
	DB_ASSERT(dbp->env, inp[indx] >= HOFFSET(pagep));
	memmove(from + nbytes, from, inp[indx] - HOFFSET(pagep));
	HOFFSET(pagep) += nbytes;

	/* Adjust the indices' offsets. */
	offset = inp[indx];
	for (cnt = 0; cnt < NUM_ENT(pagep); ++cnt)
		if (inp[cnt] < offset)
			inp[cnt] += nbytes;

	/* Shift the indices down. */
	--NUM_ENT(pagep);
	if (indx != NUM_ENT(pagep))
		memmove(&inp[indx], &inp[indx + 1],
		    sizeof(db_indx_t) * (NUM_ENT(pagep) - indx));

	return (0);
}

/*
 * __db_ditem --
 *	Remove an item from a page, logging it if enabled.
 *
 * PUBLIC:  int __db_ditem __P((DBC *, PAGE *, u_int32_t, u_int32_t));
 */
int
__db_ditem(dbc, pagep, indx, nbytes)
	DBC *dbc;
	PAGE *pagep;
	u_int32_t indx, nbytes;
{
	DB *dbp;
	DBT ldbt;
	int ret;

	dbp = dbc->dbp;

	if (DBC_LOGGING(dbc)) {
		ldbt.data = P_ENTRY(dbp, pagep, indx);
		ldbt.size = nbytes;
		if ((ret = __db_addrem_log(dbp, dbc->txn,
		    &LSN(pagep), 0, DB_REM_DUP, PGNO(pagep),
		    (u_int32_t)indx, nbytes, &ldbt, NULL, &LSN(pagep))) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(LSN(pagep));

	return (__db_ditem_nolog(dbc, pagep, indx, nbytes));
}

/*
 * __db_pitem_nolog --
 *	Put an item on a page without logging.
 *
 * PUBLIC: int __db_pitem_nolog
 * PUBLIC:     __P((DBC *, PAGE *, u_int32_t, u_int32_t, DBT *, DBT *));
 */
int
__db_pitem_nolog(dbc, pagep, indx, nbytes, hdr, data)
	DBC *dbc;
	PAGE *pagep;
	u_int32_t indx;
	u_int32_t nbytes;
	DBT *hdr, *data;
{
	BKEYDATA bk;
	DB *dbp;
	DBT thdr;
	db_indx_t *inp;
	u_int8_t *p;

	dbp = dbc->dbp;

	DB_ASSERT(dbp->env, IS_DIRTY(pagep));

	if (nbytes > P_FREESPACE(dbp, pagep)) {
		DB_ASSERT(dbp->env, nbytes <= P_FREESPACE(dbp, pagep));
		return (EINVAL);
	}

	if (hdr == NULL) {
		B_TSET(bk.type, B_KEYDATA);
		bk.len = data == NULL ? 0 : data->size;

		thdr.data = &bk;
		thdr.size = SSZA(BKEYDATA, data);
		hdr = &thdr;
	}
	inp = P_INP(dbp, pagep);

	/* Adjust the index table, then put the item on the page. */
	if (indx != NUM_ENT(pagep))
		memmove(&inp[indx + 1], &inp[indx],
		    sizeof(db_indx_t) * (NUM_ENT(pagep) - indx));
	HOFFSET(pagep) -= nbytes;
	inp[indx] = HOFFSET(pagep);
	++NUM_ENT(pagep);

	p = P_ENTRY(dbp, pagep, indx);
	memcpy(p, hdr->data, hdr->size);
	if (data != NULL)
		memcpy(p + hdr->size, data->data, data->size);

	return (0);
}

/*
 * __db_pitem --
 *	Put an item on a page.
 *
 * PUBLIC: int __db_pitem
 * PUBLIC:     __P((DBC *, PAGE *, u_int32_t, u_int32_t, DBT *, DBT *));
 */
int
__db_pitem(dbc, pagep, indx, nbytes, hdr, data)
	DBC *dbc;
	PAGE *pagep;
	u_int32_t indx;
	u_int32_t nbytes;
	DBT *hdr, *data;
{
	DB *dbp;
	int ret;

	dbp = dbc->dbp;
	/*
	 * Put a single item onto a page.  The logic figuring out where to
	 * insert and whether it fits is handled in the caller.  All we do
	 * here is manage the page shuffling.  We cheat a little bit in that
	 * we don't want to copy the dbt on a normal put twice.  If hdr is
	 * NULL, we create a BKEYDATA structure on the page, otherwise, just
	 * copy the caller's information onto the page.
	 *
	 * This routine is also used to put entries onto the page where the
	 * entry is pre-built, e.g., during recovery.  In this case, the hdr
	 * will point to the entry, and the data argument will be NULL.
	 *
	 * !!!
	 * There's a tremendous potential for off-by-one errors here, since
	 * the passed in header sizes must be adjusted for the structure's
	 * placeholder for the trailing variable-length data field.
	 */
	if (DBC_LOGGING(dbc)) {
		if ((ret = __db_addrem_log(dbp, dbc->txn,
		    &LSN(pagep), 0, DB_ADD_DUP, PGNO(pagep),
		    (u_int32_t)indx, nbytes, hdr, data, &LSN(pagep))) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(LSN(pagep));

	return (__db_pitem_nolog(dbc, pagep, indx, nbytes, hdr, data));
}
