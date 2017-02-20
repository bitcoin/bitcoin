/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "csv.h"
#include "csv_local.h"
#include "csv_extern.h"

/*
 * entry_print --
 *	Display the primary database's data item.
 */
int
entry_print(void *data, size_t len, u_int32_t field_count)
{
	u_int32_t a, *offset;
	u_int i;
	char *raw;

	memcpy(&a, data, sizeof(u_int32_t));
	printf("\tversion: %lu\n", (u_long)a);

	offset = (u_int32_t *)data + 1;
	if (field_count == 0) {
		memcpy(&a, offset++, sizeof(u_int32_t));
		printf("\tcolumn map: %lu fields: {%.*s}\n", (u_long)a,
		    (int)(len - 2 * sizeof(u_int32_t)),
		    (u_int8_t *)data + 2 * sizeof(u_int32_t));
	} else {
		raw = (char *)(offset + (field_count + 1));
		for (i = 0; i < field_count; ++i) {
			memcpy(&a, &offset[i], sizeof(u_int32_t));
			len = OFFSET_LEN(offset, i);
			printf("\toffset %4lu: len %4lu: {%.*s}\n",
			    (u_long)offset[i],
			    (u_long)len, (int)len, raw + a);
		}
	}

	return (0);
}

/*
 * strtod_err --
 *	strtod(3) with error checking.
 */
int
strtod_err(char *input, double *valp)
{
	double val;
	char *end;

	/*
	 * strtoul requires setting errno to detect errors.
	 */
	errno = 0;
	val = strtod(input, &end);
	if (errno == ERANGE) {
		dbenv->err(dbenv, ERANGE, "%s", input);
		return (1);
	}
	if (input[0] == '\0' ||
	    (end[0] != '\0' && end[0] != '\n' && !isspace(end[0]))) {
		dbenv->errx(dbenv,
		    "%s: invalid floating point argument", input);
		return (1);
	}

	*valp = val;
	return (0);
}

/*
 * strtoul_err --
 *	strtoul(3) with error checking.
 */
int
strtoul_err(char *input, u_long *valp)
{
	u_long val;
	char *end;

	/*
	 * strtoul requires setting errno to detect errors.
	 */
	errno = 0;
	val = strtoul(input, &end, 10);
	if (errno == ERANGE) {
		dbenv->err(dbenv, ERANGE, "%s", input);
		return (1);
	}
	if (input[0] == '\0' ||
	    (end[0] != '\0' && end[0] != '\n' && !isspace(end[0]))) {
		dbenv->errx(dbenv, "%s: invalid unsigned long argument", input);
		return (1);
	}

	*valp = val;
	return (0);
}

int
secondary_callback(DB *db_arg, const DBT *key, const DBT *data, DBT *result)
{
	DbField *f;
	DbRecord record;
	void *faddr, *addr;

	/* Populate the field. */
	if (DbRecord_init(key, data, &record) != 0)
		return (-1);

	f = db_arg->app_private;
	faddr = (u_int8_t *)&record + f->offset;

	/*
	 * If necessary, copy the field into separate memory.
	 * Set up the result DBT.
	 */
	switch (f->type) {
	case STRING:
		result->data = *(char **)faddr;
		result->size = strlen(*(char **)faddr) + 1;
		break;
	case DOUBLE:
		if ((addr = malloc(sizeof(double))) == NULL)
			return (-1);
		result->data = addr;
		result->size = sizeof(double);
		result->flags = DB_DBT_APPMALLOC;
		memcpy(addr, faddr, sizeof(double));
		break;
	case UNSIGNED_LONG:
		if ((addr = malloc(sizeof(u_long))) == NULL)
			return (-1);
		result->data = addr;
		result->size = sizeof(u_long);
		result->flags = DB_DBT_APPMALLOC;
		memcpy(addr, faddr, sizeof(u_long));
		break;
	default:
	case NOTSET:
		abort();
		/* NOTREACHED */
	}

	return (0);
}

/*
 * compare_double --
 *	Compare two keys.
 */
int
compare_double(DB *db_arg, const DBT *a_arg, const DBT *b_arg)
{
	double a, b;

	db_arg = db_arg;			/* Quiet compiler. */

	memcpy(&a, a_arg->data, sizeof(double));
	memcpy(&b, b_arg->data, sizeof(double));
	return (a > b ? 1 : ((a < b) ? -1 : 0));
}

/*
 * compare_ulong --
 *	Compare two keys.
 */
int
compare_ulong(DB *db_arg, const DBT *a_arg, const DBT *b_arg)
{
	u_long a, b;

	db_arg = db_arg;			/* Quiet compiler. */

	memcpy(&a, a_arg->data, sizeof(u_long));
	memcpy(&b, b_arg->data, sizeof(u_long));
	return (a > b ? 1 : ((a < b) ? -1 : 0));
}

/*
 * field_cmp_double --
 *	Compare two double.
 */
int
field_cmp_double(void *a, void *b, OPERATOR op)
{
	switch (op) {
	case GT:
		return (*(double *)a > *(double *)b);
	case GTEQ:
		return (*(double *)a >= *(double *)b);
	case LT:
		return (*(double *)a < *(double *)b);
	case LTEQ:
		return (*(double *)a <= *(double *)b);
	case NEQ:
		return (*(double *)a != *(double *)b);
	case EQ:
		return (*(double *)a == *(double *)b);
	case WC:
	case NWC:
		break;
	}

	abort();
	/* NOTREACHED */
}

/*
 * field_cmp_re --
 *	Compare against regular expression.
 */
int
field_cmp_re(void *a, void *b, OPERATOR op)
{
	op = op;				/* Quiet compiler. */

	switch (op) {
#ifdef HAVE_WILDCARD_SUPPORT
	case WC:
		return (regexec(b, *(char **)a, 0, NULL, 0) == 0);
	case NWC:
		return (regexec(b, *(char **)a, 0, NULL, 0) != 0);
#else
	case WC:
	case NWC:
		a = a;
		b = b;				/* Quiet compiler. */
		/* FALLTHROUGH */
#endif
	case GT:
	case GTEQ:
	case LT:
	case LTEQ:
	case NEQ:
	case EQ:
		break;
	}

	abort();
	/* NOTREACHED */
}

/*
 * field_cmp_string --
 *	Compare two strings.
 */
int
field_cmp_string(void *a, void *b, OPERATOR op)
{
	int v;

	v = strcasecmp(*(char **)a, b);
	switch (op) {
	case GT:
		return (v > 0 ? 1 : 0);
	case GTEQ:
		return (v >= 0 ? 1 : 0);
	case LT:
		return (v < 0 ? 1 : 0);
	case LTEQ:
		return (v <= 0 ? 1 : 0);
	case NEQ:
		return (v ? 1 : 0);
	case EQ:
		return (v ? 0 : 1);
	case WC:
	case NWC:
		break;
	}

	abort();
	/* NOTREACHED */
}

/*
 * field_cmp_ulong --
 *	Compare two ulongs.
 */
int
field_cmp_ulong(void *a, void *b, OPERATOR op)
{
	switch (op) {
	case GT:
		return (*(u_long *)a > *(u_long *)b);
	case GTEQ:
		return (*(u_long *)a >= *(u_long *)b);
	case LT:
		return (*(u_long *)a < *(u_long *)b);
	case LTEQ:
		return (*(u_long *)a <= *(u_long *)b);
	case NEQ:
		return (*(u_long *)a != *(u_long *)b);
	case EQ:
		return (*(u_long *)a == *(u_long *)b);
	case WC:
	case NWC:
		break;
	}

	abort();
	/* NOTREACHED */
}
