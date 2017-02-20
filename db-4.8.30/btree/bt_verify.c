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
#include "dbinc/lock.h"
#include "dbinc/mp.h"

static int __bam_safe_getdata __P((DB *, DB_THREAD_INFO *,
    PAGE *, u_int32_t, int, DBT *, int *));
static int __bam_vrfy_inp __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t,
    db_indx_t *, u_int32_t));
static int __bam_vrfy_treeorder __P((DB *, DB_THREAD_INFO *, PAGE *,
    BINTERNAL *, BINTERNAL *, int (*)(DB *, const DBT *, const DBT *),
    u_int32_t));
static int __ram_vrfy_inp __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t,
    db_indx_t *, u_int32_t));

/*
 * __bam_vrfy_meta --
 *	Verify the btree-specific part of a metadata page.
 *
 * PUBLIC: int __bam_vrfy_meta __P((DB *, VRFY_DBINFO *, BTMETA *,
 * PUBLIC:     db_pgno_t, u_int32_t));
 */
int
__bam_vrfy_meta(dbp, vdp, meta, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	BTMETA *meta;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	int isbad, t_ret, ret;
	db_indx_t ovflsize;

	env = dbp->env;
	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	/*
	 * If VRFY_INCOMPLETE is not set, then we didn't come through
	 * __db_vrfy_pagezero and didn't incompletely
	 * check this page--we haven't checked it at all.
	 * Thus we need to call __db_vrfy_meta and check the common fields.
	 *
	 * If VRFY_INCOMPLETE is set, we've already done all the same work
	 * in __db_vrfy_pagezero, so skip the check.
	 */
	if (!F_ISSET(pip, VRFY_INCOMPLETE) &&
	    (ret = __db_vrfy_meta(dbp, vdp, &meta->dbmeta, pgno, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	/* bt_minkey:  must be >= 2; must produce sensible ovflsize */

	/* avoid division by zero */
	ovflsize = meta->minkey > 0 ?
	    B_MINKEY_TO_OVFLSIZE(dbp, meta->minkey, dbp->pgsize) : 0;

	if (meta->minkey < 2 ||
	    ovflsize > B_MINKEY_TO_OVFLSIZE(dbp, DEFMINKEYPAGE, dbp->pgsize)) {
		pip->bt_minkey = 0;
		isbad = 1;
		EPRINT((env,
	    "Page %lu: nonsensical bt_minkey value %lu on metadata page",
		    (u_long)pgno, (u_long)meta->minkey));
	} else
		pip->bt_minkey = meta->minkey;

	/* re_len: no constraints on this (may be zero or huge--we make rope) */
	pip->re_pad = meta->re_pad;
	pip->re_len = meta->re_len;

	/*
	 * The root must not be current page or 0 and it must be within
	 * database.  If this metadata page is the master meta data page
	 * of the file, then the root page had better be page 1.
	 */
	pip->root = 0;
	if (meta->root == PGNO_INVALID ||
	    meta->root == pgno || !IS_VALID_PGNO(meta->root) ||
	    (pgno == PGNO_BASE_MD && meta->root != 1)) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: nonsensical root page %lu on metadata page",
		    (u_long)pgno, (u_long)meta->root));
	} else
		pip->root = meta->root;

	/* Flags. */
	if (F_ISSET(&meta->dbmeta, BTM_RENUMBER))
		F_SET(pip, VRFY_IS_RRECNO);

	if (F_ISSET(&meta->dbmeta, BTM_SUBDB)) {
		/*
		 * If this is a master db meta page, it had better not have
		 * duplicates.
		 */
		if (F_ISSET(&meta->dbmeta, BTM_DUP) && pgno == PGNO_BASE_MD) {
			isbad = 1;
			EPRINT((env,
"Page %lu: Btree metadata page has both duplicates and multiple databases",
			    (u_long)pgno));
		}
		F_SET(pip, VRFY_HAS_SUBDBS);
	}

	if (F_ISSET(&meta->dbmeta, BTM_DUP))
		F_SET(pip, VRFY_HAS_DUPS);
	if (F_ISSET(&meta->dbmeta, BTM_DUPSORT))
		F_SET(pip, VRFY_HAS_DUPSORT);
	if (F_ISSET(&meta->dbmeta, BTM_RECNUM))
		F_SET(pip, VRFY_HAS_RECNUMS);
	if (F_ISSET(pip, VRFY_HAS_RECNUMS) && F_ISSET(pip, VRFY_HAS_DUPS)) {
		EPRINT((env,
    "Page %lu: Btree metadata page illegally has both recnums and dups",
		    (u_long)pgno));
		isbad = 1;
	}

	if (F_ISSET(&meta->dbmeta, BTM_RECNO)) {
		F_SET(pip, VRFY_IS_RECNO);
		dbp->type = DB_RECNO;
	} else if (F_ISSET(pip, VRFY_IS_RRECNO)) {
		isbad = 1;
		EPRINT((env,
    "Page %lu: metadata page has renumber flag set but is not recno",
		    (u_long)pgno));
	}

#ifdef HAVE_COMPRESSION
	if (F_ISSET(&meta->dbmeta, BTM_COMPRESS)) {
		F_SET(pip, VRFY_HAS_COMPRESS);
		if (!DB_IS_COMPRESSED(dbp)) {
			((BTREE *)dbp->bt_internal)->bt_compress =
			    __bam_defcompress;
			((BTREE *)dbp->bt_internal)->bt_decompress =
			    __bam_defdecompress;
		}
		/*
		 * Copy dup_compare to compress_dup_compare, and use the
		 * compression duplicate compare.
		 */
		if (F_ISSET(pip, VRFY_HAS_DUPSORT)) {
			if (dbp->dup_compare == NULL)
				dbp->dup_compare = __bam_defcmp;
			if (((BTREE *)dbp->bt_internal)->compress_dup_compare
				== NULL) {
				((BTREE *)dbp->bt_internal)->
					compress_dup_compare = dbp->dup_compare;
				dbp->dup_compare = __bam_compress_dupcmp;
			}
		}
	}

	if (F_ISSET(pip, VRFY_HAS_RECNUMS) && F_ISSET(pip, VRFY_HAS_COMPRESS)) {
		EPRINT((env,
    "Page %lu: Btree metadata page illegally has both recnums and compression",
		    (u_long)pgno));
		isbad = 1;
	}
	if (F_ISSET(pip, VRFY_HAS_DUPS) && !F_ISSET(pip, VRFY_HAS_DUPSORT) &&
	    F_ISSET(pip, VRFY_HAS_COMPRESS)) {
		EPRINT((env,
    "Page %lu: Btree metadata page illegally has both unsorted duplicates%s",
		    (u_long)pgno,
		    " and compression"));
		isbad = 1;
	}
#endif

	if (F_ISSET(pip, VRFY_IS_RECNO) && F_ISSET(pip, VRFY_HAS_DUPS)) {
		EPRINT((env,
		    "Page %lu: recno metadata page specifies duplicates",
		    (u_long)pgno));
		isbad = 1;
	}

	if (F_ISSET(&meta->dbmeta, BTM_FIXEDLEN))
		F_SET(pip, VRFY_IS_FIXEDLEN);
	else if (pip->re_len > 0) {
		/*
		 * It's wrong to have an re_len if it's not a fixed-length
		 * database
		 */
		isbad = 1;
		EPRINT((env,
		    "Page %lu: re_len of %lu in non-fixed-length database",
		    (u_long)pgno, (u_long)pip->re_len));
	}

	/*
	 * We do not check that the rest of the page is 0, because it may
	 * not be and may still be correct.
	 */

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (LF_ISSET(DB_SALVAGE) &&
	   (t_ret = __db_salvage_markdone(vdp, pgno)) != 0 && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __ram_vrfy_leaf --
 *	Verify a recno leaf page.
 *
 * PUBLIC: int __ram_vrfy_leaf __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t,
 * PUBLIC:     u_int32_t));
 */
int
__ram_vrfy_leaf(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	BKEYDATA *bk;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_indx_t i;
	int ret, t_ret, isbad;
	u_int32_t re_len_guess, len;

	env = dbp->env;
	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	if (TYPE(h) != P_LRECNO) {
		ret = __db_unknown_path(env, "__ram_vrfy_leaf");
		goto err;
	}

	/*
	 * Verify (and, if relevant, save off) page fields common to
	 * all PAGEs.
	 */
	if ((ret = __db_vrfy_datapage(dbp, vdp, h, pgno, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	/*
	 * Verify inp[].  Return immediately if it returns DB_VERIFY_BAD;
	 * further checks are dangerous.
	 */
	if ((ret = __bam_vrfy_inp(dbp,
	    vdp, h, pgno, &pip->entries, flags)) != 0)
		goto err;

	if (F_ISSET(pip, VRFY_HAS_DUPS)) {
		EPRINT((env,
		    "Page %lu: Recno database has dups", (u_long)pgno));
		ret = DB_VERIFY_BAD;
		goto err;
	}

	/*
	 * Walk through inp and see if the lengths of all the records are the
	 * same--if so, this may be a fixed-length database, and we want to
	 * save off this value.  We know inp to be safe if we've gotten this
	 * far.
	 */
	re_len_guess = 0;
	for (i = 0; i < NUM_ENT(h); i++) {
		bk = GET_BKEYDATA(dbp, h, i);
		/* KEYEMPTY.  Go on. */
		if (B_DISSET(bk->type))
			continue;
		if (bk->type == B_OVERFLOW)
			len = ((BOVERFLOW *)bk)->tlen;
		else if (bk->type == B_KEYDATA)
			len = bk->len;
		else {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: nonsensical type for item %lu",
			    (u_long)pgno, (u_long)i));
			continue;
		}
		if (re_len_guess == 0)
			re_len_guess = len;

		/*
		 * Is this item's len the same as the last one's?  If not,
		 * reset to 0 and break--we don't have a single re_len.
		 * Otherwise, go on to the next item.
		 */
		if (re_len_guess != len) {
			re_len_guess = 0;
			break;
		}
	}
	pip->re_len = re_len_guess;

	/* Save off record count. */
	pip->rec_cnt = NUM_ENT(h);

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __bam_vrfy --
 *	Verify a btree leaf or internal page.
 *
 * PUBLIC: int __bam_vrfy __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t,
 * PUBLIC:     u_int32_t));
 */
