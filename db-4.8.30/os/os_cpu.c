/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifdef HAVE_SYSTEM_INCLUDE_FILES
#if defined(HAVE_PSTAT_GETDYNAMIC)
#include <sys/pstat.h>
#endif
#endif

/*
 * __os_cpu_count --
 *	Return the number of CPUs.
 *
 * PUBLIC: u_int32_t __os_cpu_count __P((void));
 */
u_int32_t
__os_cpu_count()
{
#if defined(HAVE_PSTAT_GETDYNAMIC)
	/*
	 * HP/UX.
	 */
	struct pst_dynamic psd;

	return ((u_int32_t)pstat_getdynamic(&psd,
	    sizeof(psd), (size_t)1, 0) == -1 ? 1 : psd.psd_proc_cnt);
#elif defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
	/*
	 * Solaris, Linux.
	 */
	long nproc;

	nproc = sysconf(_SC_NPROCESSORS_ONLN);
	return ((u_int32_t)(nproc > 1 ? nproc : 1));
#else
	return (1);
#endif
}
