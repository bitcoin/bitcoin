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

static int usage(void);

/*
 * Globals
 */
DB_ENV	 *dbenv;			/* Database environment */
DB	 *db;				/* Primary database */
int	  verbose;			/* Program verbosity */
char	 *progname;			/* Program name */

int
main(int argc, char *argv[])
{
	int ch, done, ret, t_ret;
	char **clist, **clp, *home;

	/* Initialize globals. */
	dbenv = NULL;
	db = NULL;
	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		++progname;
	verbose = 0;

	/* Initialize arguments. */
	home = NULL;
	ret = 0;

	/* Allocate enough room for command-list arguments. */
	if ((clp = clist =
	    (char **)calloc((size_t)argc + 1, sizeof(char *))) == NULL) {
		fprintf(stderr, "%s: %s\n", progname, strerror(ENOMEM));
		return (EXIT_FAILURE);
	}

	/* Process arguments. */
	while ((ch = getopt(argc, argv, "c:h:v")) != EOF)
		switch (ch) {
		case 'c':
			*clp++ = optarg;
			break;
		case 'h':
			home = optarg;
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

	/* Create or join the database environment. */
	if (csv_env_open(home, 1) != 0)
		return (EXIT_FAILURE);

	/* Handle the queries. */
	if (clp == clist)
		ret = query_interactive();
	else
		for (clp = clist, done = 0; *clp != NULL && !done; ++clp)
			if ((ret = query(*clp, &done)) != 0)
				break;

	/* Close the database environment. */
	if ((t_ret = csv_env_close()) != 0 && ret == 0)
		ret = t_ret;

	return (ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * usage --
 *	Program usage message.
 */
static int
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-v] [-c cmd] [-h home]\n", progname);
	return (EXIT_FAILURE);
}
