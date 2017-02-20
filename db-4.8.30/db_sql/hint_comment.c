/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * These are functions related to parsing and handling hint comments
 * embedded in the input SQL DDL source.  Hint comments convey BDB
 * configuration information that cannot be represented in SQL DDL.
 */

#include <ctype.h>
#include "db_sql.h"

static void
hc_warn(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "Warning: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ", near line %d\n", line_number);

	va_end(ap);
}

/*
 * Return a static copy of the given string, with the given length, in
 * which all whitespace has been removed 
 */
static char *
static_copy_minus_whitespace(in, len)
	const char *in;
	int len;
{
#define smw_bufsiz 10240
	static char out[smw_bufsiz];
	
	int in_i;
	int out_i;
	int in_quote;
	
	in_quote = 0;
	for (in_i = out_i = 0; in_i < len && in[in_i] != '\0'; in_i++) {
		if (in[in_i] == '"') {
			if (in_quote) in_quote = 0;
			else in_quote = 1;
		}
	
		if (in_quote || ! isspace(in[in_i])) {
			out[out_i++] = in[in_i];
			assert(out_i < smw_bufsiz);
		}
	}

	out[out_i] = '\0';

	return out;
}

/*
 * Extract a string from the given token.  The returned copy is static
 * and has had all whitespace removed.
 */
static char *
hint_comment_from_token(t)
	Token *t;
{
	int len;
	char *p;

	len = 0;
	p = NULL;
	if (t == NULL)
		return NULL;

	/* The token should be a whole comment; verify that */

	if (t->z[0] == '/') {
		assert(t->n >= 4 &&
		       t->z[1] == '*' &&
		       t->z[t->n - 2] == '*' &&
		       t->z[t->n - 1] == '/'); 
		p = ((char *)t->z) + 2;
		len = t->n - 4;
	} else if (t->z[0] == '-') {
		assert(t->n >= 3 &&
		       t->z[1] == '-');
		p = ((char *)t->z) + 2;
		len = t->n - 2;
	}

	assert(p != NULL);

	if (*p != '+')              /* the hint comment indicator */
		return NULL;

	return static_copy_minus_whitespace(p+1, len-1);
}

/*
 * Break a string into two parts at the delimiting char.  The left
 * token is returned, while the right token, if any, is placed in
 * *rest.  If found, the delimiting char in the input string is
 * replaced with a null char, to terminate the left token string.
 */
static char *
split(in, delimiter, rest)
	char *in;
	char delimiter;
	char **rest;
{
	char *p;

	*rest = NULL;

	for (p = in; ! (*p == delimiter || *p == '\0'); p++)
		;

	if (*p != '\0') {
		*rest = p + 1;
		*p = '\0';
	}

	return in;
}

/*
 * This is basically strtoul with multipliers for suffixes such as k,
 * m, g for kilobytes, megabytes, and gigabytes 
 */
static
unsigned long int parse_integer(s)
	char *s;
{
	unsigned long int x;
	char *t;

	x = strtoul(s, &t, 0);
	if (s == t)
		hc_warn("unparseable integer string %s", s);


	switch(*t) {
	case '\0':
		break;
	case 'k':
	case 'K':
		x = x * KILO;
		t++;
		break;
	case 'm':
	case 'M':
		x = x * MEGA;
		t++;
		break;
	case 'g':
	case 'G':
		x = x * GIGA;
		t++;
		break;
	}

	if (*t != '\0')
		hc_warn("unrecognized characters in integer string %s", s);

	return x;
}
	
static void
apply_environment_property(key, value)
	char *key;
	char *value;
{
	if (strcasecmp(key, "CACHESIZE") == 0) {
		the_schema.environment.cache_size = parse_integer(value);
	} else {
		hc_warn("Unrecognized environment property %s", key);
	}
}

static void
set_dbtype(entity, value)
	ENTITY *entity;
	char *value;
{
	if (strcasecmp(value, "btree") == 0) {
		entity->dbtype = "DB_BTREE";
	} else if (strcasecmp(value, "hash") == 0) {
		entity->dbtype = "DB_HASH";
	} else {
		hc_warn(
"unknown DBTYPE %s for antecedent %s, using default of DB_BTREE",
			value, entity->name);
		entity->dbtype = "DB_BTREE";
	}
}

static void
set_idx_dbtype(idx, value)
	DB_INDEX *idx;
	char *value;
{
	if (strcasecmp(value, "btree") == 0) {
		idx->dbtype = "DB_BTREE";
	} else if (strcasecmp(value, "hash") == 0) {
		idx->dbtype = "DB_HASH";
	} else {
		hc_warn(
"unknown DBTYPE %s for antecedent %s, using default of DB_BTREE",
			value, idx->name);
		idx->dbtype = "DB_BTREE";
	}
}

static void
apply_entity_property(key, value, entity)
	char *key;
	char *value;
	ENTITY *entity;
{
	if (strcasecmp(key, "DBTYPE") == 0) {
		set_dbtype(entity, value);
	} else {
		hc_warn("Unrecognized entity property %s", key);
	}
}

static void
apply_index_property(key, value, idx)
	char *key;
	char *value;
	DB_INDEX *idx;
{
	if (strcasecmp(key, "DBTYPE") == 0) {
		set_idx_dbtype(idx, value);
	} else {
		hc_warn("Unrecognized index property %s", key);
	}
}

/*
 * Apply a configuration keyword and parameter.
 */

static void
apply_configuration_property(key, value)
	char * key;
	char *value;
{
	switch (the_parse_progress.last_event) {
	case PE_NONE:
		hc_warn(
		      "Property setting (%s) with no antecedent SQL statement",
		      key);
		break;
	case PE_ENVIRONMENT:
		apply_environment_property(key, value);
		break;
	case PE_ENTITY:
	case PE_ATTRIBUTE:  /* no per-attribute properties yet */
		apply_entity_property(key, value, 
				      the_parse_progress.last_entity);
		break;
	case PE_INDEX:
		apply_index_property(key, value, the_parse_progress.last_index);
	}
}

/*
 * Extract property assignments from a SQL comment, if it is marked
 * as a hint comment.
 */
void parse_hint_comment(t)
	Token *t;
{
	char *assignment, *key, *value;
	char *comment;

        comment = hint_comment_from_token(t);

	if (comment == NULL)
		return;

	while (! (comment == NULL || *comment == '\0')) {
		assignment = split(comment, ',', &comment);

		/*
		 * Split the assignment into key, value tokens on the
		 * equals sign.  Verify that there is only one equals
		 * sign.
		 */
		key = split(assignment, '=', &value);

		if (value == NULL) {
			hc_warn("No value specified for property %s\n",
				key);
			break;
		}

		apply_configuration_property(key, value);

		key = split(key, '=', &value);
		if (value != NULL)
			hc_warn(
		      "Warning: incorrect hint comment syntax with property %s",
				key);
	}
}
