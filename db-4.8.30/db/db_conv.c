/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/hmac.h"
#include "dbinc/db_page.h"
#include "dbinc/db_swap.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/log.h"
#include "dbinc/qam.h"

/*
 * __db_pgin --
 *	Primary page-swap routine.
 *
 * PUBLIC: int __db_pgin __P((DB_ENV *, db_pgno_t, void *, DBT *));
 */
int
__db_pgin(dbenv, pg, pp, cookie)
	DB_ENV *dbenv;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	DB dummydb, *dbp;
	DB_CIPHER *db_cipher;
	DB_LSN not_used;
	DB_PGINFO *pginfo;
	ENV *env;
	PAGE *pagep;
	size_t sum_len;
	int is_hmac, ret;
	u_int8_t *chksum;

	pginfo = (DB_PGINFO *)cookie->data;
	env = dbenv->env;
	pagep = (PAGE *)pp;

	ret = is_hmac = 0;
	chksum = NULL;
	memset(&dummydb, 0, sizeof(DB));
	dbp = &dummydb;
	dbp->dbenv = dbenv;
	dbp->env = env;
	dbp->flags = pginfo->flags;
	dbp->pgsize = pginfo->db_pagesize;
	db_cipher = env->crypto_handle;
	switch (pagep->type) {
	case P_HASHMETA:
	case P_BTREEMETA:
	case P_QAMMETA:
		/*
		 * If checksumming is set on the meta-page, we must set
		 * it in the dbp.
		 */
		if (FLD_ISSET(((DBMETA *)pp)->metaflags, DBMETA_CHKSUM))
			F_SET(dbp, DB_AM_CHKSUM);
		else
			F_CLR(dbp, DB_AM_CHKSUM);
		if (((DBMETA *)pp)->encrypt_alg != 0 ||
		    F_ISSET(dbp, DB_AM_ENCRYPT))
			is_hmac = 1;
		/*
		 * !!!
		 * For all meta pages it is required that the chksum
		 * be at the same location.  Use BTMETA to get to it
		 * for any meta type.
		 */
		chksum = ((BTMETA *)pp)->chksum;
		sum_len = DBMETASIZE;
		break;
	case P_INVALID:
		/*
		 * We assume that we've read a file hole if we have
		 * a zero LSN, zero page number and P_INVALID.  Otherwise
		 * we have an invalid page that might contain real data.
		 */
		if (IS_ZERO_LSN(LSN(pagep)) && pagep->pgno == PGNO_INVALID) {
			sum_len = 0;
			break;
		}
		/* FALLTHROUGH */
	default:
		chksum = P_CHKSUM(dbp, pagep);
		sum_len = pginfo->db_pagesize;
		/*
		 * If we are reading in a non-meta page, then if we have
		 * a db_cipher then we are using hmac.
		 */
		is_hmac = CRYPTO_ON(env) ? 1 : 0;
		break;
	}

	/*
	 * We expect a checksum error if there was a configuration problem.
	 * If there is no configuration problem and we don't get a match,
	 * it's fatal: panic the system.
	 */
	if (F_ISSET(dbp, DB_AM_CHKSUM) && sum_len != 0) {
		if (F_ISSET(dbp, DB_AM_SWAP) && is_hmac == 0)
			P_32_SWAP(chksum);
		switch (ret = __db_check_chksum(
		    env, NULL, db_cipher, chksum, pp, sum_len, is_hmac)) {
		case 0:
			break;
		case -1:
			if (DBENV_LOGGING(env))
				(void)__db_cksum_log(
				    env, NULL, &not_used, DB_FLUSH);
			__db_errx(env,
	    "checksum error: page %lu: catastrophic recovery required",
			    (u_long)pg);
			return (__env_panic(env, DB_RUNRECOVERY));
		default:
			return (ret);
		}
	}
	if ((ret = __db_decrypt_pg(env, dbp, pagep)) != 0)
		return (ret);
	switch (pagep->type) {
	case P_INVALID:
		if (pginfo->type == DB_QUEUE)
			return (__qam_pgin_out(env, pg, pp, cookie));
		else
			return (__ham_pgin(dbp, pg, pp, cookie));
	case P_HASH_UNSORTED:
	case P_HASH:
	case P_HASHMETA:
		return (__ham_pgin(dbp, pg, pp, cookie));
	case P_BTREEMETA:
	case P_IBTREE:
	case P_IRECNO:
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
	case P_OVERFLOW:
		return (__bam_pgin(dbp, pg, pp, cookie));
	case P_QAMMETA:
	case P_QAMDATA:
		return (__qam_pgin_out(env, pg, pp, cookie));
	default:
		break;
	}
	return (__db_pgfmt(env, pg));
}

