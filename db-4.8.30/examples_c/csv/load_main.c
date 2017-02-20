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
DB	**secondary;			/* Secondaries */
int	  verbose;			/* Program verbosity */
char	 *progname;			/* Program name */

int
main(int argc, char *argv[])
{
	input_fmt ifmt;
	u_long version;
	int ch, ret, t_ret;
	char *home;

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
	ifmt = FORMAT_NL;
	version = 1;

	/* Process arguments. */
	while ((ch = getopt(argc, argv, "F:f:h:V:v")) != EOF)
		switch (ch) {
		case 'f':
			if (freopen(optarg, "r", stdin) == NULL) {
				fprintf(stderr,
				    "%s: %s\n", optarg, db_strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 'F':
			if (strcasecmp(optarg, "excel") == 0) {
				ifmt = FORMAT_EXCEL;
				break;
			}
			return (usage());
		case 'h':
			home = optarg;
			break;
		case 'V':
			if (strtoul_err(optarg, &version))
				return (EXIT_FAILURE);
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

	/*
	 * The home directory may not exist -- try and create it.  We don't
	 * bother to distinguish between failure to create it and it already
	 * existing, as the database environment open will fail if we aren't
	 * successful.
	 */
	if (home == NULL)
		home = getenv("DB_HOME");
	if (home != NULL)
		(void)mkdir(home, S_IRWXU);

	/* Create or join the database environment. */
	if (csv_env_open(home, 0) != 0)
		return (EXIT_FAILURE);

	/* Load records into the database. */
	ret = input_load(ifmt, version);

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
	(void)fprintf(stderr,
	    "usage: %s [-v] [-F excel] [-f csv-file] [-h home]\n", progname);
	return (EXIT_FAILURE);
}
