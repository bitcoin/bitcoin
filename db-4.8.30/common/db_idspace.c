/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static int __db_idcmp __P((const void *, const void *));

static int
__db_idcmp(a, b)
	const void *a;
	const void *b;
{
	u_int32_t i, j;

	i = *(u_int32_t *)a;
	j = *(u_int32_t *)b;

	if (i < j)
		return (-1);
	else if (i > j)
		return (1);
	else
		return (0);
}

/*
 * __db_idspace --
 *
 * On input, minp and maxp contain the minimum and maximum valid values for
 * the name space and on return, they contain the minimum and maximum ids
 * available (by finding the biggest gap).  The minimum can be an inuse
 * value, but the maximum cannot be.
 *
 * PUBLIC: void __db_idspace __P((u_int32_t *, int, u_int32_t *, u_int32_t *));
 */
void
__db_idspace(inuse, n, minp, maxp)
	u_int32_t *inuse;
	int n;
	u_int32_t *minp, *maxp;
{
	int i, low;
	u_int32_t gap, t;

	/* A single locker ID is a special case. */
	if (n == 1) {
		/*
		 * If the single item in use is the last one in the range,
		 * then we've got to perform wrap which means that we set
		 * the min to the minimum ID, which is what we came in with,
		 * so we don't do anything.
		 */
		if (inuse[0] != *maxp)
			*minp = inuse[0];
		*maxp = inuse[0] - 1;
		return;
	}

	gap = 0;
	low = 0;
	qsort(inuse, (size_t)n, sizeof(u_int32_t), __db_idcmp);
	for (i = 0; i < n - 1; i++)
		if ((t = (inuse[i + 1] - inuse[i])) > gap) {
			gap = t;
			low = i;
		}

	/* Check for largest gap at the end of the space. */
	if ((*maxp - inuse[n - 1]) + (inuse[0] - *minp) > gap) {
		/* Do same check as we do in the n == 1 case. */
		if (inuse[n - 1] != *maxp)
			*minp = inuse[n - 1];
		*maxp = inuse[0] - 1;
	} else {
		*minp = inuse[low];
		*maxp = inuse[low + 1] - 1;
	}
}