/*
 * __db_pgout --
 *	Primary page-swap routine.
 *
 * PUBLIC: int __db_pgout __P((DB_ENV *, db_pgno_t, void *, DBT *));
 */
int
__db_pgout(dbenv, pg, pp, cookie)
	DB_ENV *dbenv;
	db_pgno_t pg;
	void *pp;
	DBT *cookie;
{
	DB dummydb, *dbp;
	DB_PGINFO *pginfo;
	ENV *env;
	PAGE *pagep;
	int ret;

	pginfo = (DB_PGINFO *)cookie->data;
	env = dbenv->env;
	pagep = (PAGE *)pp;

	memset(&dummydb, 0, sizeof(DB));
	dbp = &dummydb;
	dbp->dbenv = dbenv;
	dbp->env = env;
	dbp->flags = pginfo->flags;
	dbp->pgsize = pginfo->db_pagesize;
	ret = 0;
	switch (pagep->type) {
	case P_INVALID:
		if (pginfo->type == DB_QUEUE)
			ret = __qam_pgin_out(env, pg, pp, cookie);
		else
			ret = __ham_pgout(dbp, pg, pp, cookie);
		break;
	case P_HASH:
	case P_HASH_UNSORTED:
		/*
		 * Support pgout of unsorted hash pages - since online
		 * replication upgrade can cause pages of this type to be
		 * written out.
		 *
		 * FALLTHROUGH
		 */
	case P_HASHMETA:
		ret = __ham_pgout(dbp, pg, pp, cookie);
		break;
	case P_BTREEMETA:
	case P_IBTREE:
	case P_IRECNO:
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
	case P_OVERFLOW:
		ret = __bam_pgout(dbp, pg, pp, cookie);
		break;
	case P_QAMMETA:
	case P_QAMDATA:
		ret = __qam_pgin_out(env, pg, pp, cookie);
		break;
	default:
		return (__db_pgfmt(env, pg));
	}
	if (ret)
		return (ret);

	return (__db_encrypt_and_checksum_pg(env, dbp, pagep));
}

/*
 * __db_decrypt_pg --
 *      Utility function to decrypt a db page.
 *
 * PUBLIC: int __db_decrypt_pg __P((ENV *, DB *, PAGE *));
 */
int
__db_decrypt_pg (env, dbp, pagep)
	ENV *env;
	DB *dbp;
	PAGE *pagep;
{
	DB_CIPHER *db_cipher;
	size_t pg_len, pg_off;
	u_int8_t *iv;
	int ret;

	db_cipher = env->crypto_handle;
	ret = 0;
	iv = NULL;
	if (F_ISSET(dbp, DB_AM_ENCRYPT)) {
		DB_ASSERT(env, db_cipher != NULL);
		DB_ASSERT(env, F_ISSET(dbp, DB_AM_CHKSUM));

		pg_off = P_OVERHEAD(dbp);
		DB_ASSERT(env, db_cipher->adj_size(pg_off) == 0);

		switch (pagep->type) {
		case P_HASHMETA:
		case P_BTREEMETA:
		case P_QAMMETA:
			/*
			 * !!!
			 * For all meta pages it is required that the iv
			 * be at the same location.  Use BTMETA to get to it
			 * for any meta type.
			 */
			iv = ((BTMETA *)pagep)->iv;
			pg_len = DBMETASIZE;
			break;
		case P_INVALID:
			if (IS_ZERO_LSN(LSN(pagep)) &&
			    pagep->pgno == PGNO_INVALID) {
				pg_len = 0;
				break;
			}
			/* FALLTHROUGH */
		default:
			iv = P_IV(dbp, pagep);
			pg_len = dbp->pgsize;
			break;
		}
		if (pg_len != 0)
			ret = db_cipher->decrypt(env, db_cipher->data,
			    iv, ((u_int8_t *)pagep) + pg_off,
			    pg_len - pg_off);
	}
	return (ret);
}

