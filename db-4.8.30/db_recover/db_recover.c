/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996-2009 Oracle.  All rights reserved.\n";
#endif

void db_recover_feedback __P((DB_ENV *, int, int));
int  main __P((int, char *[]));
int  read_timestamp __P((char *, time_t *));
int  usage __P((void));
int  version_check __P((void));

const char *progname;
int newline_needed;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB_ENV	*dbenv;
	time_t timestamp;
	u_int32_t flags;
	int ch, exitval, fatal_recover, ret, retain_env, set_feedback, verbose;
	char *home, *passwd;

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = version_check()) != 0)
		return (ret);

	home = passwd = NULL;
	timestamp = 0;
	exitval = fatal_recover = retain_env = set_feedback = verbose = 0;
	while ((ch = getopt(argc, argv, "cefh:P:t:Vv")) != EOF)
		switch (ch) {
		case 'c':
			fatal_recover = 1;
			break;
		case 'e':
			retain_env = 1;
			break;
		case 'f':
			set_feedback = 1;
			break;
		case 'h':
			home = optarg;
			break;
		case 'P':
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, "%s: strdup: %s\n",
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 't':
			if ((ret = read_timestamp(optarg, &timestamp)) != 0)
				return (ret);
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	/* Handle possible interruptions. */
	__db_util_siginit();

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (EXIT_FAILURE);
	}
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);
	if (set_feedback)
		(void)dbenv->set_feedback(dbenv, db_recover_feedback);
	if (verbose)
		(void)dbenv->set_verbose(dbenv, DB_VERB_RECOVERY, 1);
	if (timestamp &&
	    (ret = dbenv->set_tx_timestamp(dbenv, &timestamp)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_timestamp");
		goto shutdown;
	}

	if (passwd != NULL && (ret = dbenv->set_encrypt(dbenv,
	    passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		goto shutdown;
	}

	/*
	 * Initialize the environment -- we don't actually do anything
	 * else, that all that's needed to run recovery.
	 *
	 * Note that unless the caller specified the -e option, we use a
	 * private environment, as we're about to create a region, and we
	 * don't want to to leave it around.  If we leave the region around,
	 * the application that should create it will simply join it instead,
	 * and will then be running with incorrectly sized (and probably
	 * terribly small) caches.  Applications that use -e should almost
	 * certainly use DB_CONFIG files in the directory.
	 */
	flags = 0;
	LF_SET(DB_CREATE | DB_INIT_LOG |
	    DB_INIT_MPOOL | DB_INIT_TXN | DB_USE_ENVIRON);
	LF_SET(fatal_recover ? DB_RECOVER_FATAL : DB_RECOVER);
	LF_SET(retain_env ? DB_INIT_LOCK : DB_PRIVATE);
	if ((ret = dbenv->open(dbenv, home, flags, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto shutdown;
	}

	if (0) {
shutdown:	exitval = 1;
	}

	/* Flush to the next line of the output device. */
	if (newline_needed)
		printf("\n");

	/* Clean up the environment. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}
	if (passwd != NULL)
		free(passwd);

	/* Resend any caught signal. */
	__db_util_sigresend();

	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * db_recover_feedback --
 *	Provide feedback on recovery progress.
 */
void
db_recover_feedback(dbenv, opcode, percent)
	DB_ENV *dbenv;
	int opcode;
	int percent;
{
	COMPQUIET(dbenv, NULL);

	if (opcode == DB_RECOVER) {
		printf("\rrecovery %d%% complete", percent);
		(void)fflush(stdout);
		newline_needed = 1;
	}
}

#define	ATOI2(ar)	((ar)[0] - '0') * 10 + ((ar)[1] - '0'); (ar) += 2;

/*
 * read_timestamp --
 *	Convert a time argument to Epoch seconds.
 *
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
int
read_timestamp(arg, timep)
	char *arg;
	time_t *timep;
{
	struct tm *t;
	time_t now;
	int yearset;
	char *p;
					/* Start with the current time. */
	(void)time(&now);
	if ((t = localtime(&now)) == NULL) {
		fprintf(stderr,
		    "%s: localtime: %s\n", progname, strerror(errno));
		return (EXIT_FAILURE);
	}
					/* [[CC]YY]MMDDhhmm[.SS] */
	if ((p = strchr(arg, '.')) == NULL)
		t->tm_sec = 0;		/* Seconds defaults to 0. */
	else {
		if (strlen(p + 1) != 2)
			goto terr;
		*p++ = '\0';
		t->tm_sec = ATOI2(p);
	}

	yearset = 0;
	switch (strlen(arg)) {
	case 12:			/* CCYYMMDDhhmm */
		t->tm_year = ATOI2(arg);
		t->tm_year *= 100;
		yearset = 1;
		/* FALLTHROUGH */
	case 10:			/* YYMMDDhhmm */
		if (yearset) {
			yearset = ATOI2(arg);
			t->tm_year += yearset;
		} else {
			yearset = ATOI2(arg);
			if (yearset < 69)
				t->tm_year = yearset + 2000;
			else
				t->tm_year = yearset + 1900;
		}
		t->tm_year -= 1900;	/* Convert to UNIX time. */
		/* FALLTHROUGH */
	case 8:				/* MMDDhhmm */
		t->tm_mon = ATOI2(arg);
		--t->tm_mon;		/* Convert from 01-12 to 00-11 */
		t->tm_mday = ATOI2(arg);
		t->tm_hour = ATOI2(arg);
		t->tm_min = ATOI2(arg);
		break;
	default:
		goto terr;
	}

	t->tm_isdst = -1;		/* Figure out DST. */

	*timep = mktime(t);
	if (*timep == -1) {
terr:		fprintf(stderr,
	"%s: out of range or illegal time specification: [[CC]YY]MMDDhhmm[.SS]",
		    progname);
		return (EXIT_FAILURE);
	}
	return (0);
}

int
usage()
{
	(void)fprintf(stderr, "usage: %s %s\n", progname,
	    "[-cefVv] [-h home] [-P password] [-t [[CC]YY]MMDDhhmm[.SS]]");
	return (EXIT_FAILURE);
}

int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr,
	"%s: version %d.%d doesn't match library version %d.%d\n",
		    progname, DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}
