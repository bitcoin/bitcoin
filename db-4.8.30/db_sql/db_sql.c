/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 */

/*
 * This is the entry function of the db_sql command.  Db_sql is a
 * utility program that translates a schema description written in a
 * SQL Data Definition Language dialect into C code that implements
 * the schema using Berkeley DB.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "db_sql.h"

extern int getopt(int, char *const [], const char *);
static int usage(char *);
static char * change_extension(char *path, char *extension);
static int read_and_parse(FILE *fp);

char *progname = "db_sql";
int line_number = 0;
int debug = 0;

int
main(argc,argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	int opt, free_ofilename, free_hfilename;
	FILE *ifile, *hfile, *ofile, *tfile, *vfile;
	char *ifilename, *hfilename, *ofilename, *tfilename, *vfilename;

	ifilename = hfilename = ofilename = tfilename = vfilename = NULL;
	free_ofilename = free_hfilename = 0;

	progname = argv[0];

	/* parse the command line switches */

	while ((opt = getopt(argc, argv, "i:t:o:h:dv:")) != -1) {
		switch(opt) {
		case 'i':              /* input file name */
			ifilename = optarg;
			break;
		case 'h':              /* header output file name */
			hfilename = optarg;
			break;
		case 'o':              /* output file name */
			ofilename = optarg;
			break;
		case 't':              /* test code output file name */
			tfilename = optarg;
			break;
		case 'd':
			debug = 1;
			break;
		case 'v':              /* verification code output file name */
			vfilename = optarg;
			break;
		default:
			return(usage(0));
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 0) {
		fprintf(stderr, 
			"extra argument %s after switch arguments\n", *argv);
		return(usage(0));
	}

	if (ifilename == NULL)
		ifile = stdin;
	else
		if ((ifile = fopen(ifilename, "r")) == NULL)
			return(usage(ifilename));

	/* if ofilename wasn't given, use ifilename with a .c extension */

	if (ofilename == NULL && ifilename != NULL) {
		ofilename = change_extension(ifilename, "c");
		free_ofilename = 1;
	}

	if (ofilename == NULL)
		ofile = stdout;
	else
		if ((ofile = fopen(ofilename, "w")) == NULL)
			return(usage(ofilename));

	/* if hfilename wasn't given, use ofilename with a .h extension */

	if (hfilename == NULL && ofilename != NULL) {
		hfilename = change_extension(ofilename, "h");
		free_hfilename = 1;
	}

	if (hfilename == NULL)
		hfile = stdout;
	else
		if ((hfile = fopen(hfilename, "w")) == NULL)
			return(usage(hfilename));

	/*
	 * if tfile wasn't given, we won't generate the test code.
	 *  tfile == null turns off test code generation
	 */
	if (tfilename == NULL)
		tfile = 0;
	else {
		if (hfilename == NULL) {
			fprintf(stderr,
			    "Can't produce test when streaming to stdout\n");
			return(usage(0));
		}
		if ((tfile = fopen(tfilename, "w")) == NULL)
			return(usage(tfilename));
	}
	/*
	 * Verification files are generated for internal testing purposes, 
	 * they are similar to the test output file. This functionality is
	 * not targeted at end users, so is not documented.
	 */
	if (vfilename == NULL)
		vfile = 0;
	else {
		if (hfilename == NULL) {
			fprintf(stderr,
			    "Can't produce verify when streaming to stdout\n");
			return(usage(0));
		}
		if ((vfile = fopen(vfilename, "w")) == NULL)
			return(usage(vfilename));
	}

	if (read_and_parse(ifile))
		exit(1);

	generate(hfile, ofile, tfile, vfile, hfilename);

	/* clean up the allocated memory */
	if (free_ofilename)
		free(ofilename);
	if (free_hfilename)
		free(hfilename);
	return 0;
}

/*
 * Scan input buffer for a semicolon that is not in a comment.
 * Later, this may need to notice quotes as well.
 */
static char *
scan_for_rightmost_semicolon(p)
	char *p;
{
	static enum scanner_state {
		IDLE = 0, GOT_SLASH = 1, IN_SLASHSTAR_COMMENT = 2,
		GOT_STAR = 3, GOT_HYPHEN = 4, IN_HYPHHYPH_COMMENT = 5
	} state = IDLE;

	char *result;

	result = NULL;

	if (p == NULL || *p == '\0')
		return result;

	do {
		switch(state) {
		case IDLE:
			switch(*p) {
			case '/': state = GOT_SLASH; break;
			case '*': state = GOT_STAR; break;
			case '-': state = GOT_HYPHEN; break;
			}
			break;
		case GOT_SLASH:
			switch(*p) {
			case '*': state = IN_SLASHSTAR_COMMENT; break;
			default: state = IDLE;
			}
			break;
		case IN_SLASHSTAR_COMMENT:
			switch(*p) {
			case '*': state = GOT_STAR; break;
			}
			break;
		case GOT_STAR:
			switch(*p) {
			case '/': state = IDLE; break;
			default: state = IN_SLASHSTAR_COMMENT; break;
			}
			break;
		case GOT_HYPHEN:
			switch(*p) {
			case '-': state = IN_HYPHHYPH_COMMENT; break;
			default: state = IDLE; break;
			}
		case IN_HYPHHYPH_COMMENT:
			switch(*p) {
			case '\n': state = IDLE; break;
			}
			break;
		}

		if (state == IDLE && *p == ';')
			result = p;

	} while (*p++);

	return result;
}

