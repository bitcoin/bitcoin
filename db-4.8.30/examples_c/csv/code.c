/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "csv.h"

typedef struct {
	char		*name;		/* Field name */
	char		*upper;		/* Field name in upper-case */
	datatype	 type;		/* Data type */
	int		 indx;		/* Index */
} FIELD;

int	 code_source(void);
int	 code_header(void);
int	 desc_dump(void);
int	 desc_load(void);
char	*type_to_string(datatype);
int	 usage(void);

/*
 * Globals
 */
FILE	*cfp;				/* C source file */
FILE	*hfp;				/* C source file */
char	*progname;			/* Program name */
int	 verbose;			/* Verbose flag */

u_int	 field_cnt;			/* Count of fields */
FIELD	*fields;			/* Field list */

int
main(int argc, char *argv[])
{
	int ch;
	char *cfile, *hfile;

	/* Initialize globals. */
	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		++progname;

	/* Initialize arguments. */
	cfile = "csv_local.c";		/* Default header/source files */
	hfile = "csv_local.h";

	/* Process arguments. */
	while ((ch = getopt(argc, argv, "c:f:h:v")) != EOF)
		switch (ch) {
		case 'c':
			cfile = optarg;
			break;
		case 'f':
			if (freopen(optarg, "r", stdin) == NULL) {
				fprintf(stderr,
				    "%s: %s\n", optarg, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 'h':
			hfile = optarg;
			break;
		case 'v':
			++verbose;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (*argv != NULL)
		return (usage());

	/* Load records from the input file. */
	if (desc_load())
		return (EXIT_FAILURE);

	/* Dump records for debugging. */
	if (verbose && desc_dump())
		return (EXIT_FAILURE);

	/* Open output files. */
	if ((cfp = fopen(cfile, "w")) == NULL) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, cfile, strerror(errno));
		return (EXIT_FAILURE);
	}
	if ((hfp = fopen(hfile, "w")) == NULL) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, hfile, strerror(errno));
		return (EXIT_FAILURE);
	}

	/* Build the source and header files. */
	if (code_header())
		return (EXIT_FAILURE);
	if (code_source())
		return (EXIT_FAILURE);

	return (EXIT_SUCCESS);
}

/*
 * desc_load --
 *	Load a description file.
 */
int
desc_load()
{
	u_int field_alloc;
	int version;
	char *p, *t, save_ch, buf[256];

	field_alloc = version = 0;
	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		if ((p = strchr(buf, '\n')) == NULL) {
			fprintf(stderr, "%s: input line too long\n", progname);
			return (1);
		}
		*p = '\0';

		/* Skip leading whitespace. */
		for (p = buf; isspace(*p); ++p)
			;

		/* Skip empty lines or lines beginning with '#'. */
		if (*p == '\0' || *p == '#')
			continue;

		/* Get a version. */
		if (!version) {
			if (strncasecmp(
			    p, "version", sizeof("version") - 1) == 0) {
				version = 1;
				continue;
			}
			fprintf(stderr,
			    "%s: expected \"version\" line\n", progname);
			return (1);
		}

		/*
		 * Skip block close -- not currently useful, but when this
		 * code supports versioned descriptions, it will matter.
		 */
		if (*p == '}') {
			version = 0;
			continue;
		}

		/* Allocate a new field structure as necessary. */
		if (field_cnt == field_alloc &&
		    (fields = realloc(fields, field_alloc += 100)) == NULL) {
			fprintf(stderr, "%s: %s\n", progname, strerror(errno));
			return (1);
		}

		/* Find the end of the field name. */
		for (t = p; *t != '\0' && !isspace(*t); ++t)
			;
		save_ch = *t;
		*t = '\0';
		if ((fields[field_cnt].name = strdup(p)) == NULL ||
		    (fields[field_cnt].upper = strdup(p)) == NULL) {
			fprintf(stderr, "%s: %s\n", progname, strerror(errno));
			return (1);
		}
		*t = save_ch;
		p = t;

		fields[field_cnt].indx = 0;
		fields[field_cnt].type = NOTSET;
		for (;;) {
			/* Skip to the next field, if any. */
			for (; *p != '\0' && isspace(*p); ++p)
				;
			if (*p == '\0')
				break;

			/* Find the end of the field. */
			for (t = p; *t != '\0' && !isspace(*t); ++t)
				;
			save_ch = *t;
			*t = '\0';
			if (strcasecmp(p, "double") == 0)
				fields[field_cnt].type = DOUBLE;
			else if (strcasecmp(p, "index") == 0)
				fields[field_cnt].indx = 1;
			else if (strcasecmp(p, "string") == 0)
				fields[field_cnt].type = STRING;
			else if (strcasecmp(p, "unsigned_long") == 0)
				fields[field_cnt].type = UNSIGNED_LONG;
			else {
				fprintf(stderr,
				    "%s: unknown keyword: %s\n", progname, p);
				return (1);
			}
			*t = save_ch;
			p = t;
		}

		/* Create a copy of the field name that's upper-case. */
		for (p = fields[field_cnt].upper; *p != '\0'; ++p)
			if (islower(*p))
				*p = (char)toupper(*p);
		++field_cnt;
	}
	if (ferror(stdin)) {
		fprintf(stderr, "%s: stdin: %s\n", progname, strerror(errno));
		return (1);
	}
	return (0);
}

/*
 * desc_dump --
 *	Dump a set of FIELD structures.
 */
int
desc_dump()
{
	FIELD *f;
	u_int i;

	for (f = fields, i = 0; i < field_cnt; ++i, ++f) {
		fprintf(stderr, "field {%s}: (", f->name);
		switch (f->type) {
		case NOTSET:
			fprintf(stderr, "ignored");
			break;
		case DOUBLE:
			fprintf(stderr, "double");
			break;
		case STRING:
			fprintf(stderr, "string");
			break;
		case UNSIGNED_LONG:
			fprintf(stderr, "unsigned_long");
			break;
		}
		if (f->indx)
			fprintf(stderr, ", indexed");
		fprintf(stderr, ")\n");
	}
	return (0);
}

