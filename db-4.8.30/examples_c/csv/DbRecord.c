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

static int	DbRecord_field(DbRecord *, u_int, void *, datatype);
static int	DbRecord_search_field(DbField *, char *, OPERATOR);
static int	DbRecord_search_recno(char *, OPERATOR);

/*
 * DbRecord_print --
 *	Display a DbRecord structure.
 */
void
DbRecord_print(DbRecord *recordp, FILE *fp)
{
	DbField *f;
	void *faddr;

	if (fp == NULL)
		fp = stdout;

	fprintf(fp, "Record: %lu:\n", (u_long)recordp->recno);
	for (f = fieldlist; f->name != NULL; ++f) {
		faddr = (u_int8_t *)recordp + f->offset;
		fprintf(fp, "\t%s: ", f->name);
		switch (f->type) {
		case NOTSET:
			/* NOTREACHED */
			abort();
			break;
		case DOUBLE:
			fprintf(fp, "%f\n", *(double *)faddr);
			break;
		case STRING:
			fprintf(fp, "%s\n", *(char **)faddr);
			break;
		case UNSIGNED_LONG:
			fprintf(fp, "%lu\n", *(u_long *)faddr);
			break;
		}
	}
}

/*
 * DbRecord_read --
 *	Read a specific record from the database.
 */
int
DbRecord_read(u_long recno_ulong, DbRecord *recordp)
{
	DBT key, data;
	u_int32_t recno;
	int ret;

	/*
	 * XXX
	 * This code assumes a record number (typed as u_int32_t) is the same
	 * size as an unsigned long, and there's no reason to believe that.
	 */
	recno = recno_ulong;

	/*
	 * Retrieve the requested record from the primary database.  Have
	 * Berkeley DB allocate memory for us, keeps the DB handle thread
	 * safe.
	 *
	 * We have the Berkeley DB library allocate memory for the record,
	 * which we own and must eventually free.  The reason is so we can
	 * have the string fields in the structure point into the actual
	 * record, rather than allocating structure local memory to hold them
	 * and copying them out of the record.
	 */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = &recno;
	key.size = sizeof(recno);
	data.flags = DB_DBT_MALLOC;
	if ((ret = db->get(db, NULL, &key, &data, 0)) != 0)
		return (ret);

	if ((ret = DbRecord_init(&key, &data, recordp)) != 0)
		return (ret);

	return (0);
}

/*
 * DbRecord_discard --
 *	Discard a DbRecord structure.
 */
int
DbRecord_discard(DbRecord *recordp)
{
	/* Free the allocated memory. */
	free(recordp->raw);
	recordp->raw = NULL;

	return (0);
}

/*
 * DbRecord_init --
 *	Fill in a DbRecord from the database key/data pair.
 */
int
DbRecord_init(const DBT *key, const DBT *data, DbRecord *recordp)
{
	DbField *f;
	u_int32_t skip;
	void *faddr;

	/* Initialize the structure (get the pre-set index values). */
	*recordp = DbRecord_base;

	/* Fill in the ID and version. */
	memcpy(&recordp->recno, key->data, sizeof(u_int32_t));
	memcpy(&recordp->version,
	    (u_int32_t *)data->data + 1, sizeof(u_int32_t));

	/* Set up the record references. */
	recordp->raw = data->data;
	recordp->offset = (u_int32_t *)data->data + 1;
	skip = (recordp->field_count + 2) * sizeof(u_int32_t);
	recordp->record = (u_int8_t *)data->data + skip;
	recordp->record_len = data->size - skip;

	for (f = fieldlist; f->name != NULL; ++f) {
		faddr = (u_int8_t *)recordp + f->offset;
		if (DbRecord_field(
		    recordp, f->fieldno, faddr, f->type) != 0)
			return (1);
	}
	return (0);
}

/*
 * DbRecord_field --
 *	Fill in an individual field of the DbRecord.
 */
