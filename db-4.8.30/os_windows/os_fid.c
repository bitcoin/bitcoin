/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_fileid --
 *	Return a unique identifier for a file.
 */
int
__os_fileid(env, fname, unique_okay, fidp)
	ENV *env;
	const char *fname;
	int unique_okay;
	u_int8_t *fidp;
{
	pid_t pid;
	size_t i;
	u_int32_t tmp;
	u_int8_t *p;
	int ret;

	/*
	 * The documentation for GetFileInformationByHandle() states that the
	 * inode-type numbers are not constant between processes.  Actually,
	 * they are, they're the NTFS MFT indexes.  So, this works on NTFS,
	 * but perhaps not on other platforms, and perhaps not over a network.
	 * Can't think of a better solution right now.
	 */
	DB_FH *fhp;
	BY_HANDLE_FILE_INFORMATION fi;
	BOOL retval = FALSE;

	DB_ASSERT(env, fname != NULL);

	/* Clear the buffer. */
	memset(fidp, 0, DB_FILE_ID_LEN);

	/*
	 * First we open the file, because we're not given a handle to it.
	 * If we can't open it, we're in trouble.
	 */
	if ((ret = __os_open(env, fname, 0,
	    DB_OSO_RDONLY, DB_MODE_400, &fhp)) != 0)
		return (ret);

	/* File open, get its info */
	if ((retval = GetFileInformationByHandle(fhp->handle, &fi)) == FALSE)
		ret = __os_get_syserr();
	(void)__os_closehandle(env, fhp);

	if (retval == FALSE)
		return (__os_posix_err(ret));

	/*
	 * We want the three 32-bit words which tell us the volume ID and
	 * the file ID.  We make a crude attempt to copy the bytes over to
	 * the callers buffer.
	 *
	 * We don't worry about byte sexing or the actual variable sizes.
	 *
	 * When this routine is called from the DB access methods, it's only
	 * called once -- whatever ID is generated when a database is created
	 * is stored in the database file's metadata, and that is what is
	 * saved in the mpool region's information to uniquely identify the
	 * file.
	 *
	 * When called from the mpool layer this routine will be called each
	 * time a new thread of control wants to share the file, which makes
	 * things tougher.  As far as byte sexing goes, since the mpool region
	 * lives on a single host, there's no issue of that -- the entire
	 * region is byte sex dependent.  As far as variable sizes go, we make
	 * the simplifying assumption that 32-bit and 64-bit processes will
	 * get the same 32-bit values if we truncate any returned 64-bit value
	 * to a 32-bit value.
	 */
	tmp = (u_int32_t)fi.nFileIndexLow;
	for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
		*fidp++ = *p++;
	tmp = (u_int32_t)fi.nFileIndexHigh;
	for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
		*fidp++ = *p++;

	if (unique_okay) {
		/* Add in 32-bits of (hopefully) unique number. */
		__os_unique_id(env, &tmp);
		for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
			*fidp++ = *p++;

		/*
		 * Initialize/increment the serial number we use to help
		 * avoid fileid collisions.  Note we don't bother with
		 * locking; it's unpleasant to do from down in here, and
		 * if we race on this no real harm will be done, since the
		 * finished fileid has so many other components.
		 *
		 * We use the bottom 32-bits of the process ID, hoping they
		 * are more random than the top 32-bits (should we be on a
		 * machine with 64-bit process IDs).
		 *
		 * We increment by 100000 on each call as a simple way of
		 * randomizing; simply incrementing seems potentially less
		 * useful if pids are also simply incremented, since this
		 * is process-local and we may be one of a set of processes
		 * starting up.  100000 pushes us out of pid space on most
		 * 32-bit platforms, and has few interesting properties in
		 * base 2.
		 */
		if (DB_GLOBAL(fid_serial) == 0) {
			__os_id(env->dbenv, &pid, NULL);
			DB_GLOBAL(fid_serial) = (u_int32_t)pid;
		} else
			DB_GLOBAL(fid_serial) += 100000;

	} else {
		tmp = (u_int32_t)fi.dwVolumeSerialNumber;
		for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
			*fidp++ = *p++;
	}

	return (0);
}
