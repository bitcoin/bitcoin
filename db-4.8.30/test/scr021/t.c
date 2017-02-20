/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
chk(f, str)
	char *f, *str;
{
	char *s;
	int ch, l, ok, pc;

	if (freopen(f, "r", stdin) == NULL) {
		fprintf(stderr, "%s: %s\n", f, strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (l = 1, ok = 1, s = str; (ch = getchar()) != EOF;) {
		if (ch == '\n')
			++l;
		if (!ok || ch != *s) {
			s = str;
			ok = !isalpha(ch) && !isdigit(ch) && ch != '_';
			continue;
		}
		if (*++s != '\0')
			continue;

		/* Match. */
		printf("%s: %d: %s", f, l, str);
		for (pc = 1; (ch = getchar()) != EOF;) {
			switch (ch) {
			case '(':
				++pc;
				break;
			case ')':
				--pc;
				break;
			case '\n':
				++l;
				break;
			}
			if (ch == '\n')
				putchar(' ');
			else
				putchar(ch);
			if (pc <= 0) {
				putchar('\n');
				break;
			}
		}
		s = str;
	}
}

int
main(int argc, char *argv[])
{
	int r;

	for (r = 0; *++argv != NULL;) {
		chk(*argv, "FLD_CLR(");
		chk(*argv, "FLD_ISSET(");
		chk(*argv, "FLD_SET(");
		chk(*argv, "F_CLR(");
		chk(*argv, "F_ISSET(");
		chk(*argv, "F_SET(");
	}
	return (0);
}