/*
 * code_header --
 *	Print out the C #include file.
 */
int
code_header()
{
	FIELD *f;
	u_int i;

	fprintf(hfp, "/*\n");
	fprintf(hfp, " *  DO NOT EDIT: automatically built by %s.\n", progname);
	fprintf(hfp, " *\n");
	fprintf(hfp, " * Record structure.\n");
	fprintf(hfp, " */\n");
	fprintf(hfp, "typedef struct __DbRecord {\n");
	fprintf(hfp, "\tu_int32_t\t recno;\t\t/* Record number */\n");
	fprintf(hfp, "\n");
	fprintf(hfp, "\t/*\n");
	fprintf(hfp, "\t * Management fields\n");
	fprintf(hfp, "\t */\n");
	fprintf(hfp, "\tvoid\t\t*raw;\t\t/* Memory returned by DB */\n");
	fprintf(hfp, "\tu_char\t\t*record;\t/* Raw record */\n");
	fprintf(hfp, "\tsize_t\t\t record_len;\t/* Raw record length */\n\n");
	fprintf(hfp, "\tu_int32_t\t field_count;\t/* Field count */\n");
	fprintf(hfp, "\tu_int32_t\t version;\t/* Record version */\n\n");
	fprintf(hfp, "\tu_int32_t\t*offset;\t/* Offset table */\n");
	fprintf(hfp, "\n");

	fprintf(hfp, "\t/*\n");
	fprintf(hfp, "\t * Indexed fields\n");
	fprintf(hfp, "\t */\n");
	for (f = fields, i = 0; i < field_cnt; ++i, ++f) {
		if (f->type == NOTSET)
			continue;
		if (i != 0)
			fprintf(hfp, "\n");
		fprintf(hfp, "#define	CSV_INDX_%s\t%d\n", f->upper, i + 1);
		switch (f->type) {
		case NOTSET:
			/* NOTREACHED */
			abort();
			break;
		case DOUBLE:
			fprintf(hfp, "\tdouble\t\t %s;\n", f->name);
			break;
		case STRING:
			fprintf(hfp, "\tchar\t\t*%s;\n", f->name);
			break;
		case UNSIGNED_LONG:
			fprintf(hfp, "\tu_long\t\t %s;\n", f->name);
			break;
		}
	}
	fprintf(hfp, "} DbRecord;\n");

	return (0);
}

/*
 * code_source --
 *	Print out the C structure initialization.
 */
int
code_source()
{
	FIELD *f;
	u_int i;

	fprintf(cfp, "/*\n");
	fprintf(cfp,
	   " *  DO NOT EDIT: automatically built by %s.\n", progname);
	fprintf(cfp, " *\n");
	fprintf(cfp, " * Initialized record structure.\n");
	fprintf(cfp, " */\n");
	fprintf(cfp, "\n");
	fprintf(cfp, "#include \"csv.h\"\n");
	fprintf(cfp, "#include \"csv_local.h\"\n");
	fprintf(cfp, "\n");
	fprintf(cfp, "DbRecord DbRecord_base = {\n");
	fprintf(cfp, "\t0,\t\t/* Record number */\n");
	fprintf(cfp, "\tNULL,\t\t/* Memory returned by DB */\n");
	fprintf(cfp, "\tNULL,\t\t/* Raw record */\n");
	fprintf(cfp, "\t0,\t\t/* Raw record length */\n");
	fprintf(cfp, "\t%d,\t\t/* Field count */\n", field_cnt);
	fprintf(cfp, "\t0,\t\t/* Record version */\n");
	fprintf(cfp, "\tNULL,\t\t/* Offset table */\n");
	fprintf(cfp, "\n");
	for (f = fields, i = 0; i < field_cnt; ++i, ++f) {
		if (f->type == NOTSET)
			continue;
		switch (f->type) {
		case NOTSET:
			abort();
			/* NOTREACHED */
			break;
		case DOUBLE:
		case UNSIGNED_LONG:
			fprintf(cfp, "\t0,\t\t/* %s */\n", f->name);
			break;
		case STRING:
			fprintf(cfp, "\tNULL,\t\t/* %s */\n", f->name);
			break;
		}
	}
	fprintf(cfp, "};\n");

	fprintf(cfp, "\n");
	fprintf(cfp, "DbField fieldlist[] = {\n");
	for (f = fields, i = 0; i < field_cnt; ++i, ++f) {
		if (f->type == NOTSET)
			continue;
		fprintf(cfp, "\t{ \"%s\",", f->name);
		fprintf(cfp, " CSV_INDX_%s,", f->upper);
		fprintf(cfp, "\n\t    %s,", type_to_string(f->type));
		fprintf(cfp, " %d,", f->indx ? 1 : 0);
		fprintf(cfp, " NULL,");
		fprintf(cfp, " FIELD_OFFSET(%s)},\n", f->name);
	}
	fprintf(cfp, "\t{NULL, 0, STRING, 0, NULL, 0}\n};\n");

	return (0);
}

char *
type_to_string(type)
	datatype type;
{
	switch (type) {
	case NOTSET:
		return ("NOTSET");
	case DOUBLE:
		return ("DOUBLE");
	case STRING:
		return ("STRING");
	case UNSIGNED_LONG:
		return ("UNSIGNED_LONG");
	}

	abort();
	/* NOTREACHED */
}

int
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-v] [-c source-file] [-f input] [-h header-file]\n",
	    progname);
	exit(1);
}
