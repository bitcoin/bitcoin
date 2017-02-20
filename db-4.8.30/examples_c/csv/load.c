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

typedef enum { GL_OK, GL_EOF, GL_FAIL } getline_status;

static int input_field_count(const char *, size_t, u_int32_t *);
static getline_status
	   input_getline(char **, size_t *, size_t *);
static int input_put_alloc(u_int32_t **, size_t *, size_t, u_int32_t);
static int input_set_offset(u_int32_t *, char *, size_t, u_int32_t);

static input_fmt ifmt;			/* Input format. */
static u_long	 record_count = 0;	/* Input record count for errors. */
static u_long	 version;		/* Version we're loading. */

/*
 * input_load --
 *	Read the input file and load new records into the database.
 */
int
input_load(input_fmt ifmt_arg, u_long version_arg)
{
	getline_status gtl_status;
	DBT key, data;
	DBC *cursor;
	u_int32_t field_count, primary_key, *put_line;
	size_t input_len, len, put_len;
	int is_first, ret;
	char *input_line;

	field_count = 0;			/* Shut the compiler up. */

	/* ifmt and version are global to this file. */
	ifmt = ifmt_arg;
	version = version_arg;

	/*
	 * The primary key for the database is a unique number.  Find out the
	 * last unique number allocated in this database by opening a cursor
	 * and fetching the last record.
	 */
	if ((ret = db->cursor(db, NULL, &cursor, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->cursor");
		return (1);
	}
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	if ((ret = cursor->c_get(cursor, &key, &data, DB_LAST)) != 0)
		if (ret == DB_NOTFOUND)
			primary_key = 0;
		else {
			dbenv->err(dbenv, ret, "DB->cursor: DB_LAST");
			return (1);
		}
	else
		memcpy(&primary_key, key.data, sizeof(primary_key));
	if ((ret = cursor->c_close(cursor)) != 0) {
		dbenv->err(dbenv, ret, "DBC->close");
		return (1);
	}
	if (verbose)
		dbenv->errx(dbenv,
		    "maximum existing record in the database is %lu",
		    (u_long)primary_key);

	key.data = &primary_key;
	key.size = sizeof(primary_key);
	input_line = NULL;
	put_line = NULL;
	input_len = put_len = 0;

	/*
	 * See the README file for a description of the file input format.
	 */
	for (is_first = 1; (gtl_status =
	    input_getline(&input_line, &input_len, &len)) == GL_OK;) {
		++record_count;
		if (verbose > 1)
			dbenv->errx(dbenv, "reading %lu", (u_long)record_count);

		/* The first non-blank line of the input is a column map. */
		if (is_first) {
			is_first = 0;

			/* Count the fields we're expecting in the input. */
			if (input_field_count(
			    input_line, len, &field_count) != 0)
				return (1);

		}

		/* Allocate room for the table of offsets. */
		if (input_put_alloc(
		    &put_line, &put_len, len, field_count) != 0)
			return (1);

		/*
		 * Build the offset table and create the record we're
		 * going to store.
		 */
		if (input_set_offset(put_line,
		    input_line, len, field_count) != 0)
			return (1);

		++primary_key;

		memcpy(put_line + (field_count + 2), input_line, len);
		data.data = put_line;
		data.size = (field_count + 2) * sizeof(u_int32_t) + len;

		if (verbose > 1)
			(void)entry_print(
			    data.data, data.size, field_count);

		/* Load the key/data pair into the database. */
		if ((ret = db->put(db, NULL, &key, &data, 0)) != 0) {
			dbenv->err(dbenv, ret,
			    "DB->put: %lu", (u_long)primary_key);
			return (1);
		}
	}

	if (gtl_status != GL_EOF)
		return (1);

	if (verbose)
		dbenv->errx(dbenv,
		    "%lu records read from the input file into the database",
		    record_count);

	/*
	 * This program isn't transactional, limit the window for corruption.
	 */
	if ((ret = db->sync(db, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->sync");
		return (1);
	}

	return (0);
}

/*
 * input_getline --
 *	Read in a line of input into a buffer.
 */
static getline_status
input_getline(char **input_linep, size_t *input_lenp, size_t *lenp)
{
	size_t input_len, len;
	int ch;
	char *input_line, *p, *endp;

	input_line = *input_linep;
	input_len = *input_lenp;

	p = input_line;
	endp = input_line + input_len;

	for (len = 0; (ch = getchar()) != EOF;) {
		if (ch == '\0')		/* Strip <nul> (\000) bytes. */
			continue;
		switch (ifmt) {
		case FORMAT_NL:
			if (ch == '\n')
				goto end;
			break;
		case FORMAT_EXCEL:
			/* Strip <nl> (\012) bytes. */
			if (ch == '\n')
				continue;
			/*
			 * <cr> (\015) bytes terminate lines.
			 * Skip blank lines.
			 */
			if (ch == '\015') {
				if (len == 0)
					continue;
				goto end;
			}
		}
		if (input_line == endp) {
			input_len += 256;
			input_len *= 2;
			if ((input_line =
			    realloc(input_line, input_len)) == NULL) {
				dbenv->err(dbenv, errno,
				    "unable to allocate %lu bytes for record",
				    (u_long)input_len);
				return (GL_FAIL);
			}
			p = input_line;
			endp = p + input_len;
		}

		if (isprint(ch)) {	/* Strip unprintables. */
			*p++ = (char)ch;
			++len;
		}
	}

end:	if (len == 0)
		return (GL_EOF);

	*lenp = len;
	*input_linep = input_line;
	*input_lenp = input_len;

	return (GL_OK);
}

/*
 * input_field_count --
 *	Count the fields in the line.
 */
static int
input_field_count(const char *line, size_t len, u_int32_t *field_countp)
{
	u_int32_t field_count;
	int quoted;

	field_count = 1;

	/*
	 * There are N-1 separators for N fields, that is, "a,b,c" is three
	 * fields, with two comma separators.
	 */
	switch (ifmt) {
	case FORMAT_EXCEL:
		quoted = 0;
		for (field_count = 1; len > 0; ++line, --len)
			if (*line == '"')
				quoted = !quoted;
			else if (*line == ',' && !quoted)
				++field_count;
		break;
	case FORMAT_NL:
		for (field_count = 1; len > 0; ++line, --len)
			if (*line == ',')
				++field_count;
		break;
	}
	*field_countp = field_count;

	if (verbose)
		dbenv->errx(dbenv,
		    "input file made up of %lu fields", (u_int)field_count);

	return (0);
}

/*
 * input_put_alloc --
 *	Allocate room for the offset table plus the input.
 */
static int
input_put_alloc(u_int32_t **put_linep,
    size_t *put_lenp, size_t len, u_int32_t field_count)
{
	size_t total;

	total = (field_count + 2) * sizeof(u_int32_t) + len;
	if (total > *put_lenp &&
	    (*put_linep = realloc(*put_linep, *put_lenp += total)) == NULL) {
		dbenv->err(dbenv, errno,
		    "unable to allocate %lu bytes for record",
		    (u_long)*put_lenp);
		return (1);
	}
	return (0);
}

/*
 * input_set_offset --
 *	Build an offset table and record combination.
 */
static int
input_set_offset(u_int32_t *put_line,
    char *input_line, size_t len, u_int32_t field_count)
{
	u_int32_t *op;
	int quoted;
	char *p, *endp;

	op = put_line;

	/* The first field is the version number. */
	*op++ = version;

	/*
	 * Walk the input line, looking for comma separators.  It's an error
	 * to have too many or too few fields.
	 */
	*op++ = 0;
	quoted = 0;
	for (p = input_line, endp = input_line + len;; ++p) {
		if (ifmt == FORMAT_EXCEL && p < endp) {
			if (*p == '"')
				quoted = !quoted;
			if (quoted)
				continue;
		}
		if (*p == ',' || p == endp) {
			if (field_count == 0) {
				dbenv->errx(dbenv,
				    "record %lu: too many fields in the record",
				    record_count);
				return (1);
			}
			--field_count;

			*op++ = (u_int32_t)(p - input_line) + 1;

			if (verbose > 1)
				dbenv->errx(dbenv,
				    "offset %lu: {%.*s}", op[-1],
				    OFFSET_LEN(op, -2), input_line + op[-2]);

			/*
			 * Don't insert a new field if the input lines ends
			 * in a comma.
			 */
			if (p == endp || p + 1 == endp)
				break;
		}
	}
	*op++ = (u_int32_t)(p - input_line);

	if (field_count != 0) {
		dbenv->errx(dbenv,
		    "record %lu: not enough fields in the record",
		    record_count);
		return (1);
	}
	memcpy(op, input_line, len);

	return (0);
}
