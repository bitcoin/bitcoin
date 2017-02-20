/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "bench.h"

int main __P((int, char *[]));

static int  run __P((char *));
static int  usage __P((void));

char *progname;					/* program name */
db_timespec __start_time, __end_time;		/* TIMER_START & TIMER_END */

static int test_start = 1;			/* first test to run */
static int test_end = 0;			/* last test to run */

static struct {
	char *name;				/* command name */
	int (*f)(int, char *[]);		/* function */
} cmdlist[] = {
	{ "b_curalloc", b_curalloc },
	{ "b_curwalk", b_curwalk },
	{ "b_del", b_del },
	{ "b_get", b_get },
	{ "b_inmem", b_inmem },
	{ "b_latch", b_latch },
	{ "b_load", b_load },
	{ "b_open", b_open },
	{ "b_put", b_put },
	{ "b_recover", b_recover },
	{ "b_txn", b_txn },
	{ "b_txn_write", b_txn_write },
	{ "b_workload", b_workload },
	{ NULL, NULL }
};

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int ch, ret;
	char *run_directory, *ifile;

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

#ifdef DB_BREW
	if (bdb_brew_begin() != 0) {
		fprintf(stderr,
		    "%s: failed to initialize Berkeley DB on BREW\n");
		return (EXIT_FAILURE);
	}
#endif

	run_directory = NULL;
	ifile = "run.std";
	while ((ch = getopt(argc, argv, "d:e:i:s:")) != EOF)
		switch (ch) {
		case 'd':
			run_directory = optarg;
			break;
		case 'e':
			test_end = atoi(optarg);
			break;
		case 'i':
			ifile = optarg;
			break;
		case 's':
			test_start = atoi(optarg);
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	/* Run in the target directory. */
	if (run_directory != NULL && chdir(run_directory) != 0) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, run_directory, strerror(errno));
		return (1);
	}

	/* Clean up any left-over test directory. */
	if (b_util_dir_teardown())
		return (1);

	ret = run(ifile);

#ifdef DB_BREW
	bdb_brew_end();
#endif

	return (ret ? EXIT_FAILURE : EXIT_SUCCESS);
}

/*
 * run --
 *	Read a configuration file and run the tests.
 */
static int
run(ifile)
	char *ifile;
{
#ifdef HAVE_GETOPT_OPTRESET
	extern int optreset;
#endif
	extern int optind;
	static int test_cur = 0;
	FILE *ifp;
	int argc, cmdindx, lineno, ret;
	char *p, cmd[1024], path[1024], **argv;

	/* Identify the run. */
	if (b_uname() != 0)
		return (1);

	/* Open the list of tests. */
	if ((ifp = fopen(ifile, "r")) == NULL) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, ifile, strerror(errno));
		return (1);
	}

	for (lineno = 1; fgets(cmd, sizeof(cmd), ifp) != NULL; ++lineno) {
		/*
		 * Nul-terminate the command line; check for a trailing \r
		 * on Windows.
		 */
		if ((p = strchr(cmd, '\n')) == NULL) {
format_err:		fprintf(stderr, "%s: %s: line %d: illegal input\n",
			    progname, ifile, lineno);
			return (1);
		}
		if (p > cmd && p[-1] == '\r')
			--p;
		*p = '\0';

		/* Skip empty lines and comments. */
		if (cmd[0] == '\0' || cmd[0] == '#')
			continue;

		/* Optionally limit the test run to specific tests. */
		if (++test_cur < test_start ||
		    (test_end != 0 && test_cur > test_end))
			continue;

		fprintf(stderr, "%d: %s\n", test_cur, cmd);

		/* Find the command. */
		if ((p = strchr(cmd, ' ')) == NULL)
			goto format_err;
		*p++ = '\0';
		for (cmdindx = 0; cmdlist[cmdindx].name != NULL; ++cmdindx)
			if (strcmp(cmd, cmdlist[cmdindx].name) == 0)
				break;
		if (cmdlist[cmdindx].name == NULL)
			goto format_err;

		/* Build argc/argv. */
		if (__db_util_arg(cmd, p, &argc, &argv) != 0)
			return (1);

		/* Re-direct output into the test log file.  */
		(void)snprintf(path, sizeof(path), "%d", test_cur);
		if (freopen(path, "a", stdout) == NULL) {
			fprintf(stderr,
			    "%s: %s: %s\n", progname, path, strerror(errno));
			return (1);
		}

		/*
		 * Each underlying "program" re-parses its arguments --
		 * reset getopt.
		 */
#ifdef HAVE_GETOPT_OPTRESET
		optreset = 1;
#endif
		optind = 1;

		/* Prepare the test directory. */
		if (b_util_dir_setup())
			return (1);

		ret = cmdlist[cmdindx].f(argc, argv);

		/* Clean up the test directory. */
		if (b_util_dir_teardown())
			return (1);

		(void)fflush(stdout);

#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 1
		__os_free(NULL, argv, 0);
#else
		__os_free(NULL, argv);
#endif
		if (ret != 0)
			return (ret);
	}

	return (0);
}

static int
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-d directory] [-e end] [-i input] [-s start]\n",
	    progname);
	return (EXIT_FAILURE);
}
