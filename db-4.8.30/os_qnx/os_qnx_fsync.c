/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$ 
 */

#include "db_config.h"

#include "db_int.h"

/*
 * QNX has special requirements on FSYNC: if the file is a shared memory
 * object, we can not call fsync because it is not implemented, instead,
 * we set the O_DSYNC flag to the file descriptor  and then do an empty
 * write so that all data are synced. We only sync this way if the file
 * is a shared memory object, other types of ordinary files are still synced
 * using fsync, to be not only faster but also atomic.
 * We don't just set the O_DSYNC flag on open, since it would force all writes
 * to be sync'ed. And we remove the O_DSYNC if it is not originally set to
 * the file descriptor before passed in to this function.
 * This is slightly different to the VxWorks and hp code above, since QNX does
 * supply a fsync call, it just has a unique requirement.
 */
int
__qnx_fsync(fhp)
	DB_FH *fhp;
{
	int ret;
	int fd, unset, flags;

	fd = fhp->fd;
	unset = 1;
	ret = flags = 0;
	if (F_ISSET(fhp, DB_FH_REGION))
	{
		RETRY_CHK(fcntl(fd, F_GETFL), ret);
		if (ret == -1)
			goto err;
		/*
		 * if already has O_DSYNC flag, we can't remove it
		 * after the empty write
		 */
		if (ret & O_DSYNC != 0)
			unset = 0;
		else {
			ret |= O_DSYNC;
			flags = ret;
			RETRY_CHK(fcntl(fd, F_SETFL, flags), ret);
			if (ret == -1)
				goto err;
		}
		/* Do an empty write, to force a sync */
		RETRY_CHK(write(fd, "", 0), ret);
		if (ret == -1)
			goto err;
		/* remove the O_DSYNC flag if necessary */
		if (unset) {
			RETRY_CHK(fcntl(fd, F_GETFL), ret);
			if (ret == -1)
				goto err;
			ret &= ~O_DSYNC;
			flags = ret;
			RETRY_CHK(fcntl(fd, F_SETFL, flags), ret);
			if (ret == -1)
				goto err;
		}
	} else
		RETRY_CHK(fdatasync(fd), ret);

err:	return (ret);
}
