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
#include "dbinc/db_swap.h"
#include "dbinc/btree.h"

/*
 * __bam_pgin --
 *	Convert host-specific page layout from the host-independent format
 *	stored on disk.
 *
 * PUBLIC: int __bam_pgin __P((DB *, db_pgno_t, void *, DBT *));
 */
int
__bam_pgin(dbp, pg, pp, cookie)
	DB *dbp;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	DB_PGINFO *pginfo;
	PAGE *h;

	pginfo = (DB_PGINFO *)cookie->data;
	if (!F_ISSET(pginfo, DB_AM_SWAP))
		return (0);

	h = pp;
	return (TYPE(h) == P_BTREEMETA ?  __bam_mswap(dbp->env, pp) :
	    __db_byteswap(dbp, pg, pp, pginfo->db_pagesize, 1));
}

/*
 * __bam_pgout --
 *	Convert host-specific page layout to the host-independent format
 *	stored on disk.
 *
 * PUBLIC: int __bam_pgout __P((DB *, db_pgno_t, void *, DBT *));
 */
int
__bam_pgout(dbp, pg, pp, cookie)
	DB *dbp;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	DB_PGINFO *pginfo;
	PAGE *h;

	pginfo = (DB_PGINFO *)cookie->data;
	if (!F_ISSET(pginfo, DB_AM_SWAP))
		return (0);

	h = pp;
	return (TYPE(h) == P_BTREEMETA ?  __bam_mswap(dbp->env, pp) :
	    __db_byteswap(dbp, pg, pp, pginfo->db_pagesize, 0));
}

/*
 * __bam_mswap --
 *	Swap the bytes on the btree metadata page.
 *
 * PUBLIC: int __bam_mswap __P((ENV *, PAGE *));
 */
int
__bam_mswap(env, pg)
	ENV *env;
	PAGE *pg;
{
	u_int8_t *p;

	COMPQUIET(env, NULL);

	__db_metaswap(pg);
	p = (u_int8_t *)pg + sizeof(DBMETA);

	p += sizeof(u_int32_t);	/* unused */
	SWAP32(p);		/* minkey */
	SWAP32(p);		/* re_len */
	SWAP32(p);		/* re_pad */
	SWAP32(p);		/* root */
	p += 92 * sizeof(u_int32_t); /* unused */
	SWAP32(p);		/* crypto_magic */

	return (0);
}