/*
 * __db_encrypt_and_checksum_pg --
 *	Utility function to encrypt and checksum a db page.
 *
 * PUBLIC: int __db_encrypt_and_checksum_pg
 * PUBLIC:     __P((ENV *, DB *, PAGE *));
 */
int
__db_encrypt_and_checksum_pg (env, dbp, pagep)
	ENV *env;
	DB *dbp;
	PAGE *pagep;
{
	DB_CIPHER *db_cipher;
	int ret;
	size_t pg_off, pg_len, sum_len;
	u_int8_t *chksum, *iv, *key;

	chksum = iv = key = NULL;
	db_cipher = env->crypto_handle;

	if (F_ISSET(dbp, DB_AM_ENCRYPT)) {
		DB_ASSERT(env, db_cipher != NULL);
		DB_ASSERT(env, F_ISSET(dbp, DB_AM_CHKSUM));

		pg_off = P_OVERHEAD(dbp);
		DB_ASSERT(env, db_cipher->adj_size(pg_off) == 0);

		key = db_cipher->mac_key;

		switch (pagep->type) {
		case P_HASHMETA:
		case P_BTREEMETA:
		case P_QAMMETA:
			/*
			 * !!!
			 * For all meta pages it is required that the iv
			 * be at the same location.  Use BTMETA to get to it
			 * for any meta type.
			 */
			iv = ((BTMETA *)pagep)->iv;
			pg_len = DBMETASIZE;
			break;
		default:
			iv = P_IV(dbp, pagep);
			pg_len = dbp->pgsize;
			break;
		}
		if ((ret = db_cipher->encrypt(env, db_cipher->data,
		    iv, ((u_int8_t *)pagep) + pg_off, pg_len - pg_off)) != 0)
			return (ret);
	}
	if (F_ISSET(dbp, DB_AM_CHKSUM)) {
		switch (pagep->type) {
		case P_HASHMETA:
		case P_BTREEMETA:
		case P_QAMMETA:
			/*
			 * !!!
			 * For all meta pages it is required that the chksum
			 * be at the same location.  Use BTMETA to get to it
			 * for any meta type.
			 */
			chksum = ((BTMETA *)pagep)->chksum;
			sum_len = DBMETASIZE;
			break;
		default:
			chksum = P_CHKSUM(dbp, pagep);
			sum_len = dbp->pgsize;
			break;
		}
		__db_chksum(NULL, (u_int8_t *)pagep, sum_len, key, chksum);
		if (F_ISSET(dbp, DB_AM_SWAP) && !F_ISSET(dbp, DB_AM_ENCRYPT))
			 P_32_SWAP(chksum);
	}
	return (0);
}

/*
 * __db_metaswap --
 *	Byteswap the common part of the meta-data page.
 *
 * PUBLIC: void __db_metaswap __P((PAGE *));
 */
void
__db_metaswap(pg)
	PAGE *pg;
{
	u_int8_t *p;

	p = (u_int8_t *)pg;

	/* Swap the meta-data information. */
	SWAP32(p);	/* lsn.file */
	SWAP32(p);	/* lsn.offset */
	SWAP32(p);	/* pgno */
	SWAP32(p);	/* magic */
	SWAP32(p);	/* version */
	SWAP32(p);	/* pagesize */
	p += 4;		/* unused, page type, unused, unused */
	SWAP32(p);	/* free */
	SWAP32(p);	/* alloc_lsn part 1 */
	SWAP32(p);	/* alloc_lsn part 2 */
	SWAP32(p);	/* cached key count */
	SWAP32(p);	/* cached record count */
	SWAP32(p);	/* flags */
}

/*
 * __db_byteswap --
 *	Byteswap an ordinary database page.
 *
 * PUBLIC: int __db_byteswap
 * PUBLIC:         __P((DB *, db_pgno_t, PAGE *, size_t, int));
 */
