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
#include "dbinc/lock.h"

/*
 * The next two functions are the hash functions used to store objects in the
 * lock hash tables.  They are hashing the same items, but one (__lock_ohash)
 * takes a DBT (used for hashing a parameter passed from the user) and the
 * other (__lock_lhash) takes a DB_LOCKOBJ (used for hashing something that is
 * already in the lock manager).  In both cases, we have a special check to
 * fast path the case where we think we are doing a hash on a DB page/fileid
 * pair.  If the size is right, then we do the fast hash.
 *
 * We know that DB uses DB_LOCK_ILOCK types for its lock objects.  The first
 * four bytes are the 4-byte page number and the next DB_FILE_ID_LEN bytes
 * are a unique file id, where the first 4 bytes on UNIX systems are the file
 * inode number, and the first 4 bytes on Windows systems are the FileIndexLow
 * bytes.  This is followed by a random number.  The inode values tend
 * to increment fairly slowly and are not good for hashing.  So, we use
 * the XOR of the page number and the four bytes of the file id randome
 * number to produce a 32-bit hash value.
 *
 * We have no particular reason to believe that this algorithm will produce
 * a good hash, but we want a fast hash more than we want a good one, when
 * we're coming through this code path.
 */
#define	FAST_HASH(P) {			\
	u_int32_t __h;			\
	u_int8_t *__cp, *__hp;		\
	__hp = (u_int8_t *)&__h;	\
	__cp = (u_int8_t *)(P);		\
	__hp[0] = __cp[0] ^ __cp[12];	\
	__hp[1] = __cp[1] ^ __cp[13];	\
	__hp[2] = __cp[2] ^ __cp[14];	\
	__hp[3] = __cp[3] ^ __cp[15];	\
	return (__h);			\
}

/*
 * __lock_ohash --
 *
 * PUBLIC: u_int32_t __lock_ohash __P((const DBT *));
 */
u_int32_t
__lock_ohash(dbt)
	const DBT *dbt;
{
	if (dbt->size == sizeof(DB_LOCK_ILOCK))
		FAST_HASH(dbt->data);

	return (__ham_func5(NULL, dbt->data, dbt->size));
}

/*
 * __lock_lhash --
 *
 * PUBLIC: u_int32_t __lock_lhash __P((DB_LOCKOBJ *));
 */
u_int32_t
__lock_lhash(lock_obj)
	DB_LOCKOBJ *lock_obj;
{
	void *obj_data;

	obj_data = SH_DBT_PTR(&lock_obj->lockobj);

	if (lock_obj->lockobj.size == sizeof(DB_LOCK_ILOCK))
		FAST_HASH(obj_data);

	return (__ham_func5(NULL, obj_data, lock_obj->lockobj.size));
}

/*
 * __lock_nomem --
 *	Report a lack of some resource.
 *
 * PUBLIC: int __lock_nomem __P((ENV *, const char *));
 */
int
__lock_nomem(env, res)
	ENV *env;
	const char *res;
{
	__db_errx(env, "Lock table is out of available %s", res);
	return (ENOMEM);
}