/*
 * read_and_parse reads lines from the input file (containing SQL DDL),
 * and sends the to the tokenizer and parser.  Because of the way the
 * SQLite tokenizer works, the chunks sent to the tokenizer must
 * contain a multiple of whole SQL statements -- a partial statement
 * will produce a syntax error.  Therefore, this function splits its
 * input at semicolons.
 */
static int
read_and_parse(fp)
	FILE *fp;
{
	size_t line_len, copy_len, collector_len;
	char *q, *collector, buf[256], *err_msg;

	collector = 0;
	collector_len = 0;
	err_msg = 0;

	/* line_number is global */

	for (line_number = 1; fgets(buf, sizeof(buf), fp) != 0; line_number++) {

		line_len = strlen(buf);

		if (1 + strlen(buf)  == sizeof(buf)) {
			fprintf(stderr, "%s: line %d is too long", progname, 
				line_number);
			return 1;
		}

		/*
		 * Does this line contain a semicolon?  If so, copy
		 * the line, up to and including its last semicolon,
		 * into collector and parse it.  Then reinitialize
		 * collector with the remainer of the line
		 */
		if ((q = scan_for_rightmost_semicolon(buf)) != NULL)
			copy_len = 1 + q - buf;
		else
			copy_len = line_len;

		collector_len += 1 + copy_len;
		if (collector == NULL)
			collector = calloc(1, collector_len);
		else
			collector = realloc(collector, collector_len);

		strnconcat(collector, collector_len, buf, copy_len);

		if (q != 0) {
			if (do_parse(collector, &err_msg) != 0) {
				fprintf(stderr,
					"parsing error at line %d : %s\n",
					line_number, err_msg);
				return 1;
			}
				
			collector_len = 1 + line_len - copy_len;
			collector = realloc(collector, collector_len);
			memcpy(collector, buf + copy_len, collector_len);
			assert(collector[collector_len-1] == 0);
		}
	}

	/*
	 * if there's anything after the final semicolon, send it on
	 * to the tokenizer -- it might be a hint comment 
	 */
	if (collector != 0) {
		if (strlen(collector) > 0 &&
		    do_parse(collector, &err_msg) != 0) {
			fprintf(stderr, "parsing error at end of file: %s\n",
				err_msg);
			return 1;
		}
	
		free (collector);
	}
	
	return 0;
}
	
/*
 * Basename isn't available everywhere, so we have our own version
 * which works on unix and windows.
 */
static char *
final_component_of(path)
	char *path;
{
	char *p;
	p = strrchr(path, '/');
	if (p == NULL)
		p = strrchr(path, '\\');
	if (p != NULL)
		return p + 1;

	return path;
}
			
/*
 * Return a new pathname in which any existing "extension" (the part
 * after ".") has been replaced by the given extension.  If the
 * pathname has no extension, the new extension is simply appended.
 * Returns allocated memory 
 */
static char *
change_extension(path, extension)
	char *path, *extension;
{
        size_t path_len, copy_len;
	char *p, *copy;
	const char dot = '.';

	/* isolate the final component of the pathname, so that we can
	 * examine it for the presence of a '.' without finding a '.'
	 * in a directory name componenet of the pathname
	 */

	p = final_component_of(path);
	if (*p != 0)
		p++;  /* skip initial char in basename, it could be a dot */

	/*
	 * Is there a dot in the basename? If so, then the path has
	 * an extension that we'll elide before adding the new one.
	 */
	if (strrchr(p, dot) != 0) {
		p = strrchr(path, dot);
		path_len = p - path;
	} else
		path_len = strlen(path);

	copy_len = 2 + path_len + strlen(extension);
	copy = malloc(copy_len);
	memcpy(copy, path, path_len);
	copy[path_len] = 0; /* terminate the string */
	strconcat(copy, copy_len, ".");
	strconcat(copy, copy_len, extension);

	return copy;
}

static int
usage(char *error_tag) {
	if (error_tag != 0)
		perror(error_tag);
	fprintf(stderr, "\
Usage:  %s [-i inputFile] [-h outputHeaderFile] [-o outputFile] \
[-t testOutputFile] [-d] [-v verificationOutputFile]\n",
		progname);
	return(1);
}
