/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static void  __os_windows_ct_numb __P((char *, int));

/*
 * __os_ctime --
 *	Format a time-stamp.
 */
char *
__os_ctime(tod, time_buf)
	const time_t *tod;
	char *time_buf;
{
	char *ncp;
	__int64 i64_tod;
	struct _FILETIME file_tod, file_loc;
	struct _SYSTEMTIME sys_loc;
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS = 10000000; /* 10^7 */

	strcpy(time_buf, "Thu Jan 01 00:00:00 1970");
	time_buf[CTIME_BUFLEN - 1] = '\0';

	/* Convert the tod to a SYSTEM_TIME struct */
	i64_tod = *tod;
	i64_tod = (i64_tod + SECS_BETWEEN_EPOCHS)*SECS_TO_100NS;
	memcpy(&file_tod, &i64_tod, sizeof(file_tod));
	FileTimeToLocalFileTime(&file_tod, &file_loc);
	FileTimeToSystemTime(&file_loc, &sys_loc);

	/*
	 * Convert the _SYSTEMTIME to the correct format in time_buf.
	 * Based closely on the os_brew/ctime.c implementation.
	 *
	 * wWeekDay : Day of the week 0-6 (0=Monday, 6=Sunday)
	 */
	ncp = &"MonTueWedThuFriSatSun"[sys_loc.wDayOfWeek*3];
	time_buf[0] = *ncp++;
	time_buf[1] = *ncp++;
	time_buf[2] = *ncp;
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[(sys_loc.wMonth - 1) * 3];
	time_buf[4] = *ncp++;
	time_buf[5] = *ncp++;
	time_buf[6] = *ncp;

	__os_windows_ct_numb(time_buf + 8, sys_loc.wDay);
					/* Add 100 to keep the leading zero. */
	__os_windows_ct_numb(time_buf + 11, sys_loc.wHour + 100);
	__os_windows_ct_numb(time_buf + 14, sys_loc.wMinute + 100);
	__os_windows_ct_numb(time_buf + 17, sys_loc.wSecond + 100);

	if (sys_loc.wYear < 100) {		/* 9 99 */
		time_buf[20] = ' ';
		time_buf[21] = ' ';
		__os_windows_ct_numb(time_buf + 22, sys_loc.wYear);
	} else {			/* 99 1999 */
		__os_windows_ct_numb(time_buf + 20, sys_loc.wYear / 100);
		__os_windows_ct_numb(time_buf + 22, sys_loc.wYear % 100 + 100);
	}

	return (time_buf);
}

/*
 * __os_windows_ct_numb --
 *	Append ASCII representations for two digits to a string.
 */
static void
__os_windows_ct_numb(cp, n)
	char *cp;
	int n;
{
	cp[0] = ' ';
	if (n >= 10)
		cp[0] = (n / 10) % 10 + '0';
	cp[1] = n % 10 + '0';
}
