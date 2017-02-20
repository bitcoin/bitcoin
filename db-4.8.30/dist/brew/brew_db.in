/*-
 * $Id$
 *
 * The following provides the information necessary to build Berkeley DB
 * on BREW.
 */

#include <AEEAppGen.h>
#include <AEEShell.h>
#include <AEEFile.h>
#include <AEEStdLib.h>

#include "errno.h"
#include "db.h"
#include "clib_port.h"

/*
 * BREW doesn't have stdio.
 */
#define	EOF	(-1)			/* Declare stdio's EOF. */
#define	stderr	((IFile *)1)		/* Flag to call DBGPRINTF. */
#define	stdout	((IFile *)1)

/*
 * POSIX time structure/function compatibility.
 *
 * !!!
 * This is likely wrong, we should probably move all Berkeley DB time-specific
 * functionality into os/.
 */
struct tm {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
	/*
	 * We don't provide tm_zone or tm_gmtoff because BREW doesn't
	 * provide them.
	 */
};

/*
* The ctime() function converts the time pointed to by clock, representing
* time in seconds since the Epoch to local time in the form of a string.
*
* Berkeley DB uses POSIX time values internally.
*
* The POSIX time function returns seconds since the Epoch, which was
* 1970/01/01 00:00:00 UTC.
*
* BREW's GETUTCSECONDS() returns the number of seconds since
* 1980/01/06 00:00:00 UTC.
*
* To convert from BREW to POSIX, add the seconds between 1970 and 1980 plus
* 6 more days.
*/
#define	BREW_EPOCH_OFFSET	(315964800L)

/*
 * Map ANSI C library functions to BREW specific APIs.
 */
#define	atoi(a)			ATOI(a)
#define	free(a)			FREE(a)
#define	malloc(a)		MALLOC(a)
#define	memcmp(a, b, c)		MEMCMP(a, b, c)
#define	memmove(a, b, c)	MEMMOVE(a, b, c)
#define	memset(a, b, c)		MEMSET(a, b, c)
#define	realloc(a, b)		REALLOC(a, b)
#define	snprintf		SNPRINTF
#define	sprintf			SPRINTF
#define	strcat(a, b)		STRCAT(a, b)
#define	strchr(a, b)		STRCHR(a, b)
#define	strcmp(a, b)		STRCMP(a, b)
#define	strcpy(a, b)		STRCPY(a, b)
#define	strdup(a)		STRDUP(a)
#define	strlen(a)		STRLEN(a)
#define	strncmp(a, b, c)	STRNCMP(a, b, c)
#define	strncpy(a, b, c)	STRNCPY(a, b, c)
#define	strrchr(a, b)		STRRCHR(a, b)
#define	strtoul(a, b, c)	STRTOUL(a, b, c)
#define	vsnprintf(a, b, c, d)	VSNPRINTF(a, b, c, d)

/*
 * !!!
 * Don't #define memcpy to MEMCPY, even though it exists, because that results
 * in a C pre-processor loop and compile failure.
 *
 * Don't #define memcpy to MEMMOVE directly, that results in failure as well.
 */
#define	memcpy			memmove

/*
 * BREW does not have concept of 'sync'.
 *
 * It depends on the implementation of the BREW on various platforms, but
 * Mobilus confirms the version of BREW that ships to North America in 3G
 * models has sync features guaranteeing safe physical writes whenever a
 * write is performed using BREW API. Therefore, the issue is not on the
 * applications running top of BREW, but the implementation of BREW itself
 * that has to be checked in order to provide durability.
 */
#define	__os_fsync(a, b)	(0)
#define	fflush(a)		(0)

/*
 * The ANSI C library functions for which we wrote local versions.
 */
int	   fclose(FILE *);
int	   fgetc(FILE *);
char	  *fgets(char *, int, FILE *);
FILE	  *fopen(const char *, const char *);
size_t	   fwrite(const void *, size_t, size_t, FILE *);
char	  *getcwd(char *, size_t);
struct tm *localtime(const time_t *);
time_t	   time(time_t *);

/*
 * FILE_MANAGER_CREATE --
 *	Instantiate file manager instance.
 */
#define	FILE_MANAGER_CREATE(dbenv, mgr, ret) do {			\
	AEEApplet *__app = (AEEApplet *)GETAPPINSTANCE();		\
	int __ret;							\
	if ((__ret = ISHELL_CreateInstance(__app->m_pIShell,		\
	    AEECLSID_FILEMGR, (void **)&(mgr))) == SUCCESS)		\
		ret = 0;						\
	else {								\
		__db_syserr(dbenv, __ret, "ISHELL_CreateInstance");	\
		ret = __os_posix_err(__ret);				\
	}								\
} while (0)

/*
 * FILE_MANAGER_ERR --
 *	Handle file manager method error.
 */
#define	FILE_MANAGER_ERR(dbenv, mgr, name, op, ret) do {		\
	int __ret;							\
	__ret = IFILEMGR_GetLastError(mgr);				\
	if ((name) == NULL)						\
		__db_syserr(dbenv, __ret, "%s", op);			\
	else								\
		__db_syserr(dbenv, __ret, "%s: %s", name, op);		\
	(ret) = __os_posix_err(__ret);					\
} while (0)
