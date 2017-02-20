/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#if defined(HAVE_SYSTEM_INCLUDE_FILES) && defined(HAVE_BACKTRACE) && \
    defined(HAVE_BACKTRACE_SYMBOLS) && defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

/*
 * __os_stack --
 *	Output a stack trace to the message file handle.
 *
 * PUBLIC: void __os_stack __P((ENV *));
 */
void
__os_stack(env)
	ENV *env;
{
#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)
	void *array[200];
	size_t i, size;
	char **strings;

	/*
	 * Solaris and the GNU C library support this interface.  Solaris
	 * has additional interfaces (printstack and walkcontext), I don't
	 * know if they offer any additional value or not.
	 */
	size = backtrace(array, sizeof(array) / sizeof(array[0]));
	strings = backtrace_symbols(array, size);

	for (i = 0; i < size; ++i)
		__db_errx(env, "%s", strings[i]);
	free(strings);
#endif
	COMPQUIET(env, NULL);
}
