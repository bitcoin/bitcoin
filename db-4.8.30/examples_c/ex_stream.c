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

#define	DATABASE	"stream.db"
#define	CHUNK_SIZE	500
#define	DATA_SIZE	CHUNK_SIZE * 100

int main __P((int, char *[]));
int usage __P((void));
int invarg __P((const char *, int, const char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB *dbp;
	DBC *dbcp;
	DBT key, data;
	DBTYPE db_type;
	int ch, chunk_sz, chunk_off, data_sz, i, ret, rflag;
	int page_sz;
	char *database, *buf;
	const char *progname = "ex_stream";		/* Program name. */

	chunk_sz = CHUNK_SIZE;
	data_sz = DATA_SIZE;
	chunk_off = page_sz = rflag = 0;
	db_type = DB_BTREE;
	while ((ch = getopt(argc, argv, "c:d:p:t:")) != EOF)
		switch (ch) {
		case 'c':
			if ((chunk_sz = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'd':
			if ((data_sz = atoi(optarg)) <= 0)
				return (invarg(progname, ch, optarg));
			break;
		case 'p':
			if ((page_sz = atoi(optarg)) <= 0 ||
			    page_sz % 2 != 0 || page_sz < 512 ||
			    page_sz > 64 * 1024)
				return (invarg(progname, ch, optarg));
			break;
		case 't':
			switch (optarg[0]) {
			case 'b':
				db_type = DB_BTREE;
				break;
			case 'h':
				db_type = DB_HASH;
				break;
			case 'r':
				db_type = DB_RECNO;
				break;
			default:
				return (invarg(progname, ch, optarg));
				break;
			}
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	/* Accept optional database name. */
	database = *argv == NULL ? DATABASE : argv[0];

	if (chunk_sz > data_sz) {
		fprintf(stderr,
"Chunk size must be less than and a factor of the data size\n");

		return (usage());
	}

	/* Discard any existing database. */
	(void)remove(database);

	/* Create and initialize database object, open the database. */
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_create: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	dbp->set_errfile(dbp, stderr);
	dbp->set_errpfx(dbp, progname);
	if (page_sz != 0 && (ret = dbp->set_pagesize(dbp, page_sz)) != 0) {
		dbp->err(dbp, ret, "set_pagesize");
		goto err1;
	}
	if ((ret = dbp->set_cachesize(dbp, 0, 32 * 1024, 0)) != 0) {
		dbp->err(dbp, ret, "set_cachesize");
		goto err1;
	}
	if ((ret = dbp->open(dbp,
	    NULL, database, NULL, db_type, DB_CREATE, 0664)) != 0) {
		dbp->err(dbp, ret, "%s: open", database);
		goto err1;
	}

	/* Ensure the data size is a multiple of the chunk size. */
	data_sz = data_sz - (data_sz % chunk_sz);

	/* Initialize the key/data pair for a streaming insert. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = &chunk_sz; /* Our key value does not really matter. */
	key.size = sizeof(int);
	data.ulen = data_sz;
	data.size = chunk_sz;
	data.data = buf = malloc(data_sz);
	data.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;

	/* Populate the data with something. */
	for (i = 0; i < data_sz; ++i)
		buf[i] = (char)('a' + i % ('z' - 'a'));

	if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
		dbp->err(dbp, ret, "DB->cursor");
		goto err1;
	}
	for (chunk_off = 0; chunk_off < data_sz; chunk_off += chunk_sz) {
		data.size = chunk_sz;
		if ((ret = dbcp->put(dbcp, &key, &data,
		    (chunk_off == 0 ? DB_KEYFIRST : DB_CURRENT)) != 0)) {
			dbp->err(dbp, ret, "DBCursor->put");
			goto err2;
		}
		data.doff += chunk_sz;
	}
	if ((ret = dbcp->close(dbcp)) != 0) {
		dbp->err(dbp, ret, "DBcursor->close");
		goto err1;
	}

	memset(data.data, 0, data.ulen);
	/* Retrieve the data item in chunks. */
	if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
		dbp->err(dbp, ret, "DB->cursor");
		goto err1;
	}
	data.doff = 0;
	data.dlen = chunk_sz;
	memset(data.data, 0, data.ulen);

	/*
	 * Loop over the item, retrieving a chunk at a time.
	 * The requested chunk will be stored at the start of data.data.
	 */
	for (chunk_off = 0; chunk_off < data_sz; chunk_off += chunk_sz) {
		if ((ret = dbcp->get(dbcp, &key, &data,
		    (chunk_off == 0 ? DB_SET : DB_CURRENT)) != 0)) {
			dbp->err(dbp, ret, "DBCursor->get");
			goto err2;
		}
		data.doff += chunk_sz;
	}

	if ((ret = dbcp->close(dbcp)) != 0) {
		dbp->err(dbp, ret, "DBcursor->close");
		goto err1;
	}
	if ((ret = dbp->close(dbp, 0)) != 0) {
		fprintf(stderr,
		    "%s: DB->close: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);

err2:	(void)dbcp->close(dbcp);
err1:	(void)dbp->close(dbp, 0);
	return (EXIT_FAILURE);
}

int
invarg(progname, arg, str)
	const char *progname;
	int arg;
	const char *str;
{
	(void)fprintf(stderr,
	    "%s: invalid argument for -%c: %s\n", progname, arg, str);
	return (EXIT_FAILURE);
}

int
usage()
{
	(void)fprintf(stderr,
"usage: ex_stream [-c int] [-d int] [-p int] [-t char] [database]\n");
	(void)fprintf(stderr, "Where options are:\n");
	(void)fprintf(stderr, "\t-c set the chunk size.\n");
	(void)fprintf(stderr, "\t-d set the total record size.\n");
	(void)fprintf(stderr,
	    "\t-t choose a database type btree (b), hash (h) or recno (r)\n");

	return (EXIT_FAILURE);
}
