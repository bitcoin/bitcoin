/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_fs_notzero --
 *	Return 1 if allocated filesystem blocks are not zeroed.
 *
 * PUBLIC: int __os_fs_notzero __P((void));
 */
int
__os_fs_notzero()
{
	/* Most filesystems zero out implicitly created pages. */
	return (0);
}

/*
 * __os_support_direct_io --
 *	Return 1 if we support direct I/O.
 *
 * PUBLIC: int __os_support_direct_io __P((void));
 */
int
__os_support_direct_io()
{
	int ret;

	ret = 0;

#ifdef HAVE_O_DIRECT
	ret = 1;
#endif
#if defined(HAVE_DIRECTIO) && defined(DIRECTIO_ON)
	ret = 1;
#endif
	return (ret);
}

/*
 * __os_support_db_register --
 *	Return 1 if the system supports DB_REGISTER.
 *
 * PUBLIC: int __os_support_db_register __P((void));
 */
int
__os_support_db_register()
{
	return (1);
}

/*
 * __os_support_replication --
 *	Return 1 if the system supports replication.
 *
 * PUBLIC: int __os_support_replication __P((void));
 */
int
__os_support_replication()
{
	return (1);
}
