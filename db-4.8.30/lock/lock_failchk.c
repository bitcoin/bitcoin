/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/lock.h"
#include "dbinc/txn.h"

/*
 * __lock_failchk --
 *	Check for locks held by dead threads of control and release
 *	read locks.  If any write locks were held by dead non-trasnactional
 *	lockers then we must abort and run recovery.  Otherwise we release
 *	read locks for lockers owned by dead threads.  Write locks for
 *	dead transactional lockers will be freed when we abort the transaction.
 *
 * PUBLIC: int __lock_failchk __P((ENV *));
 */
int
__lock_failchk(env)
	ENV *env;
{
	DB_ENV *dbenv;
	DB_LOCKER *lip;
	DB_LOCKREGION *lrp;
	DB_LOCKREQ request;
	DB_LOCKTAB *lt;
	u_int32_t i;
	int ret;
	char buf[DB_THREADID_STRLEN];

	dbenv = env->dbenv;
	lt = env->lk_handle;
	lrp = lt->reginfo.primary;

retry:	LOCK_LOCKERS(env, lrp);

	ret = 0;
	for (i = 0; i < lrp->locker_t_size; i++)
		SH_TAILQ_FOREACH(lip, &lt->locker_tab[i], links, __db_locker) {
			/*
			 * If the locker is transactional, we can ignore it if
			 * it has no read locks or has no locks at all.  Check
			 * the heldby list rather then nlocks since a lock may
			 * be PENDING.  __txn_failchk aborts any transactional
			 * lockers.  Non-transactional lockers progress to  
			 * is_alive test.
			 */
			if ((lip->id >= TXN_MINIMUM) &&
			     (SH_LIST_EMPTY(&lip->heldby) ||
			     lip->nlocks == lip->nwrites))
				continue;

			/* If the locker is still alive, it's not a problem. */
			if (dbenv->is_alive(dbenv, lip->pid, lip->tid, 0))
				continue;

			/*
			 * We can only deal with read locks.  If a
			 * non-transactional locker holds write locks we
			 * have to assume a Berkeley DB operation was
			 * interrupted with only 1-of-N pages modified.
			 */
			if (lip->id < TXN_MINIMUM && lip->nwrites != 0) {
				ret = __db_failed(env,
				     "locker has write locks",
				     lip->pid, lip->tid);
				break;
			}

			/*
			 * Discard the locker and its read locks.
			 */
			if (!SH_LIST_EMPTY(&lip->heldby)) {
				__db_msg(env, 
				    "Freeing read locks for locker %#lx: %s",
			    	    (u_long)lip->id, dbenv->thread_id_string(
			    	    dbenv, lip->pid, lip->tid, buf));
				UNLOCK_LOCKERS(env, lrp);
				memset(&request, 0, sizeof(request));
				request.op = DB_LOCK_PUT_READ;
				if ((ret = __lock_vec(env,
			    	    lip, 0, &request, 1, NULL)) != 0)
					return (ret);
			}
			else
				UNLOCK_LOCKERS(env, lrp);

			/*
			 * This locker is most likely referenced by a cursor
			 * which is owned by a dead thread.  Normally the
			 * cursor would be available for other threads
			 * but we assume the dead thread will never release
			 * it.
			 */
			if (lip->id < TXN_MINIMUM &&
			    (ret = __lock_freefamilylocker(lt, lip)) != 0)
				return (ret);
			goto retry;
		}

	UNLOCK_LOCKERS(env, lrp);

	return (ret);
}
