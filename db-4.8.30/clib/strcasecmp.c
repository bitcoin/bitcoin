/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * strcasecmp --
 *	Do strcmp(3) in a case-insensitive manner.
 *
 * PUBLIC: #ifndef HAVE_STRCASECMP
 * PUBLIC: int strcasecmp __P((const char *, const char *));
 * PUBLIC: #endif
 */
int
strcasecmp(s1, s2)
	const char *s1, *s2;
{
	u_char s1ch, s2ch;

	for (;;) {
		s1ch = *s1++;
		s2ch = *s2++;
		if (s1ch >= 'A' && s1ch <= 'Z')		/* tolower() */
			s1ch += 32;
		if (s2ch >= 'A' && s2ch <= 'Z')		/* tolower() */
			s2ch += 32;
		if (s1ch != s2ch)
			return (s1ch - s2ch);
		if (s1ch == '\0')
			return (0);
	}
	/* NOTREACHED */
}

/*
 * strncasecmp --
 *	Do strncmp(3) in a case-insensitive manner.
 *
 * PUBLIC: #ifndef HAVE_STRCASECMP
 * PUBLIC: int strncasecmp __P((const char *, const char *, size_t));
 * PUBLIC: #endif
 */
int
strncasecmp(s1, s2, n)
	const char *s1, *s2;
	register size_t n;
{
	u_char s1ch, s2ch;

	for (; n != 0; --n) {
		s1ch = *s1++;
		s2ch = *s2++;
		if (s1ch >= 'A' && s1ch <= 'Z')		/* tolower() */
			s1ch += 32;
		if (s2ch >= 'A' && s2ch <= 'Z')		/* tolower() */
			s2ch += 32;
		if (s1ch != s2ch)
			return (s1ch - s2ch);
		if (s1ch == '\0')
			return (0);
	}
	return (0);
}