int
__bam_vrfy(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	VRFY_PAGEINFO *pip;
	int ret, t_ret, isbad;

	env = dbp->env;
	isbad = 0;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	switch (TYPE(h)) {
	case P_IBTREE:
	case P_IRECNO:
	case P_LBTREE:
	case P_LDUP:
		break;
	default:
		ret = __db_unknown_path(env, "__bam_vrfy");
		goto err;
	}

	/*
	 * Verify (and, if relevant, save off) page fields common to
	 * all PAGEs.
	 */
	if ((ret = __db_vrfy_datapage(dbp, vdp, h, pgno, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	/*
	 * The record count is, on internal pages, stored in an overloaded
	 * next_pgno field.  Save it off;  we'll verify it when we check
	 * overall database structure.  We could overload the field
	 * in VRFY_PAGEINFO, too, but this seems gross, and space
	 * is not at such a premium.
	 */
	pip->rec_cnt = RE_NREC(h);

	/*
	 * Verify inp[].
	 */
	if (TYPE(h) == P_IRECNO) {
		if ((ret = __ram_vrfy_inp(dbp,
		    vdp, h, pgno, &pip->entries, flags)) != 0)
			goto err;
	} else if ((ret = __bam_vrfy_inp(dbp,
	    vdp, h, pgno, &pip->entries, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
		EPRINT((env,
		    "Page %lu: item order check unsafe: skipping",
		    (u_long)pgno));
	} else if (!LF_ISSET(DB_NOORDERCHK) && (ret =
	    __bam_vrfy_itemorder(dbp,
	    vdp, vdp->thread_info, h, pgno, 0, 0, 0, flags)) != 0) {
		/*
		 * We know that the elements of inp are reasonable.
		 *
		 * Check that elements fall in the proper order.
		 */
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __ram_vrfy_inp --
 *	Verify that all entries in a P_IRECNO inp[] array are reasonable,
 *	and count them.  Note that P_LRECNO uses __bam_vrfy_inp;
 *	P_IRECNOs are a special, and simpler, case, since they have
 *	RINTERNALs rather than BKEYDATA/BINTERNALs.
 */
static int
__ram_vrfy_inp(dbp, vdp, h, pgno, nentriesp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	db_indx_t *nentriesp;
	u_int32_t flags;
{
	ENV *env;
	RINTERNAL *ri;
	VRFY_CHILDINFO child;
	VRFY_PAGEINFO *pip;
	int ret, t_ret, isbad;
	u_int32_t himark, i, offset, nentries;
	db_indx_t *inp;
	u_int8_t *pagelayout, *p;

	env = dbp->env;
	isbad = 0;
	memset(&child, 0, sizeof(VRFY_CHILDINFO));
	nentries = 0;
	pagelayout = NULL;

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	if (TYPE(h) != P_IRECNO) {
		ret = __db_unknown_path(env, "__ram_vrfy_inp");
		goto err;
	}

	himark = dbp->pgsize;
	if ((ret = __os_malloc(env, dbp->pgsize, &pagelayout)) != 0)
		goto err;
	memset(pagelayout, 0, dbp->pgsize);
	inp = P_INP(dbp, h);
	for (i = 0; i < NUM_ENT(h); i++) {
		if ((u_int8_t *)inp + i >= (u_int8_t *)h + himark) {
			EPRINT((env,
			    "Page %lu: entries listing %lu overlaps data",
			    (u_long)pgno, (u_long)i));
			ret = DB_VERIFY_BAD;
			goto err;
		}
		offset = inp[i];
		/*
		 * Check that the item offset is reasonable:  it points
		 * somewhere after the inp array and before the end of the
		 * page.
		 */
		if (offset <= (u_int32_t)((u_int8_t *)inp + i -
		    (u_int8_t *)h) ||
		    offset > (u_int32_t)(dbp->pgsize - RINTERNAL_SIZE)) {
			isbad = 1;
			EPRINT((env,
			    "Page %lu: bad offset %lu at index %lu",
			    (u_long)pgno, (u_long)offset, (u_long)i));
			continue;
		}

		/* Update the high-water mark (what HOFFSET should be) */
		if (offset < himark)
			himark = offset;

		nentries++;

		/* Make sure this RINTERNAL is not multiply referenced. */
		ri = GET_RINTERNAL(dbp, h, i);
		if (pagelayout[offset] == 0) {
			pagelayout[offset] = 1;
			child.pgno = ri->pgno;
			child.type = V_RECNO;
			child.nrecs = ri->nrecs;
			if ((ret = __db_vrfy_childput(vdp, pgno, &child)) != 0)
				goto err;
		} else {
			EPRINT((env,
		"Page %lu: RINTERNAL structure at offset %lu referenced twice",
			    (u_long)pgno, (u_long)offset));
			isbad = 1;
		}
	}

	for (p = pagelayout + himark;
	    p < pagelayout + dbp->pgsize;
	    p += RINTERNAL_SIZE)
		if (*p != 1) {
			EPRINT((env,
			    "Page %lu: gap between items at offset %lu",
			    (u_long)pgno, (u_long)(p - pagelayout)));
			isbad = 1;
		}

	if ((db_indx_t)himark != HOFFSET(h)) {
		EPRINT((env,
		    "Page %lu: bad HOFFSET %lu, appears to be %lu",
		    (u_long)pgno, (u_long)(HOFFSET(h)), (u_long)himark));
		isbad = 1;
	}

	*nentriesp = nentries;

err:	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (pagelayout != NULL)
		__os_free(env, pagelayout);
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

typedef enum { VRFY_ITEM_NOTSET=0, VRFY_ITEM_BEGIN, VRFY_ITEM_END } VRFY_ITEM;

/*
 * __bam_vrfy_inp --
 *	Verify that all entries in inp[] array are reasonable;
 *	count them.
 */
static int
__bam_vrfy_inp(dbp, vdp, h, pgno, nentriesp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	db_indx_t *nentriesp;
	u_int32_t flags;
{
	BKEYDATA *bk;
	BOVERFLOW *bo;
	ENV *env;
	VRFY_CHILDINFO child;
	VRFY_ITEM *pagelayout;
	VRFY_PAGEINFO *pip;
	u_int32_t himark, offset;		/*
						 * These would be db_indx_ts
						 * but for alignment.
						 */
	u_int32_t i, endoff, nentries;
	int isbad, initem, isdupitem, ret, t_ret;

	env = dbp->env;
	isbad = isdupitem = 0;
	nentries = 0;
	memset(&child, 0, sizeof(VRFY_CHILDINFO));
	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	switch (TYPE(h)) {
	case P_IBTREE:
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		break;
	default:
		/*
		 * In the salvager, we might call this from a page which
		 * we merely suspect is a btree page.  Otherwise, it
		 * shouldn't get called--if it is, that's a verifier bug.
		 */
		if (LF_ISSET(DB_SALVAGE))
			break;
		ret = __db_unknown_path(env, "__bam_vrfy_inp");
		goto err;
	}

	/*
	 * Loop through inp[], the array of items, until we either
	 * run out of entries or collide with the data.  Keep track
	 * of h_offset in himark.
	 *
	 * For each element in inp[i], make sure it references a region
	 * that starts after the end of the inp array (as defined by
	 * NUM_ENT(h)), ends before the beginning of the page, doesn't
	 * overlap any other regions, and doesn't have a gap between
	 * it and the region immediately after it.
	 */
	himark = dbp->pgsize;
	if ((ret = __os_calloc(
	    env, dbp->pgsize, sizeof(pagelayout[0]), &pagelayout)) != 0)
		goto err;
	for (i = 0; i < NUM_ENT(h); i++) {
		switch (ret = __db_vrfy_inpitem(dbp,
		    h, pgno, i, 1, flags, &himark, &offset)) {
		case 0:
			break;
		case DB_VERIFY_BAD:
			isbad = 1;
			continue;
		case DB_VERIFY_FATAL:
			isbad = 1;
			goto err;
		default:
			DB_ASSERT(env, ret != 0);
			break;
		}

		/*
		 * We now have a plausible beginning for the item, and we know
		 * its length is safe.
		 *
		 * Mark the beginning and end in pagelayout so we can make sure
		 * items have no overlaps or gaps.
		 */
		bk = GET_BKEYDATA(dbp, h, i);
		if (pagelayout[offset] == VRFY_ITEM_NOTSET)
			pagelayout[offset] = VRFY_ITEM_BEGIN;
		else if (pagelayout[offset] == VRFY_ITEM_BEGIN) {
			/*
			 * Having two inp entries that point at the same patch
			 * of page is legal if and only if the page is
			 * a btree leaf and they're onpage duplicate keys--
			 * that is, if (i % P_INDX) == 0.
			 */
			if ((i % P_INDX == 0) && (TYPE(h) == P_LBTREE)) {
				/* Flag for later. */
				F_SET(pip, VRFY_HAS_DUPS);

				/* Bump up nentries so we don't undercount. */
				nentries++;

				/*
				 * We'll check to make sure the end is
				 * equal, too.
				 */
				isdupitem = 1;
			} else {
				isbad = 1;
				EPRINT((env, "Page %lu: duplicated item %lu",
				    (u_long)pgno, (u_long)i));
			}
		}

		/*
		 * Mark the end.  Its location varies with the page type
		 * and the item type.
		 *
		 * If the end already has a sign other than 0, do nothing--
		 * it's an overlap that we'll catch later.
		 */
		switch (B_TYPE(bk->type)) {
		case B_KEYDATA:
			if (TYPE(h) == P_IBTREE)
				/* It's a BINTERNAL. */
				endoff = offset + BINTERNAL_SIZE(bk->len) - 1;
			else
				endoff = offset + BKEYDATA_SIZE(bk->len) - 1;
			break;
		case B_DUPLICATE:
			/*
			 * Flag that we have dups; we'll check whether
			 * that's okay during the structure check.
			 */
			F_SET(pip, VRFY_HAS_DUPS);
			/* FALLTHROUGH */
		case B_OVERFLOW:
			/*
			 * Overflow entries on internal pages are stored
			 * as the _data_ of a BINTERNAL;  overflow entries
			 * on leaf pages are stored as the entire entry.
			 */
			endoff = offset +
			    ((TYPE(h) == P_IBTREE) ?
			    BINTERNAL_SIZE(BOVERFLOW_SIZE) :
			    BOVERFLOW_SIZE) - 1;
			break;
		default:
			/*
			 * We'll complain later;  for now, just mark
			 * a minimum.
			 */
			endoff = offset + BKEYDATA_SIZE(0) - 1;
			break;
		}

		/*
		 * If this is an onpage duplicate key we've seen before,
		 * the end had better coincide too.
		 */
		if (isdupitem && pagelayout[endoff] != VRFY_ITEM_END) {
			EPRINT((env, "Page %lu: duplicated item %lu",
			    (u_long)pgno, (u_long)i));
			isbad = 1;
		} else if (pagelayout[endoff] == VRFY_ITEM_NOTSET)
			pagelayout[endoff] = VRFY_ITEM_END;
		isdupitem = 0;

		/*
		 * There should be no deleted items in a quiescent tree,
		 * except in recno.
		 */
		if (B_DISSET(bk->type) && TYPE(h) != P_LRECNO) {
			isbad = 1;
			EPRINT((env, "Page %lu: item %lu marked deleted",
			    (u_long)pgno, (u_long)i));
		}

		/*
		 * Check the type and such of bk--make sure it's reasonable
		 * for the pagetype.
		 */
		switch (B_TYPE(bk->type)) {
		case B_KEYDATA:
			/*
			 * This is a normal, non-overflow BKEYDATA or BINTERNAL.
			 * The only thing to check is the len, and that's
			 * already been done.
			 */
			break;
		case B_DUPLICATE:
			if (TYPE(h) == P_IBTREE) {
				isbad = 1;
				EPRINT((env,
    "Page %lu: duplicate page referenced by internal btree page at item %lu",
				    (u_long)pgno, (u_long)i));
				break;
			} else if (TYPE(h) == P_LRECNO) {
				isbad = 1;
				EPRINT((env,
	"Page %lu: duplicate page referenced by recno page at item %lu",
				    (u_long)pgno, (u_long)i));
				break;
			}
			/* FALLTHROUGH */
		case B_OVERFLOW:
			bo = (TYPE(h) == P_IBTREE) ?
			    (BOVERFLOW *)(((BINTERNAL *)bk)->data) :
			    (BOVERFLOW *)bk;

			if (B_TYPE(bk->type) == B_OVERFLOW)
				/* Make sure tlen is reasonable. */
				if (bo->tlen > dbp->pgsize * vdp->last_pgno) {
					isbad = 1;
					EPRINT((env,
				"Page %lu: impossible tlen %lu, item %lu",
					    (u_long)pgno,
					    (u_long)bo->tlen, (u_long)i));
					/* Don't save as a child. */
					break;
				}

			if (!IS_VALID_PGNO(bo->pgno) || bo->pgno == pgno ||
			    bo->pgno == PGNO_INVALID) {
				isbad = 1;
				EPRINT((env,
			    "Page %lu: offpage item %lu has bad pgno %lu",
				    (u_long)pgno, (u_long)i, (u_long)bo->pgno));
				/* Don't save as a child. */
				break;
			}

			child.pgno = bo->pgno;
			child.type = (B_TYPE(bk->type) == B_OVERFLOW ?
			    V_OVERFLOW : V_DUPLICATE);
			child.tlen = bo->tlen;
			if ((ret = __db_vrfy_childput(vdp, pgno, &child)) != 0)
				goto err;
			break;
		default:
			isbad = 1;
			EPRINT((env, "Page %lu: item %lu of invalid type %lu",
			    (u_long)pgno, (u_long)i, (u_long)B_TYPE(bk->type)));
			break;
		}
	}

	/*
	 * Now, loop through and make sure the items are contiguous and
	 * non-overlapping.
	 */
	initem = 0;
	for (i = himark; i < dbp->pgsize; i++)
		if (initem == 0)
			switch (pagelayout[i]) {
			case VRFY_ITEM_NOTSET:
				/* May be just for alignment. */
				if (i != DB_ALIGN(i, sizeof(u_int32_t)))
					continue;

				isbad = 1;
				EPRINT((env,
				    "Page %lu: gap between items at offset %lu",
				    (u_long)pgno, (u_long)i));
				/* Find the end of the gap */
				for (; pagelayout[i + 1] == VRFY_ITEM_NOTSET &&
				    (size_t)(i + 1) < dbp->pgsize; i++)
					;
				break;
			case VRFY_ITEM_BEGIN:
				/* We've found an item. Check its alignment. */
				if (i != DB_ALIGN(i, sizeof(u_int32_t))) {
					isbad = 1;
					EPRINT((env,
					    "Page %lu: offset %lu unaligned",
					    (u_long)pgno, (u_long)i));
				}
				initem = 1;
				nentries++;
				break;
			case VRFY_ITEM_END:
				/*
				 * We've hit the end of an item even though
				 * we don't think we're in one;  must
				 * be an overlap.
				 */
				isbad = 1;
				EPRINT((env,
				    "Page %lu: overlapping items at offset %lu",
				    (u_long)pgno, (u_long)i));
				break;
			}
		else
			switch (pagelayout[i]) {
			case VRFY_ITEM_NOTSET:
				/* In the middle of an item somewhere. Okay. */
				break;
			case VRFY_ITEM_END:
				/* End of an item; switch to out-of-item mode.*/
				initem = 0;
				break;
			case VRFY_ITEM_BEGIN:
				/*
				 * Hit a second item beginning without an
				 * end.  Overlap.
				 */
				isbad = 1;
				EPRINT((env,
				    "Page %lu: overlapping items at offset %lu",
				    (u_long)pgno, (u_long)i));
				break;
			}

	__os_free(env, pagelayout);

	/* Verify HOFFSET. */
	if ((db_indx_t)himark != HOFFSET(h)) {
		EPRINT((env, "Page %lu: bad HOFFSET %lu, appears to be %lu",
		    (u_long)pgno, (u_long)HOFFSET(h), (u_long)himark));
		isbad = 1;
	}

err:	if (nentriesp != NULL)
		*nentriesp = nentries;

	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;

	return ((isbad == 1 && ret == 0) ? DB_VERIFY_BAD : ret);
}

/*
 * __bam_vrfy_itemorder --
 *	Make sure the items on a page sort correctly.
 *
 *	Assumes that NUM_ENT(h) and inp[0]..inp[NUM_ENT(h) - 1] are
 *	reasonable;  be sure that __bam_vrfy_inp has been called first.
 *
 *	If ovflok is set, it also assumes that overflow page chains
 *	hanging off the current page have been sanity-checked, and so we
 *	can use __bam_cmp to verify their ordering.  If it is not set,
 *	and we run into an overflow page, carp and return DB_VERIFY_BAD;
 *	we shouldn't be called if any exist.
 *
 * PUBLIC: int __bam_vrfy_itemorder __P((DB *, VRFY_DBINFO *, DB_THREAD_INFO *,
 * PUBLIC:      PAGE *, db_pgno_t, u_int32_t, int, int, u_int32_t));
 */
int
__bam_vrfy_itemorder(dbp, vdp, ip, h, pgno, nentries, ovflok, hasdups, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	DB_THREAD_INFO *ip;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t nentries;
	int ovflok, hasdups;
	u_int32_t flags;
{
	BINTERNAL *bi;
	BKEYDATA *bk;
	BOVERFLOW *bo;
	BTREE *bt;
	DBC *dbc;
	DBT dbta, dbtb, dup_1, dup_2, *p1, *p2, *tmp;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_indx_t i, *inp;
	int adj, cmp, freedup_1, freedup_2, isbad, ret, t_ret;
	int (*dupfunc) __P((DB *, const DBT *, const DBT *));
	int (*func) __P((DB *, const DBT *, const DBT *));
	void *buf1, *buf2, *tmpbuf;

	/*
	 * We need to work in the ORDERCHKONLY environment where we might
	 * not have a pip, but we also may need to work in contexts where
	 * NUM_ENT isn't safe.
	 */
	if (vdp != NULL) {
		if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
			return (ret);
		nentries = pip->entries;
	} else
		pip = NULL;

	env = dbp->env;
	ret = isbad = 0;
	bo = NULL;			/* Shut up compiler. */

	memset(&dbta, 0, sizeof(DBT));
	F_SET(&dbta, DB_DBT_REALLOC);

	memset(&dbtb, 0, sizeof(DBT));
	F_SET(&dbtb, DB_DBT_REALLOC);

	buf1 = buf2 = NULL;

	DB_ASSERT(env, !LF_ISSET(DB_NOORDERCHK));

	dupfunc = (dbp->dup_compare == NULL) ? __bam_defcmp : dbp->dup_compare;
	if (TYPE(h) == P_LDUP)
		func = dupfunc;
	else {
		func = __bam_defcmp;
		if (dbp->bt_internal != NULL) {
			bt = (BTREE *)dbp->bt_internal;
			if (bt->bt_compare != NULL)
				func = bt->bt_compare;
		}
	}

	/*
	 * We alternate our use of dbta and dbtb so that we can walk
	 * through the page key-by-key without copying a dbt twice.
	 * p1 is always the dbt for index i - 1, and p2 for index i.
	 * Reset the data pointers in case we are retrying.
	 */
retry:	p1 = &dbta;
	p1->data = NULL;
	p2 = &dbtb;
	p2->data = NULL;

	/*
	 * Loop through the entries.  nentries ought to contain the
	 * actual count, and so is a safe way to terminate the loop;  whether
	 * we inc. by one or two depends on whether we're a leaf page--
	 * on a leaf page, we care only about keys.  On internal pages
	 * and LDUP pages, we want to check the order of all entries.
	 *
	 * Note that on IBTREE pages or the index page of a partitioned
	 * database, we start with item 1, since item 0 doesn't get looked
	 * at by __bam_cmp.
	 */
	inp = P_INP(dbp, h);
	adj = (TYPE(h) == P_LBTREE) ? P_INDX : O_INDX;
	for (i = (TYPE(h) == P_IBTREE || dbp->p_internal != NULL) ? adj : 0;
	    i < nentries; i += adj) {
		/*
		 * Put key i-1, now in p2, into p1, by swapping DBTs and bufs.
		 */
		tmp = p1;
		p1 = p2;
		p2 = tmp;
		tmpbuf = buf1;
		buf1 = buf2;
		buf2 = tmpbuf;

		/*
		 * Get key i into p2.
		 */
		switch (TYPE(h)) {
		case P_IBTREE:
			bi = GET_BINTERNAL(dbp, h, i);
			if (B_TYPE(bi->type) == B_OVERFLOW) {
				bo = (BOVERFLOW *)(bi->data);
				goto overflow;
			} else {
				p2->data = bi->data;
				p2->size = bi->len;
			}

			/*
			 * The leftmost key on an internal page must be
			 * len 0, since it's just a placeholder and
			 * automatically sorts less than all keys.
			 *
			 * XXX
			 * This criterion does not currently hold!
			 * See todo list item #1686.  Meanwhile, it's harmless
			 * to just not check for it.
			 */
#if 0
			if (i == 0 && bi->len != 0) {
				isbad = 1;
				EPRINT((env,
		"Page %lu: lowest key on internal page of nonzero length",
				    (u_long)pgno));
			}
#endif
			break;
		case P_LBTREE:
		case P_LDUP:
			bk = GET_BKEYDATA(dbp, h, i);
			if (B_TYPE(bk->type) == B_OVERFLOW) {
				bo = (BOVERFLOW *)bk;
				goto overflow;
			} else {
				p2->data = bk->data;
				p2->size = bk->len;
			}
			break;
		default:
			/*
			 * This means our caller screwed up and sent us
			 * an inappropriate page.
			 */
			ret = __db_unknown_path(env, "__bam_vrfy_itemorder");
			goto err;
		}

		if (0) {
			/*
			 * If ovflok != 1, we can't safely go chasing
			 * overflow pages with the normal routines now;
			 * they might be unsafe or nonexistent.  Mark this
			 * page as incomplete and return.
			 *
			 * Note that we don't need to worry about freeing
			 * buffers, since they can't have been allocated
			 * if overflow items are unsafe.
			 */
overflow:		if (!ovflok) {
				F_SET(pip, VRFY_INCOMPLETE);
				goto err;
			}

			/*
			 * Overflow items are safe to chase.  Do so.
			 * Fetch the overflow item into p2->data,
			 * NULLing it or reallocing it as appropriate.
			 *
			 * (We set p2->data to buf2 before the call
			 * so we're sure to realloc if we can and if p2
			 * was just pointing at a non-overflow item.)
			 */
			p2->data = buf2;
			if ((ret = __db_cursor_int(dbp, ip, NULL, DB_BTREE,
			    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &dbc)) != 0)
				goto err;
			if ((ret = __db_goff(dbc,
			    p2, bo->tlen, bo->pgno, NULL, NULL)) != 0) {
				isbad = 1;
				EPRINT((env,
			    "Page %lu: error %lu in fetching overflow item %lu",
				    (u_long)pgno, (u_long)ret, (u_long)i));
			}
			/* In case it got realloc'ed and thus changed. */
			buf2 = p2->data;
		}

		/* Compare with the last key. */
		if (p1->data != NULL && p2->data != NULL) {
			cmp = inp[i] == inp[i - adj] ? 0 : func(dbp, p1, p2);

			/* comparison succeeded */
			if (cmp > 0) {
				/*
				 * If we are looking at an internal page, we
				 * don't know whether it is part of the main
				 * database or in an off-page-duplicate tree.
				 * If the main comparator fails, retry with
				 * the duplicate comparator.
				 */
				if (TYPE(h) == P_IBTREE && func != dupfunc) {
					func = dupfunc;
					goto retry;
				}

				isbad = 1;
				EPRINT((env,
				    "Page %lu: out-of-order key at entry %lu",
				    (u_long)pgno, (u_long)i));
				/* proceed */
			} else if (cmp == 0) {
				if (inp[i] != inp[i - adj]) {
					/* See above. */
					if (TYPE(h) == P_IBTREE &&
					    func != dupfunc) {
						func = dupfunc;
						goto retry;
					}
					isbad = 1;
					EPRINT((env,
				     "Page %lu: non-dup dup key at entry %lu",
					   (u_long)pgno, (u_long)i));
				}
				/*
				 * If they compared equally, this
				 * had better be a (sub)database with dups.
				 * Mark it so we can check during the
				 * structure check.
				 */
				if (pip != NULL)
					F_SET(pip, VRFY_HAS_DUPS);
				else if (hasdups == 0) {
					/* See above. */
					if (TYPE(h) == P_IBTREE &&
					    func != dupfunc) {
						func = dupfunc;
						goto retry;
					}
					isbad = 1;
					EPRINT((env,
	"Page %lu: database with no duplicates has duplicated keys",
					    (u_long)pgno));
				}

				/*
				 * If we're a btree leaf, check to see
				 * if the data items of these on-page dups are
				 * in sorted order.  If not, flag this, so
				 * that we can make sure during the
				 * structure checks that the DUPSORT flag
				 * is unset.
				 *
				 * At this point i points to a duplicate key.
				 * Compare the datum before it (same key)
				 * to the datum after it, i.e. i-1 to i+1.
				 */
				if (TYPE(h) == P_LBTREE) {
					/*
					 * Unsafe;  continue and we'll pick
					 * up the bogus nentries later.
					 */
					if (i + 1 >= (db_indx_t)nentries)
						continue;

					/*
					 * We don't bother with clever memory
					 * management with on-page dups,
					 * as it's only really a big win
					 * in the overflow case, and overflow
					 * dups are probably (?) rare.
					 */
					if (((ret = __bam_safe_getdata(dbp,
					    ip, h, i - 1, ovflok,
					    &dup_1, &freedup_1)) != 0) ||
					    ((ret = __bam_safe_getdata(dbp,
					    ip, h, i + 1, ovflok,
					    &dup_2, &freedup_2)) != 0))
						goto err;

					/*
					 * If either of the data are NULL,
					 * it's because they're overflows and
					 * it's not safe to chase them now.
					 * Mark an incomplete and return.
					 */
					if (dup_1.data == NULL ||
					    dup_2.data == NULL) {
						DB_ASSERT(env, !ovflok);
						F_SET(pip, VRFY_INCOMPLETE);
						goto err;
					}

					/*
					 * If the dups are out of order,
					 * flag this.  It's not an error
					 * until we do the structure check
					 * and see whether DUPSORT is set.
					 */
					if (dupfunc(dbp, &dup_1, &dup_2) > 0)
						F_SET(pip, VRFY_DUPS_UNSORTED);

					if (freedup_1)
						__os_ufree(env, dup_1.data);
					if (freedup_2)
						__os_ufree(env, dup_2.data);
				}
			}
		}
	}

err:	if (pip != NULL && ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, pip)) != 0) && ret == 0)
		ret = t_ret;

	if (buf1 != NULL)
		__os_ufree(env, buf1);
	if (buf2 != NULL)
		__os_ufree(env, buf2);

	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __bam_vrfy_structure --
 *	Verify the tree structure of a btree database (including the master
 *	database containing subdbs).
 *
 * PUBLIC: int __bam_vrfy_structure __P((DB *, VRFY_DBINFO *, db_pgno_t,
 * PUBLIC:     void *, void *, u_int32_t));
 */
int
__bam_vrfy_structure(dbp, vdp, meta_pgno, lp, rp, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t meta_pgno;
	void *lp, *rp;
	u_int32_t flags;
{
	DB *pgset;
	ENV *env;
	VRFY_PAGEINFO *mip, *rip;
	db_pgno_t root, p;
	int t_ret, ret;
	u_int32_t nrecs, level, relen, stflags;

	env = dbp->env;
	mip = rip = 0;
	pgset = vdp->pgset;

	if ((ret = __db_vrfy_getpageinfo(vdp, meta_pgno, &mip)) != 0)
		return (ret);

	if ((ret = __db_vrfy_pgset_get(pgset,
	    vdp->thread_info, meta_pgno, (int *)&p)) != 0)
		goto err;
	if (p != 0) {
		EPRINT((env,
		    "Page %lu: btree metadata page observed twice",
		    (u_long)meta_pgno));
		ret = DB_VERIFY_BAD;
		goto err;
	}
	if ((ret =
	    __db_vrfy_pgset_inc(pgset, vdp->thread_info, meta_pgno)) != 0)
		goto err;

	root = mip->root;

	if (root == 0) {
		EPRINT((env,
		    "Page %lu: btree metadata page has no root",
		    (u_long)meta_pgno));
		ret = DB_VERIFY_BAD;
		goto err;
	}

	if ((ret = __db_vrfy_getpageinfo(vdp, root, &rip)) != 0)
		goto err;

	switch (rip->type) {
	case P_IBTREE:
	case P_LBTREE:
		stflags = flags | DB_ST_TOPLEVEL;
		if (F_ISSET(mip, VRFY_HAS_DUPS))
			stflags |= DB_ST_DUPOK;
		if (F_ISSET(mip, VRFY_HAS_DUPSORT))
			stflags |= DB_ST_DUPSORT;
		if (F_ISSET(mip, VRFY_HAS_RECNUMS))
			stflags |= DB_ST_RECNUM;
		ret = __bam_vrfy_subtree(dbp,
		    vdp, root, lp, rp, stflags, NULL, NULL, NULL);
		break;
	case P_IRECNO:
	case P_LRECNO:
		stflags =
		    flags | DB_ST_RECNUM | DB_ST_IS_RECNO | DB_ST_TOPLEVEL;
		if (mip->re_len > 0)
			stflags |= DB_ST_RELEN;
		if ((ret = __bam_vrfy_subtree(dbp, vdp,
		    root, NULL, NULL, stflags, &level, &nrecs, &relen)) != 0)
			goto err;
		/*
		 * Even if mip->re_len > 0, re_len may come back zero if the
		 * tree is empty.  It should be okay to just skip the check in
		 * this case, as if there are any non-deleted keys at all,
		 * that should never happen.
		 */
		if (mip->re_len > 0 && relen > 0 && mip->re_len != relen) {
			EPRINT((env,
			    "Page %lu: recno database has bad re_len %lu",
			    (u_long)meta_pgno, (u_long)relen));
			ret = DB_VERIFY_BAD;
			goto err;
		}
		ret = 0;
		break;
	case P_LDUP:
		EPRINT((env,
		    "Page %lu: duplicate tree referenced from metadata page",
		    (u_long)meta_pgno));
		ret = DB_VERIFY_BAD;
		break;
	default:
		EPRINT((env,
	    "Page %lu: btree root of incorrect type %lu on metadata page",
		    (u_long)meta_pgno, (u_long)rip->type));
		ret = DB_VERIFY_BAD;
		break;
	}

err:	if (mip != NULL && ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, mip)) != 0) && ret == 0)
		ret = t_ret;
	if (rip != NULL && ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, rip)) != 0) && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __bam_vrfy_subtree--
 *	Verify a subtree (or entire) btree with specified root.
 *
 *	Note that this is public because it must be called to verify
 *	offpage dup trees, including from hash.
 *
 * PUBLIC: int __bam_vrfy_subtree __P((DB *, VRFY_DBINFO *, db_pgno_t, void *,
 * PUBLIC:     void *, u_int32_t, u_int32_t *, u_int32_t *, u_int32_t *));
 */
int
__bam_vrfy_subtree(dbp, vdp, pgno, l, r, flags, levelp, nrecsp, relenp)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	void *l, *r;
	u_int32_t flags, *levelp, *nrecsp, *relenp;
{
	BINTERNAL *li, *ri;
	DB *pgset;
	DBC *cc;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	VRFY_CHILDINFO *child;
	VRFY_PAGEINFO *pip;
	db_indx_t i;
	db_pgno_t next_pgno, prev_pgno;
	db_recno_t child_nrecs, nrecs;
	u_int32_t child_level, child_relen, j, level, relen, stflags;
	u_int8_t leaf_type;
	int (*func) __P((DB *, const DBT *, const DBT *));
	int isbad, p, ret, t_ret, toplevel;

	if (levelp != NULL)	/* Don't leave uninitialized on error. */
		*levelp = 0;
	if (nrecsp != NULL)
		*nrecsp = 0;

	env = dbp->env;
	mpf = dbp->mpf;
	h = NULL;
	next_pgno = prev_pgno = PGNO_INVALID;
	nrecs = 0;
	relen = 0;
	leaf_type = P_INVALID;
	isbad = ret = 0;

	/* Provide feedback on our progress to the application. */
	if (!LF_ISSET(DB_SALVAGE))
		__db_vrfy_struct_feedback(dbp, vdp);

	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	cc = NULL;
	level = pip->bt_level;

	toplevel = LF_ISSET(DB_ST_TOPLEVEL) ? 1 : 0;
	LF_CLR(DB_ST_TOPLEVEL);

	/*
	 * If this is the root, initialize the vdp's prev- and next-pgno
	 * accounting.
	 *
	 * For each leaf page we hit, we'll want to make sure that
	 * vdp->prev_pgno is the same as pip->prev_pgno and vdp->next_pgno is
	 * our page number.  Then, we'll set vdp->next_pgno to pip->next_pgno
	 * and vdp->prev_pgno to our page number, and the next leaf page in
	 * line should be able to do the same verification.
	 */
	if (toplevel) {
		/*
		 * Cache the values stored in the vdp so that if we're an
		 * auxiliary tree such as an off-page duplicate set, our
		 * caller's leaf page chain doesn't get lost.
		 */
		prev_pgno = vdp->prev_pgno;
		next_pgno = vdp->next_pgno;
		leaf_type = vdp->leaf_type;
		vdp->next_pgno = vdp->prev_pgno = PGNO_INVALID;
		vdp->leaf_type = P_INVALID;
	}

	/*
	 * We are recursively descending a btree, starting from the root
	 * and working our way out to the leaves.
	 *
	 * There are four cases we need to deal with:
	 *	1. pgno is a recno leaf page.  Any children are overflows.
	 *	2. pgno is a duplicate leaf page.  Any children
	 *	   are overflow pages;  traverse them, and then return
	 *	   level and nrecs.
	 *	3. pgno is an ordinary leaf page.  Check whether dups are
	 *	   allowed, and if so, traverse any off-page dups or
	 *	   overflows.  Then return nrecs and level.
	 *	4. pgno is a recno internal page.  Recursively check any
	 *	   child pages, making sure their levels are one lower
	 *	   and their nrecs sum to ours.
	 *	5. pgno is a btree internal page.  Same as #4, plus we
	 *	   must verify that for each pair of BINTERNAL entries
	 *	   N and N+1, the leftmost item on N's child sorts
	 *	   greater than N, and the rightmost item on N's child
	 *	   sorts less than N+1.
	 *
	 * Furthermore, in any sorted page type (P_LDUP, P_LBTREE, P_IBTREE),
	 * we need to verify the internal sort order is correct if,
	 * due to overflow items, we were not able to do so earlier.
	 */
	switch (pip->type) {
	case P_LRECNO:
	case P_LDUP:
	case P_LBTREE:
		/*
		 * Cases 1, 2 and 3.
		 *
		 * We're some sort of leaf page;  verify
		 * that our linked list of leaves is consistent.
		 */
		if (vdp->leaf_type == P_INVALID) {
			/*
			 * First leaf page.  Set the type that all its
			 * successors should be, and verify that our prev_pgno
			 * is PGNO_INVALID.
			 */
			vdp->leaf_type = pip->type;
			if (pip->prev_pgno != PGNO_INVALID)
				goto bad_prev;
		} else {
			/*
			 * Successor leaf page. Check our type, the previous
			 * page's next_pgno, and our prev_pgno.
			 */
			if (pip->type != vdp->leaf_type) {
				isbad = 1;
				EPRINT((env,
	"Page %lu: unexpected page type %lu found in leaf chain (expected %lu)",
				    (u_long)pip->pgno, (u_long)pip->type,
				    (u_long)vdp->leaf_type));
			}

			/*
			 * Don't do the prev/next_pgno checks if we've lost
			 * leaf pages due to another corruption.
			 */
			if (!F_ISSET(vdp, VRFY_LEAFCHAIN_BROKEN)) {
				if (pip->pgno != vdp->next_pgno) {
					isbad = 1;
					EPRINT((env,
	"Page %lu: incorrect next_pgno %lu found in leaf chain (should be %lu)",
					    (u_long)vdp->prev_pgno,
					    (u_long)vdp->next_pgno,
					    (u_long)pip->pgno));
				}
				if (pip->prev_pgno != vdp->prev_pgno) {
bad_prev:				isbad = 1;
					EPRINT((env,
    "Page %lu: incorrect prev_pgno %lu found in leaf chain (should be %lu)",
					    (u_long)pip->pgno,
					    (u_long)pip->prev_pgno,
					    (u_long)vdp->prev_pgno));
				}
			}
		}
		vdp->prev_pgno = pip->pgno;
		vdp->next_pgno = pip->next_pgno;
		F_CLR(vdp, VRFY_LEAFCHAIN_BROKEN);

		/*
		 * Overflow pages are common to all three leaf types;
		 * traverse the child list, looking for overflows.
		 */
		if ((ret = __db_vrfy_childcursor(vdp, &cc)) != 0)
			goto err;
		for (ret = __db_vrfy_ccset(cc, pgno, &child); ret == 0;
		    ret = __db_vrfy_ccnext(cc, &child))
			if (child->type == V_OVERFLOW &&
			    (ret = __db_vrfy_ovfl_structure(dbp, vdp,
			    child->pgno, child->tlen,
			    flags | DB_ST_OVFL_LEAF)) != 0) {
				if (ret == DB_VERIFY_BAD)
					isbad = 1;
				else
					goto done;
			}

		if ((ret = __db_vrfy_ccclose(cc)) != 0)
			goto err;
		cc = NULL;

		/* Case 1 */
		if (pip->type == P_LRECNO) {
			if (!LF_ISSET(DB_ST_IS_RECNO) &&
			    !(LF_ISSET(DB_ST_DUPOK) &&
			    !LF_ISSET(DB_ST_DUPSORT))) {
				isbad = 1;
				EPRINT((env,
				    "Page %lu: recno leaf page non-recno tree",
				    (u_long)pgno));
				goto done;
			}
			goto leaf;
		} else if (LF_ISSET(DB_ST_IS_RECNO)) {
			/*
			 * It's a non-recno leaf.  Had better not be a recno
			 * subtree.
			 */
			isbad = 1;
			EPRINT((env,
			    "Page %lu: non-recno leaf page in recno tree",
			    (u_long)pgno));
			goto done;
		}

		/* Case 2--no more work. */
		if (pip->type == P_LDUP)
			goto leaf;

		/* Case 3 */

		/* Check if we have any dups. */
		if (F_ISSET(pip, VRFY_HAS_DUPS)) {
			/* If dups aren't allowed in this btree, trouble. */
			if (!LF_ISSET(DB_ST_DUPOK)) {
				isbad = 1;
				EPRINT((env,
				    "Page %lu: duplicates in non-dup btree",
				    (u_long)pgno));
			} else {
				/*
				 * We correctly have dups.  If any are off-page,
				 * traverse those btrees recursively.
				 */
				if ((ret =
				    __db_vrfy_childcursor(vdp, &cc)) != 0)
					goto err;
				for (ret = __db_vrfy_ccset(cc, pgno, &child);
				    ret == 0;
				    ret = __db_vrfy_ccnext(cc, &child)) {
					stflags =
					    flags | DB_ST_RECNUM | DB_ST_DUPSET;
					/* Skip any overflow entries. */
					if (child->type == V_DUPLICATE) {
						if ((ret = __db_vrfy_duptype(
						    dbp, vdp, child->pgno,
						    stflags)) != 0) {
							isbad = 1;
							/* Next child. */
							continue;
						}
						if ((ret = __bam_vrfy_subtree(
						    dbp, vdp, child->pgno,
						    NULL, NULL,
						    stflags | DB_ST_TOPLEVEL,
						    NULL, NULL, NULL)) != 0) {
							if (ret ==
							    DB_VERIFY_BAD)
								isbad = 1;
							else
								goto err;
						}
					}
				}

				if ((ret = __db_vrfy_ccclose(cc)) != 0)
					goto err;
				cc = NULL;

				/*
				 * If VRFY_DUPS_UNSORTED is set,
				 * DB_ST_DUPSORT had better not be.
				 */
				if (F_ISSET(pip, VRFY_DUPS_UNSORTED) &&
				    LF_ISSET(DB_ST_DUPSORT)) {
					isbad = 1;
					EPRINT((env,
		    "Page %lu: unsorted duplicate set in sorted-dup database",
					    (u_long)pgno));
				}
			}
		}
		goto leaf;
	case P_IBTREE:
	case P_IRECNO:
		/* We handle these below. */
		break;
	default:
		/*
		 * If a P_IBTREE or P_IRECNO contains a reference to an
		 * invalid page, we'll wind up here;  handle it gracefully.
		 * Note that the code at the "done" label assumes that the
		 * current page is a btree/recno one of some sort;  this
		 * is not the case here, so we goto err.
		 *
		 * If the page is entirely zeroed, its pip->type will be a lie
		 * (we assumed it was a hash page, as they're allowed to be
		 * zeroed);  handle this case specially.
		 */
		if (F_ISSET(pip, VRFY_IS_ALLZEROES))
			ZEROPG_ERR_PRINT(env, pgno, "btree or recno page");
		else
			EPRINT((env,
	    "Page %lu: btree or recno page is of inappropriate type %lu",
			    (u_long)pgno, (u_long)pip->type));

		/*
		 * We probably lost a leaf page (or more if this was an
		 * internal page) from our prev/next_pgno chain.  Flag
		 * that this is expected;  we don't want or need to
		 * spew error messages about erroneous prev/next_pgnos,
		 * since that's probably not the real problem.
		 */
		F_SET(vdp, VRFY_LEAFCHAIN_BROKEN);

		ret = DB_VERIFY_BAD;
		goto err;
	}

	/*
	 * Cases 4 & 5: This is a btree or recno internal page.  For each child,
	 * recurse, keeping a running count of nrecs and making sure the level
	 * is always reasonable.
	 */
	if ((ret = __db_vrfy_childcursor(vdp, &cc)) != 0)
		goto err;
	for (ret = __db_vrfy_ccset(cc, pgno, &child); ret == 0;
	    ret = __db_vrfy_ccnext(cc, &child))
		if (child->type == V_RECNO) {
			if (pip->type != P_IRECNO) {
				ret = __db_unknown_path(
				    env, "__bam_vrfy_subtree");
				goto err;
			}
			if ((ret = __bam_vrfy_subtree(dbp, vdp, child->pgno,
			    NULL, NULL, flags, &child_level, &child_nrecs,
			    &child_relen)) != 0) {
				if (ret == DB_VERIFY_BAD)
					isbad = 1;
				else
					goto done;
			}

			if (LF_ISSET(DB_ST_RELEN)) {
				if (relen == 0)
					relen = child_relen;
				/*
				 * child_relen may be zero if the child subtree
				 * is empty.
				 */
				else if (child_relen > 0 &&
				    relen != child_relen) {
					isbad = 1;
					EPRINT((env,
			   "Page %lu: recno page returned bad re_len %lu",
					    (u_long)child->pgno,
					    (u_long)child_relen));
				}
				if (relenp)
					*relenp = relen;
			}
			if (LF_ISSET(DB_ST_RECNUM)) {
				if (child->nrecs != child_nrecs) {
					isbad = 1;
					EPRINT((env,
		"Page %lu: record count incorrect: actual %lu, in record %lu",
					    (u_long)child->pgno,
					    (u_long)child_nrecs,
					    (u_long)child->nrecs));
				}
				nrecs += child_nrecs;
			}
			if (isbad == 0 && level != child_level + 1) {
				isbad = 1;
				EPRINT((env,
		"Page %lu: recno level incorrect: got %lu, expected %lu",
				    (u_long)child->pgno, (u_long)child_level,
				    (u_long)(level - 1)));
			}
		} else if (child->type == V_OVERFLOW) {
			/*
			 * It is possible for one internal page to reference
			 * a single overflow page twice, if all the items
			 * in the subtree referenced by slot 0 are deleted,
			 * then a similar number of items are put back
			 * before the key that formerly had been in slot 1.
			 *
			 * (Btree doesn't look at the key in slot 0, so the
			 * fact that the key formerly at slot 1 is the "wrong"
			 * parent of the stuff in the slot 0 subtree isn't
			 * really incorrect.)
			 *
			 * __db_vrfy_ovfl_structure is designed to be
			 * efficiently called multiple times for multiple
			 * references;  call it here as many times as is
			 * appropriate.
			 */

			/* Otherwise, __db_vrfy_childput would be broken. */
			DB_ASSERT(env, child->refcnt >= 1);

			/*
			 * An overflow referenced more than twice here
			 * shouldn't happen.
			 */
			if (child->refcnt > 2) {
				isbad = 1;
				EPRINT((env,
    "Page %lu: overflow page %lu referenced more than twice from internal page",
				    (u_long)pgno, (u_long)child->pgno));
			} else
				for (j = 0; j < child->refcnt; j++)
					if ((ret = __db_vrfy_ovfl_structure(dbp,
					    vdp, child->pgno, child->tlen,
					    flags)) != 0) {
						if (ret == DB_VERIFY_BAD)
							isbad = 1;
						else
							goto done;
					}
		}

	if ((ret = __db_vrfy_ccclose(cc)) != 0)
		goto err;
	cc = NULL;

	/* We're done with case 4. */
	if (pip->type == P_IRECNO)
		goto done;

	/*
	 * Case 5.  Btree internal pages.
	 * As described above, we need to iterate through all the
	 * items on the page and make sure that our children sort appropriately
	 * with respect to them.
	 *
	 * For each entry, li will be the "left-hand" key for the entry
	 * itself, which must sort lower than all entries on its child;
	 * ri will be the key to its right, which must sort greater.
	 */
	if (h == NULL &&
	    (ret = __memp_fget(mpf, &pgno, vdp->thread_info, NULL, 0, &h)) != 0)
		goto err;
	for (i = 0; i < pip->entries; i += O_INDX) {
		li = GET_BINTERNAL(dbp, h, i);
		ri = (i + O_INDX < pip->entries) ?
		    GET_BINTERNAL(dbp, h, i + O_INDX) : r;

		/*
		 * The leftmost key is forcibly sorted less than all entries,
		 * so don't bother passing it.
		 */
		if ((ret = __bam_vrfy_subtree(dbp, vdp, li->pgno,
		    i == 0 ? NULL : li, ri, flags, &child_level,
		    &child_nrecs, NULL)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto done;
		}

		if (LF_ISSET(DB_ST_RECNUM)) {
			/*
			 * Keep a running tally on the actual record count so
			 * we can return it to our parent (if we have one) or
			 * compare it to the NRECS field if we're a root page.
			 */
			nrecs += child_nrecs;

			/*
			 * Make sure the actual record count of the child
			 * is equal to the value in the BINTERNAL structure.
			 */
			if (li->nrecs != child_nrecs) {
				isbad = 1;
				EPRINT((env,
	"Page %lu: item %lu has incorrect record count of %lu, should be %lu",
				    (u_long)pgno, (u_long)i, (u_long)li->nrecs,
				    (u_long)child_nrecs));
			}
		}

		if (level != child_level + 1) {
			isbad = 1;
			EPRINT((env,
		"Page %lu: Btree level incorrect: got %lu, expected %lu",
			    (u_long)li->pgno,
			    (u_long)child_level, (u_long)(level - 1)));
		}
	}

	if (0) {
leaf:		level = LEAFLEVEL;
		if (LF_ISSET(DB_ST_RECNUM))
			nrecs = pip->rec_cnt;

		/* XXX
		 * We should verify that the record count on a leaf page
		 * is the sum of the number of keys and the number of
		 * records in its off-page dups.  This requires looking
		 * at the page again, however, and it may all be changing
		 * soon, so for now we don't bother.
		 */

		if (LF_ISSET(DB_ST_RELEN) && relenp)
			*relenp = pip->re_len;
	}
done:	if (F_ISSET(pip, VRFY_INCOMPLETE) && isbad == 0 && ret == 0) {
		/*
		 * During the page-by-page pass, item order verification was
		 * not finished due to the presence of overflow items.  If
		 * isbad == 0, though, it's now safe to do so, as we've
		 * traversed any child overflow pages.  Do it.
		 */
		if (h == NULL && (ret = __memp_fget(mpf, &pgno,
		    vdp->thread_info, NULL, 0, &h)) != 0)
			goto err;
		if ((ret = __bam_vrfy_itemorder(dbp,
		    vdp, vdp->thread_info, h, pgno, 0, 1, 0, flags)) != 0)
			goto err;
		F_CLR(pip, VRFY_INCOMPLETE);
	}

	/*
	 * It's possible to get to this point with a page that has no
	 * items, but without having detected any sort of failure yet.
	 * Having zero items is legal if it's a leaf--it may be the
	 * root page in an empty tree, or the tree may have been
	 * modified with the DB_REVSPLITOFF flag set (there's no way
	 * to tell from what's on disk).  For an internal page,
	 * though, having no items is a problem (all internal pages
	 * must have children).
	 */
	if (isbad == 0 && ret == 0) {
		if (h == NULL && (ret = __memp_fget(mpf, &pgno,
		    vdp->thread_info, NULL, 0, &h)) != 0)
			goto err;

		if (NUM_ENT(h) == 0 && ISINTERNAL(h)) {
			isbad = 1;
			EPRINT((env,
		    "Page %lu: internal page is empty and should not be",
			    (u_long)pgno));
			goto err;
		}
	}

	/*
	 * Our parent has sent us BINTERNAL pointers to parent records
	 * so that we can verify our place with respect to them.  If it's
	 * appropriate--we have a default sort function--verify this.
	 */
	if (isbad == 0 && ret == 0 && !LF_ISSET(DB_NOORDERCHK) &&
	    pip->type != P_IRECNO && pip->type != P_LRECNO) {
		if (h == NULL && (ret = __memp_fget(mpf, &pgno,
		    vdp->thread_info, NULL, 0, &h)) != 0)
			goto err;

		/*
		 * __bam_vrfy_treeorder needs to know what comparison function
		 * to use.  If DB_ST_DUPSET is set, we're in a duplicate tree
		 * and we use the duplicate comparison function;  otherwise,
		 * use the btree one.  If unset, use the default, of course.
		 */
		func = LF_ISSET(DB_ST_DUPSET) ? dbp->dup_compare :
		    ((BTREE *)dbp->bt_internal)->bt_compare;
		if (func == NULL)
			func = __bam_defcmp;

		if ((ret = __bam_vrfy_treeorder(dbp,
		    vdp->thread_info, h, l, r, func, flags)) != 0) {
			if (ret == DB_VERIFY_BAD)
				isbad = 1;
			else
				goto err;
		}
	}

	/*
	 * This is guaranteed to succeed for leaf pages, but no harm done.
	 *
	 * Internal pages below the top level do not store their own
	 * record numbers, so we skip them.
	 */
	if (LF_ISSET(DB_ST_RECNUM) && nrecs != pip->rec_cnt && toplevel) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: bad record count: has %lu records, claims %lu",
		    (u_long)pgno, (u_long)nrecs, (u_long)pip->rec_cnt));
	}

	if (levelp)
		*levelp = level;
	if (nrecsp)
		*nrecsp = nrecs;

	pgset = vdp->pgset;
	if ((ret = __db_vrfy_pgset_get(pgset,
	    vdp->thread_info, pgno, &p)) != 0)
		goto err;
	if (p != 0) {
		isbad = 1;
		EPRINT((env, "Page %lu: linked twice", (u_long)pgno));
	} else if ((ret =
	    __db_vrfy_pgset_inc(pgset, vdp->thread_info, pgno)) != 0)
		goto err;

	if (toplevel)
		/*
		 * The last page's next_pgno in the leaf chain should have been
		 * PGNO_INVALID.
		 */
		if (vdp->next_pgno != PGNO_INVALID) {
			isbad = 1;
			EPRINT((env, "Page %lu: unterminated leaf chain",
			    (u_long)vdp->prev_pgno));
		}

err:	if (toplevel) {
		/* Restore our caller's settings. */
		vdp->next_pgno = next_pgno;
		vdp->prev_pgno = prev_pgno;
		vdp->leaf_type = leaf_type;
	}

	if (h != NULL && (t_ret = __memp_fput(mpf,
	    vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (cc != NULL && ((t_ret = __db_vrfy_ccclose(cc)) != 0) && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __bam_vrfy_treeorder --
 *	Verify that the lowest key on a page sorts greater than the
 *	BINTERNAL which points to it (lp), and the highest key
 *	sorts less than the BINTERNAL above that (rp).
 *
 *	If lp is NULL, this means that it was the leftmost key on the
 *	parent, which (regardless of sort function) sorts less than
 *	all keys.  No need to check it.
 *
 *	If rp is NULL, lp was the highest key on the parent, so there's
 *	no higher key we must sort less than.
 */
static int
__bam_vrfy_treeorder(dbp, ip, h, lp, rp, func, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	PAGE *h;
	BINTERNAL *lp, *rp;
	int (*func) __P((DB *, const DBT *, const DBT *));
	u_int32_t flags;
{
	BOVERFLOW *bo;
	DBC *dbc;
	DBT dbt;
	ENV *env;
	db_indx_t last;
	int ret, cmp;

	env = dbp->env;
	memset(&dbt, 0, sizeof(DBT));
	F_SET(&dbt, DB_DBT_MALLOC);
	ret = 0;

	/*
	 * Empty pages are sorted correctly by definition.  We check
	 * to see whether they ought to be empty elsewhere;  leaf
	 * pages legally may be.
	 */
	if (NUM_ENT(h) == 0)
		return (0);

	switch (TYPE(h)) {
	case P_IBTREE:
	case P_LDUP:
		last = NUM_ENT(h) - O_INDX;
		break;
	case P_LBTREE:
		last = NUM_ENT(h) - P_INDX;
		break;
	default:
		return (__db_unknown_path(env, "__bam_vrfy_treeorder"));
	}

	/* Populate a dummy cursor. */
	if ((ret = __db_cursor_int(dbp, ip, NULL, DB_BTREE,
	    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &dbc)) != 0)
		return (ret);
	/*
	 * The key on page h, the child page, is more likely to be
	 * an overflow page, so we pass its offset, rather than lp/rp's,
	 * into __bam_cmp.  This will take advantage of __db_moff.
	 */

	/*
	 * Skip first-item check if we're an internal page--the first
	 * entry on an internal page is treated specially by __bam_cmp,
	 * so what's on the page shouldn't matter.  (Plus, since we're passing
	 * our page and item 0 as to __bam_cmp, we'll sort before our
	 * parent and falsely report a failure.)
	 */
	if (lp != NULL && TYPE(h) != P_IBTREE) {
		if ((ret = __db_cursor_int(dbp, ip, NULL, DB_BTREE,
		    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &dbc)) != 0)
			return (ret);
		if (lp->type == B_KEYDATA) {
			dbt.data = lp->data;
			dbt.size = lp->len;
		} else if (lp->type == B_OVERFLOW) {
			bo = (BOVERFLOW *)lp->data;
			if ((ret = __db_goff(dbc, &dbt,
			    bo->tlen, bo->pgno, NULL, NULL)) != 0)
				return (ret);
		} else
			return (
			    __db_unknown_path(env, "__bam_vrfy_treeorder"));

		/* On error, fall through, free if needed, and return. */
		if ((ret = __bam_cmp(dbc, &dbt, h, 0, func, &cmp)) == 0) {
			if (cmp > 0) {
				EPRINT((env,
	    "Page %lu: first item on page sorted greater than parent entry",
				    (u_long)PGNO(h)));
				ret = DB_VERIFY_BAD;
			}
		} else
			EPRINT((env,
			    "Page %lu: first item on page had comparison error",
			    (u_long)PGNO(h)));

		if (dbt.data != lp->data)
			__os_ufree(env, dbt.data);
		if (ret != 0)
			return (ret);
	}

	if (rp != NULL) {
		if (rp->type == B_KEYDATA) {
			dbt.data = rp->data;
			dbt.size = rp->len;
		} else if (rp->type == B_OVERFLOW) {
			bo = (BOVERFLOW *)rp->data;
			if ((ret = __db_goff(dbc, &dbt,
			    bo->tlen, bo->pgno, NULL, NULL)) != 0)
				return (ret);
		} else
			return (
			    __db_unknown_path(env, "__bam_vrfy_treeorder"));

		/* On error, fall through, free if needed, and return. */
		if ((ret = __bam_cmp(dbc, &dbt, h, last, func, &cmp)) == 0) {
			if (cmp < 0) {
				EPRINT((env,
	    "Page %lu: last item on page sorted greater than parent entry",
				    (u_long)PGNO(h)));
				ret = DB_VERIFY_BAD;
			}
		} else
			EPRINT((env,
			    "Page %lu: last item on page had comparison error",
			    (u_long)PGNO(h)));

		if (dbt.data != rp->data)
			__os_ufree(env, dbt.data);
	}

	return (ret);
}

/*
 * __bam_salvage --
 *	Safely dump out anything that looks like a key on an alleged
 *	btree leaf page, also mark overflow pages as seen.  For internal btree
 *	pages, just mark any overflow pages as seen.
 *
 * PUBLIC: int __bam_salvage __P((DB *, VRFY_DBINFO *,
 * PUBLIC:     db_pgno_t, u_int32_t, PAGE *, void *,
 * PUBLIC:     int (*)(void *, const void *), DBT *, u_int32_t));
 */
int
__bam_salvage(dbp, vdp, pgno, pgtype, h, handle, callback, key, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	u_int32_t pgtype;
	PAGE *h;
	void *handle;
	int (*callback) __P((void *, const void *));
	DBT *key;
	u_int32_t flags;
{
	BKEYDATA *bk;
	BOVERFLOW *bo;
	DBT dbt, repldbt, unknown_key, unknown_data;
	ENV *env;
	VRFY_ITEM *pgmap;
	db_indx_t i, last, beg, end, *inp;
	db_pgno_t ovflpg;
	u_int32_t himark, ovfl_bufsz;
	void *ovflbuf;
	int adj, ret, t_ret, t2_ret;
#ifdef HAVE_COMPRESSION
	DBT kcpy, *last_key;
	int unknown_dup_key;
#endif

	env = dbp->env;
	ovflbuf = pgmap = NULL;
	inp = P_INP(dbp, h);

	memset(&dbt, 0, sizeof(DBT));
	dbt.flags = DB_DBT_REALLOC;
	memset(&repldbt, 0, sizeof(DBT));

#ifdef HAVE_COMPRESSION
	memset(&kcpy, 0, sizeof(DBT));
	unknown_dup_key = LF_ISSET(DB_SA_UNKNOWNKEY);
	last_key = unknown_dup_key ? NULL : key;
#endif
	LF_CLR(DB_SA_UNKNOWNKEY);

	DB_INIT_DBT(unknown_key, "UNKNOWN_KEY", sizeof("UNKNOWN_KEY") - 1);
	DB_INIT_DBT(unknown_data, "UNKNOWN_DATA", sizeof("UNKNOWN_DATA") - 1);

	/*
	 * Allocate a buffer for overflow items.  Start at one page;
	 * __db_safe_goff will realloc as needed.
	 */
	if ((ret = __os_malloc(env, dbp->pgsize, &ovflbuf)) != 0)
		goto err;
	ovfl_bufsz = dbp->pgsize;

	if (LF_ISSET(DB_AGGRESSIVE) && (ret =
	    __os_calloc(env, dbp->pgsize, sizeof(pgmap[0]), &pgmap)) != 0)
		goto err;

	/*
	 * Loop through the inp array, spitting out key/data pairs.
	 *
	 * If we're salvaging normally, loop from 0 through NUM_ENT(h).  If
	 * we're being aggressive, loop until we hit the end of the page --
	 * NUM_ENT() may be bogus.
	 */
	himark = dbp->pgsize;
	for (i = 0, last = UINT16_MAX;; i += O_INDX) {
		/*
		 * If we're not aggressive, or if we're on an internal page,
		 * break when we hit NUM_ENT(h).
		 */
		if ((!LF_ISSET(DB_AGGRESSIVE) ||
		    pgtype == P_IBTREE) && i >= NUM_ENT(h))
			break;

		/* Verify the current item. */
		t_ret =
		    __db_vrfy_inpitem(dbp, h, pgno, i, 1, flags, &himark, NULL);

		if (t_ret != 0) {
			/*
			 * If this is a btree leaf and we've printed out a key
			 * but not its associated data item, fix this imbalance
			 * by printing an "UNKNOWN_DATA".
			 */
			if (pgtype == P_LBTREE && i % P_INDX == 1 &&
			    last == i - 1 && (t2_ret = __db_vrfy_prdbt(
			    &unknown_data,
			    0, " ", handle, callback, 0, vdp)) != 0) {
				if (ret == 0)
					ret = t2_ret;
				goto err;
			}

			/*
			 * Don't return DB_VERIFY_FATAL; it's private and means
			 * only that we can't go on with this page, not with
			 * the whole database.  It's not even an error if we've
			 * run into it after NUM_ENT(h).
			 */
			if (t_ret == DB_VERIFY_FATAL) {
				if (i < NUM_ENT(h) && ret == 0)
					ret = DB_VERIFY_BAD;
				break;
			}
			continue;
		}

		/*
		 * If this returned 0, it's safe to print or (carefully)
		 * try to fetch.
		 *
		 * We only print deleted items if DB_AGGRESSIVE is set.
		 */
		bk = GET_BKEYDATA(dbp, h, i);
		if (!LF_ISSET(DB_AGGRESSIVE) && B_DISSET(bk->type))
			continue;

		/*
		 * If this is a btree leaf and we're about to print out a data
		 * item for which we didn't print out a key, fix this imbalance
		 * by printing an "UNKNOWN_KEY".
		 */
		if (pgtype == P_LBTREE && i % P_INDX == 1 && last != i - 1) {
#ifdef HAVE_COMPRESSION
			last_key = NULL;
#endif
			if ((t_ret = __db_vrfy_prdbt(&unknown_key,
			    0, " ", handle, callback, 0, vdp)) != 0) {
				if (ret == 0)
					ret = t_ret;
				goto err;
			}
		}
		last = i;

		/*
		 * We're going to go try to print the next item.  If key is
		 * non-NULL, we're a dup page, so we've got to print the key
		 * first, unless DB_SA_SKIPFIRSTKEY is set and we're on the
		 * first entry.
		 */
		if (key != NULL && (i != 0 || !LF_ISSET(DB_SA_SKIPFIRSTKEY))) {
#ifdef HAVE_COMPRESSION
			last_key = unknown_dup_key ? NULL : key;
#endif
			if ((t_ret = __db_vrfy_prdbt(key,
			    0, " ", handle, callback, 0, vdp)) != 0) {
				if (ret == 0)
					ret = t_ret;
				goto err;
			}
		}

		beg = end = inp[i];
		switch (B_TYPE(bk->type)) {
		case B_DUPLICATE:
			if (pgtype == P_IBTREE)
				break;

			end = beg + BOVERFLOW_SIZE - 1;
			/*
			 * If we're not on a normal btree leaf page, there
			 * shouldn't be off-page dup sets.  Something's
			 * confused; just drop it, and the code to pick up
			 * unlinked offpage dup sets will print it out
			 * with key "UNKNOWN" later.
			 */
			if (pgtype != P_LBTREE)
				break;

			bo = (BOVERFLOW *)bk;

			/*
			 * If the page number is unreasonable, or if this is
			 * supposed to be a key item, output "UNKNOWN_KEY" --
			 * the best we can do is run into the data items in
			 * the unlinked offpage dup pass.
			 */
			if (!IS_VALID_PGNO(bo->pgno) || (i % P_INDX == 0)) {
				/* Not much to do on failure. */
#ifdef HAVE_COMPRESSION
				if (key == NULL && i % P_INDX == 0)
					last_key = NULL;
#endif
				if ((t_ret = __db_vrfy_prdbt(
				 i % P_INDX == 0 ? &unknown_key : &unknown_data,
				    0, " ", handle, callback, 0, vdp)) != 0) {
					if (ret == 0)
						ret = t_ret;
					goto err;
				}
				break;
			}

			/* Don't stop on error. */
			if ((t_ret = __db_salvage_duptree(dbp,
			    vdp, bo->pgno, &dbt, handle, callback,
			    flags | DB_SA_SKIPFIRSTKEY
#ifdef HAVE_COMPRESSION
			    | (last_key == NULL ? DB_SA_UNKNOWNKEY : 0)
#endif
			    )) != 0 && ret == 0)
				ret = t_ret;

			break;
		case B_KEYDATA:
			if (pgtype == P_IBTREE)
				break;

			end = (db_indx_t)DB_ALIGN(
			    beg + bk->len, sizeof(u_int32_t)) - 1;

			dbt.data = bk->data;
			dbt.size = bk->len;

#ifdef HAVE_COMPRESSION
			if (DB_IS_COMPRESSED(dbp) && last_key != NULL &&
			    (key != NULL || (i % P_INDX == 1))) {
				/* Decompress the key/data pair  - the key
				   is in last_key, and the data is in dbt */
				if ((t_ret = __bam_compress_salvage(dbp, vdp,
				    handle, callback, last_key, &dbt)) != 0) {
					if (t_ret == DB_VERIFY_FATAL) {
						if (ret == 0)
							ret = DB_VERIFY_BAD;
						if (!LF_ISSET(DB_AGGRESSIVE))
							goto err;
					} else if (ret == 0) {
						ret = t_ret;
						goto err;
					}
				}
			} else {
				if (key == NULL && i % P_INDX == 0) {
					if ((ret = __os_realloc(
					    env, dbt.size, &kcpy.data)) != 0)
						goto err;
					memcpy(kcpy.data, dbt.data, dbt.size);
					kcpy.size = dbt.size;
					last_key = &kcpy;
				}
#endif

				if ((t_ret = __db_vrfy_prdbt(&dbt,
				    0, " ", handle, callback, 0, vdp)) != 0) {
					if (ret == 0)
						ret = t_ret;
					goto err;
				}
#ifdef HAVE_COMPRESSION
			}
#endif
			break;
		case B_OVERFLOW:
			if (pgtype != P_IBTREE)
				end = beg + BOVERFLOW_SIZE - 1;
			bo = (BOVERFLOW *)bk;

			/*
			 * Check for replicated overflow keys, so that we only
			 * call __db_safe_goff once per overflow page.  If we
			 * get the same offset as the previous key just re-use
			 * the previous dbt.
			 *
			 * P_IBTREE pages will never have replicated overflow
			 * keys.
			 */
			adj = pgtype == P_IBTREE ? O_INDX : P_INDX;
			if (pgtype == P_IBTREE) {
				/*
				 * If we're looking at a P_IBTREE, we just want
				 * to mark the overflow page as seen.
				 *
				 * Note that this call to __db_safe_goff differs
				 * from the non-P_IBTREE call.
				 *
				 * Only call __db_safe_goff if the overflow page
				 * hasn't been seen.
				 */
				ovflpg = ((BOVERFLOW *)
				    ((BINTERNAL *)bk)->data)->pgno;
				if (__db_salvage_isdone(vdp, ovflpg) == 0 &&
				    (t_ret =__db_safe_goff(dbp, vdp, ovflpg,
					&dbt, &ovflbuf,
					&ovfl_bufsz, flags)) != 0 && ret == 0)
					ret = t_ret;
				break;
			} else if (i > adj - 1 &&
			    i % adj == 0 && inp[i] == inp[i - adj])
				dbt = repldbt;
			else {
				/* Don't stop on error. */
				if ((t_ret = __db_safe_goff(dbp, vdp,
				    bo->pgno, &dbt, &ovflbuf,
				    &ovfl_bufsz, flags)) != 0 && ret == 0)
					ret = t_ret;

				/*
				 * If this is a key, save it in case the next
				 * key is a replicated overflow, so we don't
				 * call __db_safe_goff again.  Copy out dbt.data
				 * in case that pointer gets realloc'd when
				 * getting a data item.
				 */
				if (i % P_INDX == 0) {
					if (t_ret == 0) {
						if ((t_ret = __os_realloc(env,
						        dbt.size,
							&repldbt.data)) != 0) {
							if (ret == 0)
								ret = t_ret;
							goto err;
						}
						memcpy(repldbt.data,
						    dbt.data, dbt.size);
						repldbt.size = dbt.size;
					} else {
						if (__os_realloc(env,
						    unknown_key.size,
						    &repldbt.data) != 0)
							goto err;
						memcpy(repldbt.data,
						    unknown_key.data,
						    unknown_key.size);
						repldbt.size = unknown_key.size;
					}
				}

			}

#ifdef HAVE_COMPRESSION
			if (DB_IS_COMPRESSED(dbp) && last_key && t_ret == 0 &&
			    (key != NULL || (i % P_INDX == 1))) {
				/* Decompress the key/data pair  - the key
				   is in last_key, and the data is in dbt */
				if ((t_ret = __bam_compress_salvage(dbp, vdp,
				    handle, callback, last_key, &dbt)) != 0) {
					if (t_ret == DB_VERIFY_FATAL) {
						if (ret == 0)
							ret = DB_VERIFY_BAD;
						if (!LF_ISSET(DB_AGGRESSIVE))
							goto err;
					} else if (ret == 0) {
						ret = t_ret;
						goto err;
					}
				}
			} else {
				if (key == NULL && i % P_INDX == 0) {
					if (t_ret == 0) {
						if ((ret = __os_realloc(env,
						    dbt.size, &kcpy.data)) != 0)
							goto err;
						memcpy(kcpy.data, dbt.data,
							dbt.size);
						kcpy.size = dbt.size;
						last_key = &kcpy;
					} else
						last_key = NULL;
				}
#endif

				if ((t_ret = __db_vrfy_prdbt(
					     t_ret == 0 ? &dbt : &unknown_key,
					     0, " ", handle, callback, 0, vdp))
					!= 0 && ret == 0)
					ret = t_ret;
#ifdef HAVE_COMPRESSION
			}
#endif
			break;
		default:
			/*
			 * We should never get here; __db_vrfy_inpitem should
			 * not be returning 0 if bk->type is unrecognizable.
			 */
			t_ret = __db_unknown_path(env, "__bam_salvage");
			if (ret == 0)
				ret = t_ret;
			goto err;
		}

		/*
		 * If we're being aggressive, mark the beginning and end of
		 * the item; we'll come back and print whatever "junk" is in
		 * the gaps in case we had any bogus inp elements and thereby
		 * missed stuff.
		 */
		if (LF_ISSET(DB_AGGRESSIVE) && pgtype != P_IBTREE) {
			pgmap[beg] = VRFY_ITEM_BEGIN;
			pgmap[end] = VRFY_ITEM_END;
		}
	}

err:	if (pgmap != NULL)
		__os_free(env, pgmap);
	if (ovflbuf != NULL)
		__os_free(env, ovflbuf);
	if (repldbt.data != NULL)
		__os_free(env, repldbt.data);
#ifdef HAVE_COMPRESSION
	if (kcpy.data != NULL)
		__os_free(env, kcpy.data);
#endif

	/* Mark this page as done. */
	if ((t_ret = __db_salvage_markdone(vdp, pgno)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __bam_salvage_walkdupint --
 *	Walk a known-good btree or recno internal page which is part of
 *	a dup tree, calling __db_salvage_duptree on each child page.
 *
 * PUBLIC: int __bam_salvage_walkdupint __P((DB *, VRFY_DBINFO *, PAGE *,
 * PUBLIC:     DBT *, void *, int (*)(void *, const void *), u_int32_t));
 */
int
__bam_salvage_walkdupint(dbp, vdp, h, key, handle, callback, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	DBT *key;
	void *handle;
	int (*callback) __P((void *, const void *));
	u_int32_t flags;
{
	BINTERNAL *bi;
	ENV *env;
	RINTERNAL *ri;
	int ret, t_ret;
	db_indx_t i;

	env = dbp->env;
	ret = 0;

	for (i = 0; i < NUM_ENT(h); i++) {
		switch (TYPE(h)) {
		case P_IBTREE:
			bi = GET_BINTERNAL(dbp, h, i);
			if ((t_ret = __db_salvage_duptree(dbp,
			    vdp, bi->pgno, key, handle, callback, flags)) != 0)
				ret = t_ret;
			break;
		case P_IRECNO:
			ri = GET_RINTERNAL(dbp, h, i);
			if ((t_ret = __db_salvage_duptree(dbp,
			    vdp, ri->pgno, key, handle, callback, flags)) != 0)
				ret = t_ret;
			break;
		default:
			return (__db_unknown_path(
			    env, "__bam_salvage_walkdupint"));
		}
		/* Pass DB_SA_SKIPFIRSTKEY, if set, on to the 0th child only. */
		flags &= ~LF_ISSET(DB_SA_SKIPFIRSTKEY);
	}

	return (ret);
}

/*
 * __bam_meta2pgset --
 *	Given a known-good meta page, return in pgsetp a 0-terminated list of
 *	db_pgno_t's corresponding to the pages in the btree.
 *
 *	We do this by a somewhat sleazy method, to avoid having to traverse the
 *	btree structure neatly:  we walk down the left side to the very
 *	first leaf page, then we mark all the pages in the chain of
 *	NEXT_PGNOs (being wary of cycles and invalid ones), then we
 *	consolidate our scratch array into a nice list, and return.  This
 *	avoids the memory management hassles of recursion and the
 *	trouble of walking internal pages--they just don't matter, except
 *	for the left branch.
 *
 * PUBLIC: int __bam_meta2pgset __P((DB *, VRFY_DBINFO *, BTMETA *,
 * PUBLIC:     u_int32_t, DB *));
 */
int
__bam_meta2pgset(dbp, vdp, btmeta, flags, pgset)
	DB *dbp;
	VRFY_DBINFO *vdp;
	BTMETA *btmeta;
	u_int32_t flags;
	DB *pgset;
{
	BINTERNAL *bi;
	DB_MPOOLFILE *mpf;
	PAGE *h;
	RINTERNAL *ri;
	db_pgno_t current, p;
	int err_ret, ret;

	DB_ASSERT(dbp->env, pgset != NULL);

	mpf = dbp->mpf;
	h = NULL;
	ret = err_ret = 0;

	for (current = btmeta->root;;) {
		if (!IS_VALID_PGNO(current) || current == PGNO(btmeta)) {
			err_ret = DB_VERIFY_BAD;
			goto err;
		}
		if ((ret = __memp_fget(mpf, &current,
		     vdp->thread_info, NULL, 0, &h)) != 0) {
			err_ret = ret;
			goto err;
		}

		switch (TYPE(h)) {
		case P_IBTREE:
		case P_IRECNO:
			if ((ret = __bam_vrfy(dbp,
			    vdp, h, current, flags | DB_NOORDERCHK)) != 0) {
				err_ret = ret;
				goto err;
			}
			if (TYPE(h) == P_IBTREE) {
				bi = GET_BINTERNAL(dbp, h, 0);
				current = bi->pgno;
			} else {	/* P_IRECNO */
				ri = GET_RINTERNAL(dbp, h, 0);
				current = ri->pgno;
			}
			break;
		case P_LBTREE:
		case P_LRECNO:
			goto traverse;
		default:
			err_ret = DB_VERIFY_BAD;
			goto err;
		}

		if ((ret = __memp_fput(mpf,
		     vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0)
			err_ret = ret;
		h = NULL;
	}

	/*
	 * At this point, current is the pgno of leaf page h, the 0th in the
	 * tree we're concerned with.
	 */
traverse:
	while (IS_VALID_PGNO(current) && current != PGNO_INVALID) {
		if (h == NULL && (ret = __memp_fget(mpf,
		    &current, vdp->thread_info, NULL, 0, &h)) != 0) {
			err_ret = ret;
			break;
		}

		if ((ret = __db_vrfy_pgset_get(pgset,
		    vdp->thread_info, current, (int *)&p)) != 0)
			goto err;

		if (p != 0) {
			/*
			 * We've found a cycle.  Return success anyway--
			 * our caller may as well use however much of
			 * the pgset we've come up with.
			 */
			break;
		}
		if ((ret =
		    __db_vrfy_pgset_inc(pgset, vdp->thread_info, current)) != 0)
			goto err;

		current = NEXT_PGNO(h);
		if ((ret = __memp_fput(mpf,
		     vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0)
			err_ret = ret;
		h = NULL;
	}

err:	if (h != NULL)
		(void)__memp_fput(mpf,
		    vdp->thread_info, h, DB_PRIORITY_UNCHANGED);

	return (ret == 0 ? err_ret : ret);
}

/*
 * __bam_safe_getdata --
 *
 *	Utility function for __bam_vrfy_itemorder.  Safely gets the datum at
 *	index i, page h, and sticks it in DBT dbt.  If ovflok is 1 and i's an
 *	overflow item, we do a safe_goff to get the item and signal that we need
 *	to free dbt->data;  if ovflok is 0, we leaves the DBT zeroed.
 */
static int
__bam_safe_getdata(dbp, ip, h, i, ovflok, dbt, freedbtp)
	DB *dbp;
	DB_THREAD_INFO *ip;
	PAGE *h;
	u_int32_t i;
	int ovflok;
	DBT *dbt;
	int *freedbtp;
{
	BKEYDATA *bk;
	BOVERFLOW *bo;
	DBC *dbc;
	int ret;

	memset(dbt, 0, sizeof(DBT));
	*freedbtp = 0;

	bk = GET_BKEYDATA(dbp, h, i);
	if (B_TYPE(bk->type) == B_OVERFLOW) {
		if (!ovflok)
			return (0);

		if ((ret = __db_cursor_int(dbp, ip, NULL, DB_BTREE,
		    PGNO_INVALID, 0, DB_LOCK_INVALIDID, &dbc)) != 0)
			return (ret);
		bo = (BOVERFLOW *)bk;
		F_SET(dbt, DB_DBT_MALLOC);

		*freedbtp = 1;
		return (__db_goff(dbc, dbt, bo->tlen, bo->pgno, NULL, NULL));
	} else {
		dbt->data = bk->data;
		dbt->size = bk->len;
	}

	return (0);
}