static int
DbRecord_field(
    DbRecord *recordp, u_int field, void *addr, datatype type)
{
	size_t len;
	char number_buf[20];

	/*
	 * The offset table is 0-based, the field numbers are 1-based.
	 * Correct.
	 */
	--field;

	switch (type) {
	case NOTSET:
		/* NOTREACHED */
		abort();
		break;
	case STRING:
		*((u_char **)addr) = recordp->record + recordp->offset[field];
		recordp->record[recordp->offset[field] +
		    OFFSET_LEN(recordp->offset, field)] = '\0';
		break;
	case DOUBLE:
	case UNSIGNED_LONG:
		/* This shouldn't be possible -- 2^32 is only 10 digits. */
		len = OFFSET_LEN(recordp->offset, field);
		if (len > sizeof(number_buf) - 1) {
			dbenv->errx(dbenv,
    "record %lu field %lu: numeric field is %lu bytes and too large to copy",
			    recordp->recno, field, (u_long)len);
			return (1);
		}
		memcpy(number_buf,
		    recordp->record + recordp->offset[field], len);
		number_buf[len] = '\0';

		if (type == DOUBLE) {
			if (len == 0)
				*(double *)addr = 0;
			else if (strtod_err(number_buf, (double *)addr) != 0)
				goto fmt_err;
		} else
			if (len == 0)
				*(u_long *)addr = 0;
			else if (strtoul_err(number_buf, (u_long *)addr) != 0) {
fmt_err:			dbenv->errx(dbenv,
				    "record %lu: numeric field %u error: %s",
				    recordp->recno, field, number_buf);
				return (1);
			}
		break;
	}
	return (0);
}

/*
 * DbRecord_search_field_name --
 *	Search, looking for a record by field name.
 */
int
DbRecord_search_field_name(char *field, char *value, OPERATOR op)
{
	DbField *f;

	for (f = fieldlist; f->name != NULL; ++f)
		if (strcasecmp(field, f->name) == 0)
			return (DbRecord_search_field(f, value, op));

	/* Record numbers aren't handled as fields. */
	if (strcasecmp(field, "id") == 0)
		return (DbRecord_search_recno(value, op));

	dbenv->errx(dbenv, "unknown field name: %s", field);
	return (1);
}

/*
 * DbRecord_search_field_number --
 *	Search, looking for a record by field number.
 */
int
DbRecord_search_field_number(u_int32_t fieldno, char *value, OPERATOR op)
{
	DbField *f;

	for (f = fieldlist; f->name != NULL; ++f)
		if (fieldno == f->fieldno)
			return (DbRecord_search_field(f, value, op));

	dbenv->errx(dbenv, "field number %lu not configured", (u_long)fieldno);
	return (1);
}

/*
 * DbRecord_search_recno --
 *	Search, looking for a record by record number.
 */
static int
DbRecord_search_recno(char *value, OPERATOR op)
{
	DBC *dbc;
	DbRecord record;
	DBT key, data;
	u_int32_t recno;
	u_long recno_ulong;
	int ret;

	/*
	 * XXX
	 * This code assumes a record number (typed as u_int32_t) is the same
	 * size as an unsigned long, and there's no reason to believe that.
	 */
	if (strtoul_err(value, &recno_ulong) != 0)
		return (1);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = &recno;
	key.size = sizeof(recno);

	if ((ret = db->cursor(db, NULL, &dbc, 0)) != 0)
		return (ret);

	/*
	 * Retrieve the first record that interests us.  The range depends on
	 * the operator:
	 *
	 *	~		error
	 *	!=		beginning to end
	 *	<		beginning to first match
	 *	<=		beginning to last match
	 *	=		first match to last match
	 *	>		record after last match to end
	 *	>=		first match to end
	 */
	if (op == LT || op == LTEQ || op == NEQ || op == WC || op == NWC)
		recno = 1;
	else if (op == WC || op == NWC) {
		dbenv->errx(dbenv,
		    "wildcard operator only supported for string fields");
		return (1);
	} else {
		recno = recno_ulong;
		if (op == GT)
			++recno;
	}
	if ((ret = dbc->c_get(dbc, &key, &data, DB_SET)) != 0)
		goto err;

	for (;;) {
		if ((ret = DbRecord_init(&key, &data, &record)) != 0)
			break;
		if (field_cmp_ulong(&record.recno, &recno_ulong, op))
			DbRecord_print(&record, NULL);
		else
			if (op == LT || op == LTEQ || op == EQ)
				break;
		if ((ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) != 0)
			break;
	}

err:	return (ret == DB_NOTFOUND ? 0 : ret);
}

/*
 * DbRecord_search_field --
 *	Search, looking for a record by field.
 */