int
__db_byteswap(dbp, pg, h, pagesize, pgin)
	DB *dbp;
	db_pgno_t pg;
	PAGE *h;
	size_t pagesize;
	int pgin;
{
	ENV *env;
	BINTERNAL *bi;
	BKEYDATA *bk;
	BOVERFLOW *bo;
	RINTERNAL *ri;
	db_indx_t i, *inp, len, tmp;
	u_int8_t *end, *p, *pgend;

	if (pagesize == 0)
		return (0);

	env = dbp->env;

	if (pgin) {
		M_32_SWAP(h->lsn.file);
		M_32_SWAP(h->lsn.offset);
		M_32_SWAP(h->pgno);
		M_32_SWAP(h->prev_pgno);
		M_32_SWAP(h->next_pgno);
		M_16_SWAP(h->entries);
		M_16_SWAP(h->hf_offset);
	}

	pgend = (u_int8_t *)h + pagesize;

	inp = P_INP(dbp, h);
	if ((u_int8_t *)inp >= pgend)
		goto out;

	switch (TYPE(h)) {
	case P_HASH_UNSORTED:
	case P_HASH:
		for (i = 0; i < NUM_ENT(h); i++) {
			if (pgin)
				M_16_SWAP(inp[i]);

			if (P_ENTRY(dbp, h, i) >= pgend)
				continue;

			switch (HPAGE_TYPE(dbp, h, i)) {
			case H_KEYDATA:
				break;
			case H_DUPLICATE:
				len = LEN_HKEYDATA(dbp, h, pagesize, i);
				p = HKEYDATA_DATA(P_ENTRY(dbp, h, i));
				for (end = p + len; p < end;) {
					if (pgin) {
						P_16_SWAP(p);
						memcpy(&tmp,
						    p, sizeof(db_indx_t));
						p += sizeof(db_indx_t);
					} else {
						memcpy(&tmp,
						    p, sizeof(db_indx_t));
						SWAP16(p);
					}
					p += tmp;
					SWAP16(p);
				}
				break;
			case H_OFFDUP:
				p = HOFFPAGE_PGNO(P_ENTRY(dbp, h, i));
				SWAP32(p);			/* pgno */
				break;
			case H_OFFPAGE:
				p = HOFFPAGE_PGNO(P_ENTRY(dbp, h, i));
				SWAP32(p);			/* pgno */
				SWAP32(p);			/* tlen */
				break;
			default:
				return (__db_pgfmt(env, pg));
			}

		}

		/*
		 * The offsets in the inp array are used to determine
		 * the size of entries on a page; therefore they
		 * cannot be converted until we've done all the
		 * entries.
		 */
		if (!pgin)
			for (i = 0; i < NUM_ENT(h); i++)
				M_16_SWAP(inp[i]);
		break;
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		for (i = 0; i < NUM_ENT(h); i++) {
			if (pgin)
				M_16_SWAP(inp[i]);

			/*
			 * In the case of on-page duplicates, key information
			 * should only be swapped once.
			 */
			if (h->type == P_LBTREE && i > 1) {
				if (pgin) {
					if (inp[i] == inp[i - 2])
						continue;
				} else {
					M_16_SWAP(inp[i]);
					if (inp[i] == inp[i - 2])
						continue;
					M_16_SWAP(inp[i]);
				}
			}

			bk = GET_BKEYDATA(dbp, h, i);
			if ((u_int8_t *)bk >= pgend)
				continue;
			switch (B_TYPE(bk->type)) {
			case B_KEYDATA:
				M_16_SWAP(bk->len);
				break;
			case B_DUPLICATE:
			case B_OVERFLOW:
				bo = (BOVERFLOW *)bk;
				M_32_SWAP(bo->pgno);
				M_32_SWAP(bo->tlen);
				break;
			default:
				return (__db_pgfmt(env, pg));
			}

			if (!pgin)
				M_16_SWAP(inp[i]);
		}
		break;
	case P_IBTREE:
		for (i = 0; i < NUM_ENT(h); i++) {
			if (pgin)
				M_16_SWAP(inp[i]);

			bi = GET_BINTERNAL(dbp, h, i);
			if ((u_int8_t *)bi >= pgend)
				continue;

			M_16_SWAP(bi->len);
			M_32_SWAP(bi->pgno);
			M_32_SWAP(bi->nrecs);

			switch (B_TYPE(bi->type)) {
			case B_KEYDATA:
				break;
			case B_DUPLICATE:
			case B_OVERFLOW:
				bo = (BOVERFLOW *)bi->data;
				M_32_SWAP(bo->pgno);
				M_32_SWAP(bo->tlen);
				break;
			default:
				return (__db_pgfmt(env, pg));
			}

			if (!pgin)
				M_16_SWAP(inp[i]);
		}
		break;
	case P_IRECNO:
		for (i = 0; i < NUM_ENT(h); i++) {
			if (pgin)
				M_16_SWAP(inp[i]);

			ri = GET_RINTERNAL(dbp, h, i);
			if ((u_int8_t *)ri >= pgend)
				continue;

			M_32_SWAP(ri->pgno);
			M_32_SWAP(ri->nrecs);

			if (!pgin)
				M_16_SWAP(inp[i]);
		}
		break;
	case P_INVALID:
	case P_OVERFLOW:
	case P_QAMDATA:
		/* Nothing to do. */
		break;
	default:
		return (__db_pgfmt(env, pg));
	}

out:	if (!pgin) {
		/* Swap the header information. */
		M_32_SWAP(h->lsn.file);
		M_32_SWAP(h->lsn.offset);
		M_32_SWAP(h->pgno);
		M_32_SWAP(h->prev_pgno);
		M_32_SWAP(h->next_pgno);
		M_16_SWAP(h->entries);
		M_16_SWAP(h->hf_offset);
	}
	return (0);
}

