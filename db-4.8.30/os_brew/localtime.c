/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * localtime --
 *
 * PUBLIC: #ifndef HAVE_LOCALTIME
 * PUBLIC: struct tm *localtime __P((const time_t *));
 * PUBLIC: #endif
 */
struct tm *
localtime(tod)
	const time_t *tod;
{
	JulianType jt;
	boolean is_ds;					/* Daylight savings. */
	time_t tt;
	int increment;

	/*
	 * Berkeley DB uses POSIX time values internally, convert to a BREW
	 * time value.
	 */
	is_ds = 0;
	tt = *tod - BREW_EPOCH_OFFSET + LOCALTIMEOFFSET(&is_ds);

	GETJULIANDATE(tt, &jt);

	DB_GLOBAL(ltm).tm_sec = jt.wSecond;	/* seconds (0 - 60) */
	DB_GLOBAL(ltm).tm_min = jt.wMinute;	/* minutes (0 - 59) */
	DB_GLOBAL(ltm).tm_hour = jt.wHour;	/* hours (0 - 23) */
	DB_GLOBAL(ltm).tm_mday = jt.wDay;	/* day of month (1 - 31) */
	DB_GLOBAL(ltm).tm_mon = jt.wMonth - 1;	/* month of year (0 - 11) */
						/* year - 1900 */
	DB_GLOBAL(ltm).tm_year = jt.wYear - 1900;
						/* day of week (Sunday = 0) */
	DB_GLOBAL(ltm).tm_wday = (jt.wWeekDay + 1) % 7;
						/* day of year (0 - 365) */
	switch (DB_GLOBAL(ltm).tm_mon) {
	default:
	case  0: increment =   0; break;
	case  1: increment =  31; break;
	case  2: increment =  59; break;	/* Feb = 28 */
	case  3: increment =  90; break;
	case  4: increment = 120; break;
	case  5: increment = 151; break;
	case  6: increment = 181; break;
	case  7: increment = 212; break;
	case  8: increment = 243; break;
	case  9: increment = 273; break;
	case 10: increment = 304; break;
	case 11: increment = 334; break;
	}
	DB_GLOBAL(ltm).tm_yday = increment + DB_GLOBAL(ltm).tm_mday - 1;

	if (DB_GLOBAL(ltm).tm_mon > 1 &&	/* +1 leap years after Feb. */
	    jt.wYear % 4 == 0 && (jt.wYear % 100 != 0 || jt.wYear % 400 == 0))
		DB_GLOBAL(ltm).tm_yday += 1;

	DB_GLOBAL(ltm).tm_isdst = is_ds;	/* daylight savings time */

	/*
	 * !!!
	 * This routine is not thread-safe.  Berkeley DB should convert
	 * to using localtime_r() where it's available, and this routine
	 * should be re-written in the form of localtime_r().
	 */
	return (&DB_GLOBAL(ltm));
}
