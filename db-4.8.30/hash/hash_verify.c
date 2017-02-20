/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_verify.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"

static int __ham_dups_unsorted __P((DB *, u_int8_t *, u_int32_t));
static int __ham_vrfy_bucket __P((DB *, VRFY_DBINFO *, HMETA *, u_int32_t,
    u_int32_t));
static int __ham_vrfy_item __P((DB *,
    VRFY_DBINFO *, db_pgno_t, PAGE *, u_int32_t, u_int32_t));

/*
 * __ham_vrfy_meta --
 *	Verify the hash-specific part of a metadata page.
 *
 *	Note that unlike btree, we don't save things off, because we
 *	will need most everything again to verify each page and the
 *	amount of state here is significant.
 *
 * PUBLIC: int __ham_vrfy_meta __P((DB *, VRFY_DBINFO *, HMETA *,
 * PUBLIC:     db_pgno_t, u_int32_t));
 */
int
__ham_vrfy_meta(dbp, vdp, m, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	HMETA *m;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	HASH *hashp;
	VRFY_PAGEINFO *pip;
	int i, ret, t_ret, isbad;
	u_int32_t pwr, mbucket;
	u_int32_t (*hfunc) __P((DB *, const void *, u_int32_t));

	env = dbp->env;
	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	hashp = dbp->h_internal;

	if (hashp != NULL && hashp->h_hash != NULL)
		hfunc = hashp->h_hash;
	else
		hfunc = __ham_func5;

	/*
	 * If we haven't already checked the common fields in pagezero,
	 * check them.
	 */
	if (!F_ISSET(pip, VRFY_INCOMPLETE) &&
	    (ret = __db_vrfy_meta(dbp, vdp, &m->dbmeta, pgno, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	/* h_charkey */
	if (!LF_ISSET(DB_NOORDERCHK))
		if (m->h_charkey != hfunc(dbp, CHARKEY, sizeof(CHARKEY))) {
			EPRINT((env,
"Page %lu: database has custom hash function; reverify with DB_NOORDERCHK set",
			    (u_long)pgno));
			/*
			 * Return immediately;  this is probably a sign of user
			 * error rather than database corruption, so we want to
			 * avoid extraneous errors.
			 */
			isbad = 1;
			goto err;
		}

	/* max_bucket must be less than the last pgno. */
	if (m->max_bucket > vdp->last_pgno) {
		EPRINT((env,
		    "Page %lu: Impossible max_bucket %lu on meta page",
		    (u_long)pgno, (u_long)m->max_bucket));
		/*
		 * Most other fields depend somehow on max_bucket, so
		 * we just return--there will be lots of extraneous
		 * errors.
		 */
		isbad = 1;
		goto err;
	}

	/*
	 * max_bucket, high_mask and low_mask: high_mask must be one
	 * less than the next power of two above max_bucket, and
	 * low_mask must be one less than the power of two below it.
	 */
	pwr = (m->max_bucket == 0) ? 1 : 1 << __db_log2(m->max_bucket + 1);
	if (m->high_mask != pwr - 1) {
		EPRINT((env,
		    "Page %lu: incorrect high_mask %lu, should be %lu",
		    (u_long)pgno, (u_long)m->high_mask, (u_long)pwr - 1));
		isbad = 1;
	}
	pwr >>= 1;
	if (m->low_mask != pwr - 1) {
		EPRINT((env,
		    "Page %lu: incorrect low_mask %lu, should be %lu",
		    (u_long)pgno, (u_long)m->low_mask, (u_long)pwr - 1));
		isbad = 1;
	}

	/* ffactor: no check possible. */
	pip->h_ffactor = m->ffactor;

	/*
	 * nelem: just make sure it's not astronomical for now. This is the
	 * same check that hash_upgrade does, since there was a bug in 2.X
	 * which could make nelem go "negative".
	 */
	if (m->nelem > 0x80000000) {
		EPRINT((env,
		    "Page %lu: suspiciously high nelem of %lu",
		    (u_long)pgno, (u_long)m->nelem));
		isbad = 1;
		pip->h_nelem = 0;
	} else
		pip->h_nelem = m->nelem;

	/* flags */
	if (F_ISSET(&m->dbmeta, DB_HASH_DUP))
		F_SET(pip, VRFY_HAS_DUPS);
	if (F_ISSET(&m->dbmeta, DB_HASH_DUPSORT))
		F_SET(pip, VRFY_HAS_DUPSORT);
	/* XXX: Why is the DB_HASH_SUBDB flag necessary? */

	/* spares array */
	for (i = 0; m->spares[i] != 0 && i < NCACHED; i++) {
		/*
		 * We set mbucket to the maximum bucket that would use a given
		 * spares entry;  we want to ensure that it's always less
		 * than last_pgno.
		 */
		mbucket = (1 << i) - 1;
		if (BS_TO_PAGE(mbucket, m->spares) > vdp->last_pgno) {
			EPRINT((env,
			    "Page %lu: spares array entry %d is invalid",
			    (u_long)pgno, i));
			isbad = 1;
		}
	}

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (LF_ISSET(DB_SALVAGE) &&
	   (t_ret = __db_salvage_markdone(vdp, pgno)) != 0 && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __ham_vrfy --
 *	Verify hash page.
 *
 * PUBLIC: int __ham_vrfy __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t,
 * PUBLIC:     u_int32_t));
 */
int
__ham_vrfy(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	DBC *dbc;
	ENV *env;
	VRFY_PAGEINFO *pip;
	u_int32_t ent, himark, inpend;
	db_indx_t *inp;
	int isbad, ret, t_ret;

	env = dbp->env;
	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	if (TYPE(h) != P_HASH && TYPE(h) != P_HASH_UNSORTED) {
		ret = __db_unknown_path(env, "__ham_vrfy");
		goto err;
	}

	/* Verify and save off fields common to all PAGEs. */
	if ((ret = __db_vrfy_datapage(dbp, vdp, h, pgno, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	/*
	 * Verify inp[].  Each offset from 0 to NUM_ENT(h) must be lower
	 * than the previous one, higher than the current end of the inp array,
	 * and lower than the page size.
	 *
	 * In any case, we return immediately if things are bad, as it would
	 * be unsafe to proceed.
	 */
	inp = P_INP(dbp, h);
	for (ent = 0, himark = dbp->pgsize,
	    inpend = (u_int32_t)((u_int8_t *)inp - (u_int8_t *)h);
	    ent < NUM_ENT(h); ent++)
		if (inp[ent] >= himark) {
			EPRINT((env,
			    "Page %lu: item %lu is out of order or nonsensical",
			    (u_long)pgno, (u_long)ent));
			isbad = 1;
			goto err;
		} else if (inpend >= himark) {
			EPRINT((env,
			    "Page %lu: entries array collided with data",
			    (u_long)pgno));
			isbad = 1;
			goto err;

		} else {
			himark = inp[ent];
			inpend += sizeof(db_indx_t);
			if ((ret = __ham_vrfy_item(
			    dbp, vdp, pgno, h, ent, flags)) != 0)
				goto err;
		}

	if ((ret = __db_cursor_int(dbp, vdp->thread_info, NULL, DB_HASH,
	    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &dbc)) != 0)
		return (ret);
	if (!LF_ISSET(DB_NOORDERCHK) && TYPE(h) == P_HASH &&
	    (ret = __ham_verify_sorted_page(dbc, h)) != 0)
		isbad = 1;

err:	if ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return (ret == 0 && isbad == 1 ? DB_VERIFY_BAD : ret);
}

/*
 * __ham_vrfy_item --
 *	Given a hash page and an offset, sanity-check the item itself,
 *	and save off any overflow items or off-page dup children as necessary.
 */
static int
__ham_vrfy_item(dbp, vdp, pgno, h, i, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	PAGE *h;
	u_int32_t i, flags;
{
	HOFFDUP hod;
	HOFFPAGE hop;
	VRFY_CHILDINFO child;
	VRFY_PAGEINFO *pip;
	db_indx_t offset, len, dlen, elen;
	int ret, t_ret;
	u_int8_t *databuf;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	switch (HPAGE_TYPE(dbp, h, i)) {
	case H_KEYDATA:
		/* Nothing to do here--everything but the type field is data */
		break;
	case H_DUPLICATE:
		/* Are we a datum or a key?  Better be the former. */
		if (i % 2 == 0) {
			EPRINT((dbp->env,
			    "Page %lu: hash key stored as duplicate item %lu",
			    (u_long)pip->pgno, (u_long)i));
		}
		/*
		 * Dups are encoded as a series within a single HKEYDATA,
		 * in which each dup is surrounded by a copy of its length
		 * on either side (so that the series can be walked in either
		 * direction.  We loop through this series and make sure
		 * each dup is reasonable.
		 *
		 * Note that at this point, we've verified item i-1, so
		 * it's safe to use LEN_HKEYDATA (which looks at inp[i-1]).
		 */
		len = LEN_HKEYDATA(dbp, h, dbp->pgsize, i);
		databuf = HKEYDATA_DATA(P_ENTRY(dbp, h, i));
		for (offset = 0; offset < len; offset += DUP_SIZE(dlen)) {
			memcpy(&dlen, databuf + offset, sizeof(db_indx_t));

			/* Make sure the length is plausible. */
			if (offset + DUP_SIZE(dlen) > len) {
				EPRINT((dbp->env,
			    "Page %lu: duplicate item %lu has bad length",
				    (u_long)pip->pgno, (u_long)i));
				ret = DB_VERIFY_BAD;
				goto err;
			}

			/*
			 * Make sure the second copy of the length is the
			 * same as the first.
			 */
			memcpy(&elen,
			    databuf + offset + dlen + sizeof(db_indx_t),
			    sizeof(db_indx_t));
			if (elen != dlen) {
				EPRINT((dbp->env,
		"Page %lu: duplicate item %lu has two different lengths",
				    (u_long)pip->pgno, (u_long)i));
				ret = DB_VERIFY_BAD;
				goto err;
			}
		}
		F_SET(pip, VRFY_HAS_DUPS);
		if (!LF_ISSET(DB_NOORDERCHK) &&
		    __ham_dups_unsorted(dbp, databuf, len))
			F_SET(pip, VRFY_DUPS_UNSORTED);
		break;
	case H_OFFPAGE:
		/* Offpage item.  Make sure pgno is sane, save off. */
		memcpy(&hop, P_ENTRY(dbp, h, i), HOFFPAGE_SIZE);
		if (!IS_VALID_PGNO(hop.pgno) || hop.pgno == pip->pgno ||
		    hop.pgno == PGNO_INVALID) {
			EPRINT((dbp->env,
			    "Page %lu: offpage item %lu has bad pgno %lu",
			    (u_long)pip->pgno, (u_long)i, (u_long)hop.pgno));
			ret = DB_VERIFY_BAD;
			goto err;
		}
		memset(&child, 0, sizeof(VRFY_CHILDINFO));
		child.pgno = hop.pgno;
		child.type = V_OVERFLOW;
		child.tlen = hop.tlen; /* This will get checked later. */
		if ((ret = __db_vrfy_childput(vdp, pip->pgno, &child)) != 0)
			goto err;
		break;
	case H_OFFDUP:
		/* Offpage duplicate item.  Same drill. */
		memcpy(&hod, P_ENTRY(dbp, h, i), HOFFDUP_SIZE);
		if (!IS_VALID_PGNO(hod.pgno) || hod.pgno == pip->pgno ||
		    hod.pgno == PGNO_INVALID) {
			EPRINT((dbp->env,
			    "Page %lu: offpage item %lu has bad page number",
			    (u_long)pip->pgno, (u_long)i));
			ret = DB_VERIFY_BAD;
			goto err;
		}
		memset(&child, 0, sizeof(VRFY_CHILDINFO));
		child.pgno = hod.pgno;
		child.type = V_DUPLICATE;
		if ((ret = __db_vrfy_childput(vdp, pip->pgno, &child)) != 0)
			goto err;
		F_SET(pip, VRFY_HAS_DUPS);
		break;
	default:
		EPRINT((dbp->env,
		    "Page %lu: item %lu has bad type",
		    (u_long)pip->pgno, (u_long)i));
		ret = DB_VERIFY_BAD;
		break;
	}

err:	if ((t_ret =
	    __db_vrfy_putpageinfo(dbp->env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __ham_vrfy_structure --
 *	Verify the structure of a hash database.
 *
 * PUBLIC: int __ham_vrfy_structure __P((DB *, VRFY_DBINFO *, db_pgno_t,
 * PUBLIC:     u_int32_t));
 */
int
__ham_vrfy_structure(dbp, vdp, meta_pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t meta_pgno;
	u_int32_t flags;
{
	DB *pgset;
	DB_MPOOLFILE *mpf;
	HMETA *m;
	PAGE *h;
	VRFY_PAGEINFO *pip;
	int isbad, p, ret, t_ret;
	db_pgno_t pgno;
	u_int32_t bucket, spares_entry;

	mpf = dbp->mpf;
	pgset = vdp->pgset;
	h = NULL;
	ret = isbad = 0;

	if ((ret = __db_vrfy_pgset_get(pgset,
	    vdp->thread_info, meta_pgno, &p)) != 0)
		return (ret);
	if (p != 0) {
		EPRINT((dbp->env,
		    "Page %lu: Hash meta page referenced twice",
		    (u_long)meta_pgno));
		return (DB_VERIFY_BAD);
	}
	if ((ret = __db_vrfy_pgset_inc(pgset,
	    vdp->thread_info, meta_pgno)) != 0)
		return (ret);

	/* Get the meta page;  we'll need it frequently. */
	if ((ret = __memp_fget(mpf,
	    &meta_pgno, vdp->thread_info, NULL, 0, &m)) != 0)
		return (ret);

	/* Loop through bucket by bucket. */
	for (bucket = 0; bucket <= m->max_bucket; bucket++)
		if ((ret =
		    __ham_vrfy_bucket(dbp, vdp, m, bucket, flags)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto err;
		    }

	/*
	 * There may be unused hash pages corresponding to buckets
	 * that have been allocated but not yet used.  These may be
	 * part of the current doubling above max_bucket, or they may
	 * correspond to buckets that were used in a transaction
	 * that then aborted.
	 *
	 * Loop through them, as far as the spares array defines them,
	 * and make sure they're all empty.
	 *
	 * Note that this should be safe, since we've already verified
	 * that the spares array is sane.
	 */
	for (bucket = m->max_bucket + 1; spares_entry = __db_log2(bucket + 1),
	    spares_entry < NCACHED && m->spares[spares_entry] != 0; bucket++) {
		pgno = BS_TO_PAGE(bucket, m->spares);
		if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
			goto err;

		/* It's okay if these pages are totally zeroed;  unmark it. */
		F_CLR(pip, VRFY_IS_ALLZEROES);

		/* It's also OK if this page is simply invalid. */
		if (pip->type == P_INVALID) {
			if ((ret = __db_vrfy_putpageinfo(dbp->env,
			    vdp, pip)) != 0)
				goto err;
			continue;
		}

		if (pip->type != P_HASH && pip->type != P_HASH_UNSORTED) {
			EPRINT((dbp->env,
			    "Page %lu: hash bucket %lu maps to non-hash page",
			    (u_long)pgno, (u_long)bucket));
			isbad = 1;
		} else if (pip->entries != 0) {
			EPRINT((dbp->env,
		    "Page %lu: non-empty page in unused hash bucket %lu",
			    (u_long)pgno, (u_long)bucket));
			isbad = 1;
		} else {
			if ((ret = __db_vrfy_pgset_get(pgset,
			    vdp->thread_info, pgno, &p)) != 0)
				goto err;
			if (p != 0) {
				EPRINT((dbp->env,
				    "Page %lu: above max_bucket referenced",
				    (u_long)pgno));
				isbad = 1;
			} else {
				if ((ret = __db_vrfy_pgset_inc(pgset,
				    vdp->thread_info, pgno)) != 0)
					goto err;
				if ((ret = __db_vrfy_putpageinfo(dbp->env,
				    vdp, pip)) != 0)
					goto err;
				continue;
			}
		}

		/* If we got here, it's an error. */
		(void)__db_vrfy_putpageinfo(dbp->env, vdp, pip);
		goto err;
	}

err:	if ((t_ret = __memp_fput(mpf, vdp->thread_info, m, dbp->priority)) != 0)
		return (t_ret);
	if (h != NULL &&
	    (t_ret = __memp_fput(mpf, vdp->thread_info, h, dbp->priority)) != 0)
		return (t_ret);
	return ((isbad == 1 && ret == 0) ? DB_VERIFY_BAD: ret);
}

/*
 * __ham_vrfy_bucket --
 *	Verify a given bucket.
 */
static int
__ham_vrfy_bucket(dbp, vdp, m, bucket, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	HMETA *m;
	u_int32_t bucket, flags;
{
	ENV *env;
	HASH *hashp;
	VRFY_CHILDINFO *child;
	VRFY_PAGEINFO *mip, *pip;
	int ret, t_ret, isbad, p;
	db_pgno_t pgno, next_pgno;
	DBC *cc;
	u_int32_t (*hfunc) __P((DB *, const void *, u_int32_t));

	env = dbp->env;
	isbad = 0;
	pip = NULL;
	cc = NULL;

	hashp = dbp->h_internal;
	if (hashp != NULL && hashp->h_hash != NULL)
		hfunc = hashp->h_hash;
	else
		hfunc = __ham_func5;

	if ((ret = __db_vrfy_getpageinfo(vdp, PGNO(m), &mip)) != 0)
		return (ret);

	/* Calculate the first pgno for this bucket. */
	pgno = BS_TO_PAGE(bucket, m->spares);

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		goto err;

	/* Make sure we got a plausible page number. */
	if (pgno > vdp->last_pgno ||
	    (pip->type != P_HASH && pip->type != P_HASH_UNSORTED)) {
		EPRINT((env,
		    "Page %lu: impossible first page in bucket %lu",
		    (u_long)pgno, (u_long)bucket));
		/* Unsafe to continue. */
		isbad = 1;
		goto err;
	}

	if (pip->prev_pgno != PGNO_INVALID) {
		EPRINT((env,
		    "Page %lu: first page in hash bucket %lu has a prev_pgno",
		    (u_long)pgno, (u_long)bucket));
		isbad = 1;
	}

	/*
	 * Set flags for dups and sorted dups.
	 */
	flags |= F_ISSET(mip, VRFY_HAS_DUPS) ? DB_ST_DUPOK : 0;
	flags |= F_ISSET(mip, VRFY_HAS_DUPSORT) ? DB_ST_DUPSORT : 0;

	/* Loop until we find a fatal bug, or until we run out of pages. */
	for (;;) {
		/* Provide feedback on our progress to the application. */
		if (!LF_ISSET(DB_SALVAGE))
			__db_vrfy_struct_feedback(dbp, vdp);

		if ((ret = __db_vrfy_pgset_get(vdp->pgset,
		    vdp->thread_info, pgno, &p)) != 0)
			goto err;
		if (p != 0) {
			EPRINT((env,
			    "Page %lu: hash page referenced twice",
			    (u_long)pgno));
			isbad = 1;
			/* Unsafe to continue. */
			goto err;
		} else if ((ret = __db_vrfy_pgset_inc(vdp->pgset,
		    vdp->thread_info, pgno)) != 0)
			goto err;

		/*
		 * Hash pages that nothing has ever hashed to may never
		 * have actually come into existence, and may appear to be
		 * entirely zeroed.  This is acceptable, and since there's
		 * no real way for us to know whether this has actually
		 * occurred, we clear the "wholly zeroed" flag on every
		 * hash page.  A wholly zeroed page, by nature, will appear
		 * to have no flags set and zero entries, so should
		 * otherwise verify correctly.
		 */
		F_CLR(pip, VRFY_IS_ALLZEROES);

		/* If we have dups, our meta page had better know about it. */
		if (F_ISSET(pip, VRFY_HAS_DUPS) &&
		    !F_ISSET(mip, VRFY_HAS_DUPS)) {
			EPRINT((env,
		    "Page %lu: duplicates present in non-duplicate database",
			    (u_long)pgno));
			isbad = 1;
		}

		/*
		 * If the database has sorted dups, this page had better
		 * not have unsorted ones.
		 */
		if (F_ISSET(mip, VRFY_HAS_DUPSORT) &&
		    F_ISSET(pip, VRFY_DUPS_UNSORTED)) {
			EPRINT((env,
			    "Page %lu: unsorted dups in sorted-dup database",
			    (u_long)pgno));
			isbad = 1;
		}

		/* Walk overflow chains and offpage dup trees. */
		if ((ret = __db_vrfy_childcursor(vdp, &cc)) != 0)
			goto err;
		for (ret = __db_vrfy_ccset(cc, pip->pgno, &child); ret == 0;
		    ret = __db_vrfy_ccnext(cc, &child))
			if (child->type == V_OVERFLOW) {
				if ((ret = __db_vrfy_ovfl_structure(dbp, vdp,
				    child->pgno, child->tlen,
				    flags | DB_ST_OVFL_LEAF)) != 0) {
					if (ret == DB_VERIFY_BAD)
						isbad = 1;
					else
						goto err;
				}
			} else if (child->type == V_DUPLICATE) {
				if ((ret = __db_vrfy_duptype(dbp,
				    vdp, child->pgno, flags)) != 0) {
					isbad = 1;
					continue;
				}
				if ((ret = __bam_vrfy_subtree(dbp, vdp,
				    child->pgno, NULL, NULL,
		    flags | DB_ST_RECNUM | DB_ST_DUPSET | DB_ST_TOPLEVEL,
				    NULL, NULL, NULL)) != 0) {
					if (ret == DB_VERIFY_BAD)
						isbad = 1;
					else
						goto err;
				}
			}
		/* Close the cursor on vdp, open one on dbp */
		if ((ret = __db_vrfy_ccclose(cc)) != 0)
			goto err;
		if ((ret = __db_cursor_int(dbp, vdp->thread_info, NULL,
		    DB_HASH, PGNO_INVALID, 0, DB_LOCK_INVALIDID, &cc)) != 0)
			goto err;
		/* If it's safe to check that things hash properly, do so. */
		if (isbad == 0 && !LF_ISSET(DB_NOORDERCHK) &&
		    (ret = __ham_vrfy_hashing(cc, pip->entries,
		    m, bucket, pgno, flags, hfunc)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto err;
		}

		next_pgno = pip->next_pgno;
		ret = __db_vrfy_putpageinfo(env, vdp, pip);

		pip = NULL;
		if (ret != 0)
			goto err;

		if (next_pgno == PGNO_INVALID)
			break;		/* End of the bucket. */

		/* We already checked this, but just in case... */
		if (!IS_VALID_PGNO(next_pgno)) {
			EPRINT((env,
			    "Page %lu: hash page has bad next_pgno",
			    (u_long)pgno));
			isbad = 1;
			goto err;
		}

		if ((ret = __db_vrfy_getpageinfo(vdp, next_pgno, &pip)) != 0)
			goto err;

		if (pip->prev_pgno != pgno) {
			EPRINT((env,
			    "Page %lu: hash page has bad prev_pgno",
			    (u_long)next_pgno));
			isbad = 1;
		}
		pgno = next_pgno;
	}

err:	if (cc != NULL && ((t_ret = __db_vrfy_ccclose(cc)) != 0) && ret == 0)
		ret = t_ret;
	if (mip != NULL && ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, mip)) != 0) && ret == 0)
		ret = t_ret;
	if (pip != NULL && ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, pip)) != 0) && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __ham_vrfy_hashing --
 *	Verify that all items on a given hash page hash correctly.
 *
 * PUBLIC: int __ham_vrfy_hashing __P((DBC *,
 * PUBLIC:     u_int32_t, HMETA *, u_int32_t, db_pgno_t, u_int32_t,
 * PUBLIC:     u_int32_t (*) __P((DB *, const void *, u_int32_t))));
 */
int
__ham_vrfy_hashing(dbc, nentries, m, thisbucket, pgno, flags, hfunc)
	DBC *dbc;
	u_int32_t nentries;
	HMETA *m;
	u_int32_t thisbucket;
	db_pgno_t pgno;
	u_int32_t flags;
	u_int32_t (*hfunc) __P((DB *, const void *, u_int32_t));
{
	DB *dbp;
	DBT dbt;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	PAGE *h;
	db_indx_t i;
	int ret, t_ret, isbad;
	u_int32_t hval, bucket;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	ret = isbad = 0;

	memset(&dbt, 0, sizeof(DBT));
	F_SET(&dbt, DB_DBT_REALLOC);
	ENV_GET_THREAD_INFO(dbp->env, ip);

	if ((ret = __memp_fget(mpf, &pgno, ip, NULL, 0, &h)) != 0)
		return (ret);

	for (i = 0; i < nentries; i += 2) {
		/*
		 * We've already verified the page integrity and that of any
		 * overflow chains linked off it;  it is therefore safe to use
		 * __db_ret.  It's also not all that much slower, since we have
		 * to copy every hash item to deal with alignment anyway;  we
		 * can tweak this a bit if this proves to be a bottleneck,
		 * but for now, take the easy route.
		 */
		if ((ret = __db_ret(dbc, h, i, &dbt, NULL, NULL)) != 0)
			goto err;
		hval = hfunc(dbp, dbt.data, dbt.size);

		bucket = hval & m->high_mask;
		if (bucket > m->max_bucket)
			bucket = bucket & m->low_mask;

		if (bucket != thisbucket) {
			EPRINT((dbp->env,
			    "Page %lu: item %lu hashes incorrectly",
			    (u_long)pgno, (u_long)i));
			isbad = 1;
		}
	}

err:	if (dbt.data != NULL)
		__os_ufree(dbp->env, dbt.data);
	if ((t_ret = __memp_fput(mpf, ip, h, dbp->priority)) != 0)
		return (t_ret);

	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __ham_salvage --
 *	Safely dump out anything that looks like a key on an alleged
 *	hash page.
 *
 * PUBLIC: int __ham_salvage __P((DB *, VRFY_DBINFO *, db_pgno_t, PAGE *,
 * PUBLIC:     void *, int (*)(void *, const void *), u_int32_t));
 */
int
__ham_salvage(dbp, vdp, pgno, h, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	PAGE *h;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	DBT dbt, key_dbt, unkdbt;
	db_pgno_t dpgno;
	int ret, err_ret, t_ret;
	u_int32_t himark, i, ovfl_bufsz;
	u_int8_t *hk, *p;
	void *buf, *key_buf;
	db_indx_t dlen, len, tlen;

	memset(&dbt, 0, sizeof(DBT));
	dbt.flags = DB_DBT_REALLOC;

	DB_INIT_DBT(unkdbt, "UNKNOWN", sizeof("UNKNOWN") - 1);

	err_ret = 0;

	/*
	 * Allocate a buffer for overflow items.  Start at one page;
	 * __db_safe_goff will realloc as needed.
	 */
	if ((ret = __os_malloc(dbp->env, dbp->pgsize, &buf)) != 0)
		return (ret);
    ovfl_bufsz = dbp->pgsize;

	himark = dbp->pgsize;
	for (i = 0;; i++) {
		/* If we're not aggressive, break when we hit NUM_ENT(h). */
		if (!LF_ISSET(DB_AGGRESSIVE) && i >= NUM_ENT(h))
			break;

		/* 
		 * Verify the current item. If we're beyond NUM_ENT errors are
		 * expected and ignored.
		 */
		ret = __db_vrfy_inpitem(dbp,
		    h, pgno, i, 0, flags, &himark, NULL);
		/* If this returned a fatality, it's time to break. */
		if (ret == DB_VERIFY_FATAL) {
			if (i >= NUM_ENT(h))
				ret = 0;
			break;
		} else if (ret != 0 && i >= NUM_ENT(h)) {
			/* Not a reportable error, but don't salvage the item. */
			ret = 0;
		} else if (ret == 0) {
			/* Set len to total entry length. */
			len = LEN_HITEM(dbp, h, dbp->pgsize, i);
			hk = P_ENTRY(dbp, h, i);
			if (len == 0 || len > dbp->pgsize ||
			    (u_int32_t)(hk + len - (u_int8_t *)h) >
			    dbp->pgsize) {
				/* Item is unsafely large; skip it. */
				err_ret = DB_VERIFY_BAD;
				continue;
			}
			switch (HPAGE_PTYPE(hk)) {
			case H_KEYDATA:
				/* Update len to size of item. */
				len = LEN_HKEYDATA(dbp, h, dbp->pgsize, i);
keydata:			memcpy(buf, HKEYDATA_DATA(hk), len);
				dbt.size = len;
				dbt.data = buf;
				if ((ret = __db_vrfy_prdbt(&dbt,
				    0, " ", handle, callback, 0, vdp)) != 0)
					err_ret = ret;
				break;
			case H_OFFPAGE:
				if (len < HOFFPAGE_SIZE) {
					err_ret = DB_VERIFY_BAD;
					continue;
				}
				memcpy(&dpgno,
				    HOFFPAGE_PGNO(hk), sizeof(dpgno));
				if ((ret = __db_safe_goff(dbp, vdp,
				    dpgno, &dbt, &buf, &ovfl_bufsz, flags)) != 0) {
					err_ret = ret;
					(void)__db_vrfy_prdbt(&unkdbt, 0, " ",
					    handle, callback, 0, vdp);
					/* fallthrough to end of case */
				} else if ((ret = __db_vrfy_prdbt(&dbt,
				    0, " ", handle, callback, 0, vdp)) != 0)
					err_ret = ret;
				break;
			case H_OFFDUP:
				if (len < HOFFDUP_SIZE) {
					err_ret = DB_VERIFY_BAD;
					continue;
				}
				memcpy(&dpgno,
				    HOFFDUP_PGNO(hk), sizeof(dpgno));
				/* UNKNOWN iff pgno is bad or we're a key. */
				if (!IS_VALID_PGNO(dpgno) || (i % 2 == 0)) {
					if ((ret =
					    __db_vrfy_prdbt(&unkdbt, 0, " ",
					    handle, callback, 0, vdp)) != 0)
						err_ret = ret;
				} else if ((ret = __db_salvage_duptree(dbp,
				    vdp, dpgno, &dbt, handle, callback,
				    flags | DB_SA_SKIPFIRSTKEY)) != 0)
					err_ret = ret;
				break;
			case H_DUPLICATE:
				/*
				 * This is an on-page duplicate item, iterate
				 * over the duplicate set, printing out
				 * key/data pairs.
				 */
				len = LEN_HKEYDATA(dbp, h, dbp->pgsize, i);
				/*
				 * If this item is at an even index it must be
				 * a key item and it should never be of type
				 * H_DUPLICATE. If we are in aggressive mode,
				 * print the item out as a normal key, and let
				 * the user resolve the discrepancy.
				 */
				if (i % 2 == 0) {
					err_ret = ret;
					if (LF_ISSET(DB_AGGRESSIVE))
						goto keydata;
					break;
				}

				/*
				 * Check to ensure that the item size is
				 * greater than the smallest possible on page
				 * duplicate.
				 */
				if (len <
				    HKEYDATA_SIZE(2 * sizeof(db_indx_t))) {
					err_ret = DB_VERIFY_BAD;
					continue;
				}

				/*
				 * Copy out the key from the dbt, it is still
				 * present from the previous pass.
				 */
				memset(&key_dbt, 0, sizeof(key_dbt));
				if ((ret = __os_malloc(
				    dbp->env, dbt.size, &key_buf)) != 0)
					return (ret);
				memcpy(key_buf, buf, dbt.size);
				key_dbt.data = key_buf;
				key_dbt.size = dbt.size;
				key_dbt.flags = DB_DBT_USERMEM;

				/* Loop until we hit the total length. */
				for (tlen = 0; tlen + sizeof(db_indx_t) < len;
				    tlen += dlen + 2 * sizeof(db_indx_t)) {
					/*
					 * Print the key for every duplicate
					 * item. Except the first dup, since
					 * the key was already output once by
					 * the previous iteration.
					 */
					if (tlen != 0) {
						if ((ret = __db_vrfy_prdbt(
						    &key_dbt, 0, " ", handle,
						    callback, 0, vdp)) != 0)
							err_ret = ret;
					}
					p = HKEYDATA_DATA(hk) + tlen;
					memcpy(&dlen, p, sizeof(db_indx_t));
					p += sizeof(db_indx_t);
					/*
					 * If dlen is too long, print all the
					 * rest of the dup set in a chunk.
					 */
					if (dlen + tlen + sizeof(db_indx_t) >
					    len) {
						dlen = len -
						    (tlen + sizeof(db_indx_t));
						err_ret = DB_VERIFY_BAD;
					}
					memcpy(buf, p, dlen);
					dbt.size = dlen;
					dbt.data = buf;
					if ((ret = __db_vrfy_prdbt(&dbt, 0, " ",
					    handle, callback, 0, vdp)) != 0)
						err_ret = ret;
				}
				__os_free(dbp->env, key_buf);
				break;
			default:
				if (!LF_ISSET(DB_AGGRESSIVE))
					break;
				err_ret = DB_VERIFY_BAD;
				break;
			}
		}
	}

	__os_free(dbp->env, buf);
	if ((t_ret = __db_salvage_markdone(vdp, pgno)) != 0)
		return (t_ret);
	return ((ret == 0 && err_ret != 0) ? err_ret : ret);
}

/*
 * __ham_meta2pgset --
 *	Return the set of hash pages corresponding to the given
 *	known-good meta page.
 *
 * PUBLIC: int __ham_meta2pgset __P((DB *, VRFY_DBINFO *, HMETA *, u_int32_t,
 * PUBLIC:     DB *));
 */
int
__ham_meta2pgset(dbp, vdp, hmeta, flags, pgset)
	DB *dbp;
	VRFY_DBINFO *vdp;
	HMETA *hmeta;
	u_int32_t flags;
	DB *pgset;
{
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t bucket, totpgs;
	int ret, val;

	/*
	 * We don't really need flags, but leave them for consistency with
	 * __bam_meta2pgset.
	 */
	COMPQUIET(flags, 0);
	ip = vdp->thread_info;

	DB_ASSERT(dbp->env, pgset != NULL);

	mpf = dbp->mpf;
	totpgs = 0;

	/*
	 * Loop through all the buckets, pushing onto pgset the corresponding
	 * page(s) for each one.
	 */
	for (bucket = 0; bucket <= hmeta->max_bucket; bucket++) {
		pgno = BS_TO_PAGE(bucket, hmeta->spares);

		/*
		 * We know the initial pgno is safe because the spares array has
		 * been verified.
		 *
		 * Safely walk the list of pages in this bucket.
		 */
		for (;;) {
			if ((ret =
			    __memp_fget(mpf, &pgno, ip, NULL, 0, &h)) != 0)
				return (ret);
			if (TYPE(h) == P_HASH || TYPE(h) == P_HASH_UNSORTED) {

				/*
				 * Make sure we don't go past the end of
				 * pgset.
				 */
				if (++totpgs > vdp->last_pgno) {
					(void)__memp_fput(mpf,
					    ip, h, dbp->priority);
					return (DB_VERIFY_BAD);
				}
				if ((ret = __db_vrfy_pgset_inc(pgset,
				    vdp->thread_info, pgno)) != 0) {
					(void)__memp_fput(mpf,
					    ip, h, dbp->priority);
					return (ret);
				}

				pgno = NEXT_PGNO(h);
			} else
				pgno = PGNO_INVALID;

			if ((ret = __memp_fput(mpf, ip, h, dbp->priority)) != 0)
				return (ret);

			/* If the new pgno is wonky, go onto the next bucket. */
			if (!IS_VALID_PGNO(pgno) ||
			    pgno == PGNO_INVALID)
				break;

			/*
			 * If we've touched this page before, we have a cycle;
			 * go on to the next bucket.
			 */
			if ((ret = __db_vrfy_pgset_get(pgset,
			    vdp->thread_info, pgno, &val)) != 0)
				return (ret);
			if (val != 0)
				break;
		}
	}
	return (0);
}

/*
 * __ham_dups_unsorted --
 *	Takes a known-safe hash duplicate set and its total length.
 *	Returns 1 if there are out-of-order duplicates in this set,
 *	0 if there are not.
 */
static int
__ham_dups_unsorted(dbp, buf, len)
	DB *dbp;
	u_int8_t *buf;
	u_int32_t len;
{
	DBT a, b;
	db_indx_t offset, dlen;
	int (*func) __P((DB *, const DBT *, const DBT *));

	memset(&a, 0, sizeof(DBT));
	memset(&b, 0, sizeof(DBT));

	func = (dbp->dup_compare == NULL) ? __bam_defcmp : dbp->dup_compare;

	/*
	 * Loop through the dup set until we hit the end or we find
	 * a pair of dups that's out of order.  b is always the current
	 * dup, a the one before it.
	 */
	for (offset = 0; offset < len; offset += DUP_SIZE(dlen)) {
		memcpy(&dlen, buf + offset, sizeof(db_indx_t));
		b.data = buf + offset + sizeof(db_indx_t);
		b.size = dlen;

		if (a.data != NULL && func(dbp, &a, &b) > 0)
			return (1);

		a.data = b.data;
		a.size = b.size;
	}

	return (0);
}