/*
 * __db_pageswap --
 *	Byteswap any database page.  Normally, the page to be swapped will be
 *	referenced by the "pp" argument and the pdata argument will be NULL.
 *	This function is also called by automatically generated log functions,
 *	where the page may be split into separate header and data parts.  In
 *	that case, pdata is not NULL we reconsitute
 *
 * PUBLIC: int __db_pageswap
 * PUBLIC:         __P((DB *, void *, size_t, DBT *, int));
 */
int
__db_pageswap(dbp, pp, len, pdata, pgin)
	DB *dbp;
	void *pp;
	size_t len;
	DBT *pdata;
	int pgin;
{
	ENV *env;
	db_pgno_t pg;
	size_t pgsize;
	void *pgcopy;
	int ret;
	u_int16_t hoffset;

	env = dbp->env;

	switch (TYPE(pp)) {
	case P_BTREEMETA:
		return (__bam_mswap(env, pp));

	case P_HASHMETA:
		return (__ham_mswap(env, pp));

	case P_QAMMETA:
		return (__qam_mswap(env, pp));

	case P_INVALID:
	case P_OVERFLOW:
	case P_QAMDATA:
		/*
		 * We may have been passed an invalid page, or a queue data
		 * page, or an overflow page where fields like hoffset have a
		 * special meaning.  In that case, no swapping of the page data
		 * is required, just the fields in the page header.
		 */
		pdata = NULL;
		break;

	default:
		break;
	}

	if (pgin) {
		P_32_COPYSWAP(&PGNO(pp), &pg);
		P_16_COPYSWAP(&HOFFSET(pp), &hoffset);
	} else {
		pg = PGNO(pp);
		hoffset = HOFFSET(pp);
	}

	if (pdata == NULL)
		ret = __db_byteswap(dbp, pg, (PAGE *)pp, len, pgin);
	else {
		pgsize = hoffset + pdata->size;
		if ((ret = __os_malloc(env, pgsize, &pgcopy)) != 0)
			return (ret);
		memset(pgcopy, 0, pgsize);
		memcpy(pgcopy, pp, len);
		memcpy((u_int8_t *)pgcopy + hoffset, pdata->data, pdata->size);

		ret = __db_byteswap(dbp, pg, (PAGE *)pgcopy, pgsize, pgin);
		memcpy(pp, pgcopy, len);

		/*
		 * If we are swapping data to be written to the log, we can't
		 * overwrite the buffer that was passed in: it may be a pointer
		 * into a page in cache.  We set DB_DBT_APPMALLOC here so that
		 * the calling code can free the memory we allocate here.
		 */
		if (!pgin) {
			if ((ret =
			    __os_malloc(env, pdata->size, &pdata->data)) != 0) {
				__os_free(env, pgcopy);
				return (ret);
			}
			F_SET(pdata, DB_DBT_APPMALLOC);
		}
		memcpy(pdata->data, (u_int8_t *)pgcopy + hoffset, pdata->size);
		__os_free(env, pgcopy);
	}

	return (ret);
}
