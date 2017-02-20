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

static int query_by_field(char *);
static int query_fieldlist(char *);
static int query_help(char *);
static int query_usage(void);

typedef struct _cmdtab {
	char	*cmd;			/* Command name */
	int	(*f)(char *);		/* Underlying function. */
	char	*help;			/* Help message. */
} CMDTAB;

static CMDTAB cmdtab[] = {
	{ "?",
		query_help,
		"?\t\tDisplay help screen" },
	{ "exit",
		NULL,
		"exit\t\tExit program" },
	{ "fields",
		query_fieldlist,
		"fields\t\tDisplay list of field names" },
	{ "help",
		query_help,
		"help\t\tDisplay help screen" },
	{ "quit",
		NULL,
		"quit\t\tExit program" },
	{ NULL,
		query_by_field,
    "field[op]value\tDisplay fields by value (=, !=, <, <=, >, >=, ~, !~)" },
	{ NULL,		NULL,		NULL }
};

/*
 * query_interactive --
 *	Allow the user to interactively query the database.
 */
int
query_interactive()
{
	int done;
	char *p, input[256];

	for (;;) {
		printf("Query: ");
		(void)fflush(stdout);
		if (fgets(input, sizeof(input), stdin) == NULL) {
			printf("\n");
			if (ferror(stdin)) {
				dbenv->err(dbenv, errno,
				    "error occurred reading from stdin");
				return (1);
			}
			break;
		}
		if ((p = strchr(input, '\n')) == NULL) {
			dbenv->errx(dbenv, "input buffer too small");
			return (1);
		}
		*p = '\0';
		if (query(input, &done) != 0)
			return (1);
		if (done != 0)
			break;
	}
	return (0);
}

/*
 * query --
 *	Process a query.
 */
int
query(char *cmd, int *donep)
{
	CMDTAB *p;

	if (donep != NULL)
		*donep = 0;

	for (p = cmdtab; p->cmd != NULL; ++p)
		if (p->cmd != NULL &&
		    strncasecmp(cmd, p->cmd, strlen(p->cmd)) == 0)
			break;

	if (p->cmd == NULL)
		return (query_by_field(cmd));

	if (p->f == NULL) {
		if (donep != NULL)
			*donep = 1;
		return (0);
	}

	return (p->f(cmd));
}

/*
 * query_by_field --
 *	Query the primary database by field.
 */
static int
query_by_field(char *input)
{
	OPERATOR operator;
	size_t len;
	char *field, *op, *value;

	/*
	 * We expect to see "field [op] value" -- figure it out.
	 *
	 * Skip leading whitespace.
	 */
	while (isspace(*input))
		++input;

	/*
	 * Find an operator, and it better not start the string.
	 */
	if ((len = strcspn(field = input, "<>!=~")) == 0)
		return (query_usage());
	op = field + len;

	/* Figure out the operator, and find the start of the value. */
	switch (op[0]) {
	case '~':
		operator = WC;
		value = op + 1;
		break;
	case '!':
		if (op[1] == '=') {
			operator = NEQ;
			value = op + 2;
			break;
		}
		if (op[1] == '~') {
			operator = NWC;
			value = op + 2;
			break;
		}
		return (query_usage());
	case '<':
		if (op[1] == '=') {
			operator = LTEQ;
			value = op + 2;
		} else {
			operator = LT;
			value = op + 1;
		}
		break;
	case '=':
		operator = EQ;
		if (op[1] == '=')
			value = op + 2;
		else
			value = op + 1;
		break;
	case '>':
		if (op[1] == '=') {
			operator = GTEQ;
			value = op + 2;
		} else {
			operator = GT;
			value = op + 1;
		}
		break;
	default:
		return (query_usage());
	}

	/* Terminate the field name, and there better be a field name. */
	while (--op > input && isspace(*op))
		;
	if (op == input)
		return (query_usage());
	op[1] = '\0';

	/* Make sure there is a value field. */
	while (isspace(*value))
		++value;
	if (*value == '\0')
		return (query_usage());

	return (DbRecord_search_field_name(field, value, operator));
}

/*
 * query_fieldlist --
 *	Display list of field names.
 */
static int
query_fieldlist(char *input)
{
	DbField *f;

	input = input;				/* Quiet compiler. */

	for (f = fieldlist; f->name != NULL; ++f)
		printf("field %3d: %s\n", f->fieldno, f->name);
	return (0);
}

/*
 * query_help --
 *	Query command list.
 */
static int
query_help(char *input)
{
	CMDTAB *p;

	input = input;				/* Quiet compiler. */

	printf("Query commands:\n");
	for (p = cmdtab; p->help != NULL; ++p)
		printf("\t%s\n", p->help);
	return (0);
}

/*
 * query_usage --
 *	Query usage message.
 */
static int
query_usage(void)
{
	fprintf(stderr, "%s: query syntax error\n", progname);
	return (query_help(NULL));
}
