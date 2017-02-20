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
 * __os_cpu_count --
 *	Return the number of CPUs.
 *
 * PUBLIC: u_int32_t __os_cpu_count __P((void));
 */
u_int32_t
__os_cpu_count()
{
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);

	return ((u_int32_t)SystemInfo.dwNumberOfProcessors);
}
