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
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
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
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/db_verify.h"
#include "dbinc/mp.h"

/*
 * __db_vrfy_overflow --
 *	Verify overflow page.
 *
 * PUBLIC: int __db_vrfy_overflow __P((DB *, VRFY_DBINFO *, PAGE *, db_pgno_t,
 * PUBLIC:     u_int32_t));
 */
int
__db_vrfy_overflow(dbp, vdp, h, pgno, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	PAGE *h;
	db_pgno_t pgno;
	u_int32_t flags;
{
	VRFY_PAGEINFO *pip;
	int isbad, ret, t_ret;

	isbad = 0;
	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	if ((ret = __db_vrfy_datapage(dbp, vdp, h, pgno, flags)) != 0) {
		if (ret == DB_VERIFY_BAD)
			isbad = 1;
		else
			goto err;
	}

	pip->refcount = OV_REF(h);
	if (pip->refcount < 1) {
		EPRINT((dbp->env,
		    "Page %lu: overflow page has zero reference count",
		    (u_long)pgno));
		isbad = 1;
	}

	/* Just store for now. */
	pip->olen = HOFFSET(h);

err:	if ((t_ret = __db_vrfy_putpageinfo(dbp->env, vdp, pip)) != 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_vrfy_ovfl_structure --
 *	Walk a list of overflow pages, avoiding cycles and marking
 *	pages seen.
 *
 * PUBLIC: int __db_vrfy_ovfl_structure
 * PUBLIC:     __P((DB *, VRFY_DBINFO *, db_pgno_t, u_int32_t, u_int32_t));
 */
int
__db_vrfy_ovfl_structure(dbp, vdp, pgno, tlen, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	u_int32_t tlen;
	u_int32_t flags;
{
	DB *pgset;
	ENV *env;
	VRFY_PAGEINFO *pip;
	db_pgno_t next, prev;
	int isbad, ret, seen_cnt, t_ret;
	u_int32_t refcount;

	env = dbp->env;
	pgset = vdp->pgset;
	DB_ASSERT(env, pgset != NULL);
	isbad = 0;

	/* This shouldn't happen, but just to be sure. */
	if (!IS_VALID_PGNO(pgno))
		return (DB_VERIFY_BAD);

	/*
	 * Check the first prev_pgno;  it ought to be PGNO_INVALID,
	 * since there's no prev page.
	 */
	if ((ret = __db_vrfy_getpageinfo(vdp, pgno, &pip)) != 0)
		return (ret);

	/* The refcount is stored on the first overflow page. */
	refcount = pip->refcount;

	if (pip->type != P_OVERFLOW) {
		EPRINT((env,
		    "Page %lu: overflow page of invalid type %lu",
		    (u_long)pgno, (u_long)pip->type));
		ret = DB_VERIFY_BAD;
		goto err;		/* Unsafe to continue. */
	}

	prev = pip->prev_pgno;
	if (prev != PGNO_INVALID) {
		EPRINT((env,
	    "Page %lu: first page in overflow chain has a prev_pgno %lu",
		    (u_long)pgno, (u_long)prev));
		isbad = 1;
	}

	for (;;) {
		/*
		 * We may have seen this page elsewhere, if the overflow entry
		 * has been promoted to an internal page;  we just want to
		 * make sure that each overflow page is seen exactly as many
		 * times as its refcount dictates.
		 *
		 * Note that this code also serves to keep us from looping
		 * infinitely if there's a cycle in an overflow chain.
		 */
		if ((ret = __db_vrfy_pgset_get(pgset,
		    vdp->thread_info, pgno, &seen_cnt)) != 0)
			goto err;
		if ((u_int32_t)seen_cnt > refcount) {
			EPRINT((env,
		"Page %lu: encountered too many times in overflow traversal",
			    (u_long)pgno));
			ret = DB_VERIFY_BAD;
			goto err;
		}
		if ((ret =
		    __db_vrfy_pgset_inc(pgset, vdp->thread_info, pgno)) != 0)
			goto err;

		/*
		 * Each overflow page can be referenced multiple times,
		 * because it's possible for overflow Btree keys to get
		 * promoted to internal pages.  We want to make sure that
		 * each page is referenced from a Btree leaf (or Hash data
		 * page, which we consider a "leaf" here) exactly once; if
		 * the parent was a leaf, set a flag to indicate that we've
		 * seen this page in a leaf context.
		 *
		 * If the parent is not a leaf--in which case it's a Btree
		 * internal page--we don't need to bother doing any further
		 * verification, as we'll do it when we hit the leaf (or
		 * complain that we never saw the leaf).  Only the first
		 * page in an overflow chain should ever have a refcount
		 * greater than 1, and the combination of the LEAFSEEN check
		 * and the fact that we bail after the first page for
		 * non-leaves should ensure this.
		 *
		 * Note that each "child" of a page, such as an overflow page,
		 * is stored and verified in a structure check exactly once,
		 * so this code does not need to contend with the fact that
		 * overflow chains used as Btree duplicate keys may be
		 * referenced multiply from a single Btree leaf page.
		 */
		if (LF_ISSET(DB_ST_OVFL_LEAF)) {
			if (F_ISSET(pip, VRFY_OVFL_LEAFSEEN)) {
				EPRINT((env,
		"Page %lu: overflow page linked twice from leaf or data page",
				    (u_long)pgno));
				ret = DB_VERIFY_BAD;
				goto err;
			}
			F_SET(pip, VRFY_OVFL_LEAFSEEN);
		}

		/*
		 * We want to verify each overflow chain only once, and
		 * although no chain should be linked more than once from a
		 * leaf page, we can't guarantee that it'll be linked that
		 * once if it's linked from an internal page and the key
		 * is gone.
		 *
		 * seen_cnt is the number of times we'd encountered this page
		 * before calling this function.
		 */
		if (seen_cnt == 0) {
			/*
			 * Keep a running tab on how much of the item we've
			 * seen.
			 */
			tlen -= pip->olen;

			/* Send the application feedback about our progress. */
			if (!LF_ISSET(DB_SALVAGE))
				__db_vrfy_struct_feedback(dbp, vdp);
		} else
			goto done;

		next = pip->next_pgno;

		/* Are we there yet? */
		if (next == PGNO_INVALID)
			break;

		/*
		 * We've already checked this when we saved it, but just
		 * to be sure...
		 */
		if (!IS_VALID_PGNO(next)) {
			EPRINT((env,
			    "Page %lu: bad next_pgno %lu on overflow page",
			    (u_long)pgno, (u_long)next));
			ret = DB_VERIFY_BAD;
			goto err;
		}

		if ((ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 ||
		    (ret = __db_vrfy_getpageinfo(vdp, next, &pip)) != 0)
			return (ret);
		if (pip->prev_pgno != pgno) {
			EPRINT((env,
		"Page %lu: bad prev_pgno %lu on overflow page (should be %lu)",
			    (u_long)next, (u_long)pip->prev_pgno,
			    (u_long)pgno));
			isbad = 1;
			/*
			 * It's safe to continue because we have separate
			 * cycle detection.
			 */
		}

		pgno = next;
	}

	if (tlen > 0) {
		isbad = 1;
		EPRINT((env,
		    "Page %lu: overflow item incomplete", (u_long)pgno));
	}

done:
err:	if ((t_ret =
	    __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	return ((ret == 0 && isbad == 1) ? DB_VERIFY_BAD : ret);
}

/*
 * __db_safe_goff --
 *	Get an overflow item, very carefully, from an untrusted database,
 *	in the context of the salvager.
 *
 * PUBLIC: int __db_safe_goff __P((DB *, VRFY_DBINFO *,
 * PUBLIC:      db_pgno_t, DBT *, void *, u_int32_t *, u_int32_t));
 */
int
__db_safe_goff(dbp, vdp, pgno, dbt, buf, bufsz, flags)
	DB *dbp;
	VRFY_DBINFO *vdp;
	db_pgno_t pgno;
	DBT *dbt;
	void *buf;
	u_int32_t *bufsz;
	u_int32_t flags;
{
	DB_MPOOLFILE *mpf;
	PAGE *h;
	int ret, t_ret;
	u_int32_t bytesgot, bytes;
	u_int8_t *src, *dest;

	mpf = dbp->mpf;
	h = NULL;
	ret = t_ret = 0;
	bytesgot = bytes = 0;

    DB_ASSERT(dbp->env, bufsz != NULL);

	/*
	 * Back up to the start of the overflow chain (if necessary) via the
	 * prev pointer of the overflow page.  This guarantees we transverse the
	 * longest possible chains of overflow pages and won't be called again
	 * with a pgno earlier in the chain, stepping on ourselves.
	 */
	for (;;) {
		if ((ret = __memp_fget(
		    mpf, &pgno, vdp->thread_info, NULL, 0, &h)) != 0)
			return (ret);

		if (PREV_PGNO(h) == PGNO_INVALID ||
		    !IS_VALID_PGNO(PREV_PGNO(h)))
			break;

		pgno = PREV_PGNO(h);

		if ((ret = __memp_fput(mpf,
		    vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0)
			return (ret);
	}
	if ((ret = __memp_fput(
	    mpf, vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0)
		return (ret);

	h = NULL;

	while ((pgno != PGNO_INVALID) && (IS_VALID_PGNO(pgno))) {
		/*
		 * Mark that we're looking at this page;  if we've seen it
		 * already, quit.
		 */
		if ((ret = __db_salvage_markdone(vdp, pgno)) != 0)
			break;

		if ((ret = __memp_fget(mpf, &pgno,
		    vdp->thread_info, NULL, 0, &h)) != 0)
			break;

		/*
		 * Make sure it's really an overflow page, unless we're
		 * being aggressive, in which case we pretend it is.
		 */
		if (!LF_ISSET(DB_AGGRESSIVE) && TYPE(h) != P_OVERFLOW) {
			ret = DB_VERIFY_BAD;
			break;
		}

		src = (u_int8_t *)h + P_OVERHEAD(dbp);
		bytes = OV_LEN(h);

		if (bytes + P_OVERHEAD(dbp) > dbp->pgsize)
			bytes = dbp->pgsize - P_OVERHEAD(dbp);

		/*
		 * Realloc if buf is too small
		 */
		if (bytesgot + bytes > *bufsz) {
			if ((ret =
			    __os_realloc(dbp->env, bytesgot + bytes, buf)) != 0)
				break;
			*bufsz = bytesgot + bytes;
		}

		dest = *(u_int8_t **)buf + bytesgot;
		bytesgot += bytes;

		memcpy(dest, src, bytes);

		pgno = NEXT_PGNO(h);

		if ((ret = __memp_fput(mpf,
		     vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0)
			break;
		h = NULL;
	}

	/*
	 * If we're being aggressive, salvage a partial datum if there
	 * was an error somewhere along the way.
	 */
	if (ret == 0 || LF_ISSET(DB_AGGRESSIVE)) {
		dbt->size = bytesgot;
		dbt->data = *(void **)buf;
	}

	/* If we broke out on error, don't leave pages pinned. */
	if (h != NULL && (t_ret = __memp_fput(mpf,
	    vdp->thread_info, h, DB_PRIORITY_UNCHANGED)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
