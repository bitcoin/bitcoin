/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_upgrade.h"
#include "dbinc/db_page.h"
#include "dbinc/qam.h"

/*
 * __qam_31_qammeta --
 *	Upgrade the database from version 1 to version 2.
 *
 * PUBLIC: int __qam_31_qammeta __P((DB *, char *, u_int8_t *));
 */
int
__qam_31_qammeta(dbp, real_name, buf)
	DB *dbp;
	char *real_name;
	u_int8_t *buf;
{
	QMETA30 *oldmeta;
	QMETA31 *newmeta;

	COMPQUIET(dbp, NULL);
	COMPQUIET(real_name, NULL);

	newmeta = (QMETA31 *)buf;
	oldmeta = (QMETA30 *)buf;

	/*
	 * Copy the fields to their new locations.
	 * They may overlap so start at the bottom and use memmove().
	 */
	newmeta->rec_page = oldmeta->rec_page;
	newmeta->re_pad = oldmeta->re_pad;
	newmeta->re_len = oldmeta->re_len;
	newmeta->cur_recno = oldmeta->cur_recno;
	newmeta->first_recno = oldmeta->first_recno;
	newmeta->start = oldmeta->start;
	memmove(newmeta->dbmeta.uid,
	    oldmeta->dbmeta.uid, sizeof(oldmeta->dbmeta.uid));
	newmeta->dbmeta.flags = oldmeta->dbmeta.flags;
	newmeta->dbmeta.record_count = 0;
	newmeta->dbmeta.key_count = 0;
	ZERO_LSN(newmeta->dbmeta.unused3);

	/* Update the version. */
	newmeta->dbmeta.version = 2;

	return (0);
}

/*
 * __qam_32_qammeta --
 *	Upgrade the database from version 2 to version 3.
 *
 * PUBLIC: int __qam_32_qammeta __P((DB *, char *, u_int8_t *));
 */
int
__qam_32_qammeta(dbp, real_name, buf)
	DB *dbp;
	char *real_name;
	u_int8_t *buf;
{
	QMETA31 *oldmeta;
	QMETA32 *newmeta;

	COMPQUIET(dbp, NULL);
	COMPQUIET(real_name, NULL);

	newmeta = (QMETA32 *)buf;
	oldmeta = (QMETA31 *)buf;

	/*
	 * Copy the fields to their new locations.
	 * We are dropping the first field so move
	 * from the top.
	 */
	newmeta->first_recno = oldmeta->first_recno;
	newmeta->cur_recno = oldmeta->cur_recno;
	newmeta->re_len = oldmeta->re_len;
	newmeta->re_pad = oldmeta->re_pad;
	newmeta->rec_page = oldmeta->rec_page;
	newmeta->page_ext = 0;
	/* cur_recno now points to the first free slot. */
	newmeta->cur_recno++;
	if (newmeta->first_recno == 0)
		newmeta->first_recno = 1;

	/* Update the version. */
	newmeta->dbmeta.version = 3;

	return (0);
}
