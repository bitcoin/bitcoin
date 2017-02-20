/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"

static int __memp_trickle __P((ENV *, int, int *));

/*
 * __memp_trickle_pp --
 *	ENV->memp_trickle pre/post processing.
 *
 * PUBLIC: int __memp_trickle_pp __P((DB_ENV *, int, int *));
 */
int
__memp_trickle_pp(dbenv, pct, nwrotep)
	DB_ENV *dbenv;
	int pct, *nwrotep;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->mp_handle, "memp_trickle", DB_INIT_MPOOL);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_trickle(env, pct, nwrotep)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_trickle --
 *	ENV->memp_trickle.
 */
static int
__memp_trickle(env, pct, nwrotep)
	ENV *env;
	int pct, *nwrotep;
{
	DB_MPOOL *dbmp;
	MPOOL *c_mp, *mp;
	u_int32_t clean, dirty, i, need_clean, total, dtmp, wrote;
	int ret;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;

	if (nwrotep != NULL)
		*nwrotep = 0;

	if (pct < 1 || pct > 100) {
		__db_errx(env,
	    "DB_ENV->memp_trickle: %d: percent must be between 1 and 100",
		    pct);
		return (EINVAL);
	}

	/*
	 * Loop through the caches counting total/dirty buffers.
	 *
	 * XXX
	 * Using hash_page_dirty is our only choice at the moment, but it's not
	 * as correct as we might like in the presence of pools having more
	 * than one page size, as a free 512B buffer may not be equivalent to
	 * having a free 8KB buffer.
	 */
	for (ret = 0, i = dirty = total = 0; i < mp->nreg; ++i) {
		c_mp = dbmp->reginfo[i].primary;
		total += c_mp->stat.st_pages;
		__memp_stat_hash(&dbmp->reginfo[i], c_mp, &dtmp);
		dirty += dtmp;
	}

	/*
	 * If there are sufficient clean buffers, no buffers or no dirty
	 * buffers, we're done.
	 */
	if (total == 0 || dirty == 0)
		return (0);

	/*
	 * The total number of pages is an exact number, but the dirty page
	 * count can change while we're walking the hash buckets, and it's
	 * even possible the dirty page count ends up larger than the total
	 * number of pages.
	 */
	clean = total > dirty ? total - dirty : 0;
	need_clean = (total * (u_int)pct) / 100;
	if (clean >= need_clean)
		return (0);

	need_clean -= clean;
	ret = __memp_sync_int(env, NULL,
	    need_clean, DB_SYNC_TRICKLE | DB_SYNC_INTERRUPT_OK, &wrote, NULL);
	STAT((mp->stat.st_page_trickle += wrote));
	if (nwrotep != NULL)
		*nwrotep = (int)wrote;

	return (ret);
}
