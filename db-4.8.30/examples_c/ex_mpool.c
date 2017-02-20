/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#else
#include <unistd.h>
#endif

#include <db.h>

int	init __P((const char *, int, int, const char *));
int	run __P((int, int, int, int, const char *));
int	run_mpool __P((int, int, int, int, const char *));
int	main __P((int, char *[]));
int	usage __P((const char *));
#define	MPOOL	"mpool"					/* File. */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int cachesize, ch, hits, npages, pagesize;
	char *progname;

	cachesize = 20 * 1024;
	hits = 1000;
	npages = 50;
	pagesize = 1024;
	progname = argv[0];
	while ((ch = getopt(argc, argv, "c:h:n:p:")) != EOF)
		switch (ch) {
		case 'c':
			if ((cachesize = atoi(optarg)) < 20 * 1024)
				return (usage(progname));
			break;
		case 'h':
			if ((hits = atoi(optarg)) <= 0)
				return (usage(progname));
			break;
		case 'n':
			if ((npages = atoi(optarg)) <= 0)
				return (usage(progname));
			break;
		case 'p':
			if ((pagesize = atoi(optarg)) <= 0)
				return (usage(progname));
			break;
		case '?':
		default:
			return (usage(progname));
		}
	argc -= optind;
	argv += optind;

	return (run_mpool(pagesize, cachesize,
	    hits, npages, progname) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
usage(progname)
	const char *progname;
{
	(void)fprintf(stderr,
	    "usage: %s [-c cachesize] [-h hits] [-n npages] [-p pagesize]\n",
	    progname);
	return (EXIT_FAILURE);
}

int
run_mpool(pagesize, cachesize, hits, npages, progname)
	int pagesize, cachesize, hits, npages;
	const char *progname;
{
	int ret;

	/* Initialize the file. */
	if ((ret = init(MPOOL, pagesize, npages, progname)) != 0)
		return (ret);

	/* Get the pages. */
	if ((ret = run(hits, cachesize, pagesize, npages, progname)) != 0)
		return (ret);

	return (0);
}

/*
 * init --
 *	Create a backing file.
 */
int
init(file, pagesize, npages, progname)
	const char *file, *progname;
	int pagesize, npages;
{
	FILE *fp;
	int cnt;
	char *p;

	/*
	 * Create a file with the right number of pages, and store a page
	 * number on each page.
	 */
	(void)remove(file);
	if ((fp = fopen(file, "wb")) == NULL) {
		fprintf(stderr,
		    "%s: %s: %s\n", progname, file, strerror(errno));
		return (1);
	}
	if ((p = (char *)malloc(pagesize)) == NULL) {
		fprintf(stderr, "%s: %s\n", progname, strerror(ENOMEM));
		return (1);
	}

	/*
	 * The pages are numbered from 0, not 1.
	 *
	 * Write the index of the page at the beginning of the page in order
	 * to verify the retrieved page (see run()).
	 */
	for (cnt = 0; cnt < npages; ++cnt) {
		*(db_pgno_t *)p = cnt;
		if (fwrite(p, pagesize, 1, fp) != 1) {
			fprintf(stderr,
			    "%s: %s: %s\n", progname, file, strerror(errno));
			return (1);
		}
	}

	(void)fclose(fp);
	free(p);
	return (0);
}

/*
 * run --
 *	Get a set of pages.
 */
int
run(hits, cachesize, pagesize, npages, progname)
	int hits, cachesize, pagesize, npages;
	const char *progname;
{
	DB_ENV *dbenv;
	DB_MPOOLFILE *mfp;
	db_pgno_t pageno;
	int cnt, ret;
	void *p;

	dbenv = NULL;
	mfp = NULL;

	printf("%s: cachesize: %d; pagesize: %d; N pages: %d\n",
	    progname, cachesize, pagesize, npages);

	/*
	 * Open a memory pool, specify a cachesize, output error messages
	 * to stderr.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (1);
	}
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);
#ifdef HAVE_VXWORKS
	if ((ret = dbenv->set_shm_key(dbenv, VXSHM_KEY)) != 0) {
		dbenv->err(dbenv, ret, "set_shm_key");
		return (1);
	}
#endif

	/* Set the cachesize. */
	if ((ret = dbenv->set_cachesize(dbenv, 0, cachesize, 0)) != 0) {
		dbenv->err(dbenv, ret, "set_cachesize");
		goto err;
	}

	/* Open the environment. */
	if ((ret = dbenv->open(
	    dbenv, NULL, DB_CREATE | DB_INIT_MPOOL, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto err;
	}

	/* Open the file in the environment. */
	if ((ret = dbenv->memp_fcreate(dbenv, &mfp, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->memp_fcreate: %s", MPOOL);
		goto err;
	}
	if ((ret = mfp->open(mfp, MPOOL, 0, 0, pagesize)) != 0) {
		dbenv->err(dbenv, ret, "DB_MPOOLFILE->open: %s", MPOOL);
		goto err;
	}

	printf("retrieve %d random pages... ", hits);

	srand((u_int)time(NULL));
	for (cnt = 0; cnt < hits; ++cnt) {
		pageno = rand() % npages;
		if ((ret = mfp->get(mfp, &pageno, NULL, 0, &p)) != 0) {
			dbenv->err(dbenv, ret,
			    "unable to retrieve page %lu", (u_long)pageno);
			goto err;
		}
		/* Verify the page's number that was written in init(). */
		if (*(db_pgno_t *)p != pageno) {
			dbenv->errx(dbenv,
			    "wrong page retrieved (%lu != %d)",
			    (u_long)pageno, *(int *)p);
			goto err;
		}
		if ((ret = mfp->put(mfp, p, DB_PRIORITY_UNCHANGED, 0)) != 0) {
			dbenv->err(dbenv, ret,
			    "unable to return page %lu", (u_long)pageno);
			goto err;
		}
	}

	printf("successful.\n");

	/* Close the file. */
	if ((ret = mfp->close(mfp, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_MPOOLFILE->close");
		goto err;
	}

	/* Close the pool. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (1);
	}
	return (0);

err:	if (mfp != NULL)
		(void)mfp->close(mfp, 0);
	if (dbenv != NULL)
		(void)dbenv->close(dbenv, 0);
	return (1);
}
