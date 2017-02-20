/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

#define	DATABASE	"sequence.db"
#define	SEQUENCE	"my_sequence"
int main __P((int, char *[]));
int usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	DB *dbp;
	DB_SEQUENCE *seq;
	DBT key;
	int i, ret, rflag;
	db_seq_t seqnum;
	char ch;
	const char *database, *progname = "ex_sequence";

	dbp = NULL;
	seq = NULL;

	rflag = 0;
	while ((ch = getopt(argc, argv, "r")) != EOF)
		switch (ch) {
		case 'r':
			rflag = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	/* Accept optional database name. */
	database = *argv == NULL ? DATABASE : argv[0];

	/* Optionally discard the database. */
	if (rflag)
		(void)remove(database);

	/* Create and initialize database object, open the database. */
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_create: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, progname);
	if ((ret = dbp->open(dbp,
	    NULL, database, NULL, DB_BTREE, DB_CREATE, 0664)) != 0) {
		dbp->err(dbp, ret, "%s: open", database);
		goto err;
	}

	if ((ret = db_sequence_create(&seq, dbp, 0)) != 0) {
		dbp->err(dbp, ret, "db_sequence_create");
		goto err;
	}

	memset(&key, 0, sizeof(DBT));
	key.data = SEQUENCE;
	key.size = (u_int32_t)strlen(SEQUENCE);

	if ((ret = seq->open(seq, NULL, &key, DB_CREATE)) != 0) {
		dbp->err(dbp, ret, "%s: DB_SEQUENCE->open", SEQUENCE);
		goto err;
	}

	for (i = 0; i < 10; i++) {
		if ((ret = seq->get(seq, NULL, 1, &seqnum, 0)) != 0) {
			dbp->err(dbp, ret, "DB_SEQUENCE->get");
			goto err;
		}

		/* There's no portable way to print 64-bit numbers. */
#ifdef _WIN32
		printf("Got sequence number %I64d\n", (int64_t)seqnum);
#else
		printf(
		    "Got sequence number %llu\n", (unsigned long long)seqnum);
#endif
	}

	/* Close everything down. */
	if ((ret = seq->close(seq, 0)) != 0) {
		seq = NULL;
		dbp->err(dbp, ret, "DB_SEQUENCE->close");
		goto err;
	}
	if ((ret = dbp->close(dbp, 0)) != 0) {
		fprintf(stderr,
		    "%s: DB->close: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);

err:	if (seq != NULL)
		(void)seq->close(seq, 0);
	if (dbp != NULL)
		(void)dbp->close(dbp, 0);
	return (EXIT_FAILURE);
}

int
usage()
{
	(void)fprintf(stderr, "usage: ex_sequence [-r] [database]\n");
	return (EXIT_FAILURE);
}
