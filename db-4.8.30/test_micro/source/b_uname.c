/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "bench.h"

#define	UNAMEFILE	"NODENAME"

static int write_info __P((FILE *));

int
b_uname()
{
	FILE *fp;
	int ret;

	if ((fp = fopen(UNAMEFILE, "w")) == NULL)
		goto file_err;

	ret = write_info(fp);

	if (fclose(fp) != 0) {
file_err:	fprintf(stderr,
		    "%s: %s: %s\n", progname, UNAMEFILE, strerror(errno));
		return (1);
	}

	return (ret);
}

#ifdef DB_WIN32
static int
write_info(fp)
	FILE *fp;
{
	OSVERSIONINFO osver;
	SYSTEM_INFO sysinfo;
	char *p;

#ifdef DB_WINCE
	p = "WinCE";
#else
	{
	DWORD len;
	char buf[1024];

	len = sizeof(buf) - 1;
	GetComputerName(buf, &len);
	p = buf;
	}
#endif
	fprintf(fp, "<p>%s, ", p);

	GetSystemInfo(&sysinfo);
	switch (sysinfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_ALPHA:
		p = "alpha";
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		p = "x86";
		break;
	case PROCESSOR_ARCHITECTURE_MIPS:
		p = "mips";
		break;
	case PROCESSOR_ARCHITECTURE_PPC:
		p = "ppc";
		break;
	default:
		p = "unknown";
		break;
	}
	fprintf(fp, "%s<br>\n", p);
	memset(&osver, 0, sizeof(osver));
	osver.dwOSVersionInfoSize = sizeof(osver);
	GetVersionEx(&osver);
	switch (osver.dwPlatformId) {
	case VER_PLATFORM_WIN32_NT:	/* NT, Windows 2000 or Windows XP */
		if (osver.dwMajorVersion == 4)
			p = "Windows NT4x";
		else if (osver.dwMajorVersion <= 3)
			p = "Windows NT3x";
		else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion < 1)
			p = "Windows 2000";
		else if (osver.dwMajorVersion >= 5)
			p = "Windows XP";
		else
			p = "unknown";
		break;
	case VER_PLATFORM_WIN32_WINDOWS:	/* Win95, Win98 or WinME */
		if ((osver.dwMajorVersion > 4) ||
		  ((osver.dwMajorVersion == 4) && (osver.dwMinorVersion > 0))) {
			if (osver.dwMinorVersion >= 90)
				p = "Windows ME";
			else
				p = "Windows 98";
		} else
			p = "Windows 95";
		break;
	case VER_PLATFORM_WIN32s:		/* Windows 3.x */
		p = "Windows";
		break;
	default:
		p = "unknown";
		break;
	}
	fprintf(fp,
	    "%s, %ld.%02ld", p, osver.dwMajorVersion, osver.dwMinorVersion);
	return (0);
}

#elif defined(HAVE_VXWORKS)
static int
write_info(fp)
	FILE *fp;
{
	fprintf(fp, "<p>VxWorks");
	return (0);
}

#else /* POSIX */
#include <sys/utsname.h>

static int
write_info(fp)
	FILE *fp;
{
	struct utsname name;

	if (uname(&name) == 0)
		fprintf(fp, "<p>%s, %s<br>\n%s, %s, %s</p>\n", name.nodename,
		    name.machine, name.sysname, name.release, name.version);
	else {
		/*
		 * We've seen random failures on some systems, complain and
		 * skip the call if it fails.
		 */
		fprintf(stderr, "%s: uname: %s\n", progname, strerror(errno));

		fprintf(fp, "<p>POSIX");
	}
	return (0);
}
#endif