static int
DbRecord_search_field(DbField *f, char *value, OPERATOR op)
{
#ifdef HAVE_WILDCARD_SUPPORT
	regex_t preq;
#endif
	DBC *dbc;
	DbRecord record;
	DBT key, data, pkey;
	double value_double;
	u_long value_ulong;
	u_int32_t cursor_flags;
	int ret, t_ret;
	int (*cmp)(void *, void *, OPERATOR);
	void *faddr, *valuep;

	dbc = NULL;
	memset(&key, 0, sizeof(key));
	memset(&pkey, 0, sizeof(pkey));
	memset(&data, 0, sizeof(data));

	/*
	 * Initialize the comparison function, crack the value.  Wild cards
	 * are always strings, otherwise we follow the field type.
	 */
	if (op == WC || op == NWC) {
#ifdef HAVE_WILDCARD_SUPPORT
		if (f->type != STRING) {
			dbenv->errx(dbenv,
		    "wildcard operator only supported for string fields");
			return (1);
		}
		if (regcomp(&preq, value, 0) != 0) {
			dbenv->errx(dbenv, "regcomp of pattern failed");
			return (1);
		}
		valuep = &preq;
		cmp = field_cmp_re;
#else
		dbenv->errx(dbenv,
		    "wildcard operators not supported in this build");
		return (1);
#endif
	} else
		switch (f->type) {
		case DOUBLE:
			if (strtod_err(value, &value_double) != 0)
				return (1);
			cmp = field_cmp_double;
			valuep = &value_double;
			key.size = sizeof(double);
			break;
		case STRING:
			valuep = value;
			cmp = field_cmp_string;
			key.size = strlen(value);
			break;
		case UNSIGNED_LONG:
			if (strtoul_err(value, &value_ulong) != 0)
				return (1);
			cmp = field_cmp_ulong;
			valuep = &value_ulong;
			key.size = sizeof(u_long);
			break;
		default:
		case NOTSET:
			abort();
			/* NOTREACHED */
		}

	/*
	 * Retrieve the first record that interests us.  The range depends on
	 * the operator:
	 *
	 *	~		beginning to end
	 *	!=		beginning to end
	 *	<		beginning to first match
	 *	<=		beginning to last match
	 *	=		first match to last match
	 *	>		record after last match to end
	 *	>=		first match to end
	 *
	 * If we have a secondary, set a cursor in the secondary, else set the
	 * cursor to the beginning of the primary.
	 *
	 * XXX
	 * If the wildcard string has a leading non-magic character we should
	 * be able to do a range search instead of a full-database search.
	 *
	 * Step through records to the first non-match or to the end of the
	 * database, depending on the operation.  If the comparison function
	 * returns success for a key/data pair, print the pair.
	 */
	if (f->secondary == NULL || op == NEQ || op == WC || op == NWC) {
		if ((ret = db->cursor(db, NULL, &dbc, 0)) != 0)
			goto err;
		while ((ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) == 0) {
			if ((ret = DbRecord_init(&key, &data, &record)) != 0)
				break;
			faddr = (u_int8_t *)&record + f->offset;
			if (cmp(faddr, valuep, op))
				DbRecord_print(&record, NULL);
			else
				if (op == EQ || op == LT || op == LTEQ)
					break;
		}
	} else {
		if ((ret =
		    f->secondary->cursor(f->secondary, NULL, &dbc, 0)) != 0)
			goto err;
		key.data = valuep;
		cursor_flags = op == LT || op == LTEQ ? DB_FIRST : DB_SET_RANGE;
		if ((ret =
		    dbc->c_pget(dbc, &key, &pkey, &data, cursor_flags)) != 0)
			goto done;
		if (op == GT) {
			while ((ret = dbc->c_pget(
			    dbc, &key, &pkey, &data, DB_NEXT)) == 0) {
				if ((ret =
				    DbRecord_init(&pkey, &data, &record)) != 0)
					break;
				faddr = (u_int8_t *)&record + f->offset;
				if (cmp(faddr, valuep, op) != 0)
					break;
			}
			if (ret != 0)
				goto done;
		}
		do {
			if ((ret = DbRecord_init(&pkey, &data, &record)) != 0)
				break;
			faddr = (u_int8_t *)&record + f->offset;
			if (cmp(faddr, valuep, op))
				DbRecord_print(&record, NULL);
			else
				if (op == EQ || op == LT || op == LTEQ)
					break;
		} while ((ret =
		    dbc->c_pget(dbc, &key, &pkey, &data, DB_NEXT)) == 0);
	}

done:	if (ret == DB_NOTFOUND)
		ret = 0;

err:	if (dbc != NULL && (t_ret = dbc->c_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_WILDCARD_SUPPORT
	if (op == WC || op == NWC)
		regfree(&preq);
#endif

	return (ret);
}
