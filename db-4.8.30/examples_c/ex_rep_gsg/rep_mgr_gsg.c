/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * NOTE: This example is a simplified version of the rep_mgr.c
 * example that can be found in the db/examples_c/ex_rep/mgr directory.
 *
 * This example is intended only as an aid in learning Replication Manager
 * concepts.  It is not complete in that many features are not exercised
 * in it, nor are many error conditions properly handled.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <db.h>

#ifdef _WIN32
extern int getopt(int, char * const *, const char *);
#endif

#define	CACHESIZE (10 * 1024 * 1024)
#define	DATABASE "quote.db"
#define	SLEEPTIME 3

typedef struct {
    int is_master;
} APP_DATA;

const char *progname = "ex_rep_gsg_repmgr";

int create_env(const char *, DB_ENV **);
int env_init(DB_ENV *, const char *);
int doloop (DB_ENV *);
int print_stocks(DB *);
static void event_callback(DB_ENV *, u_int32_t, void *);

/* Usage function */
static void
usage()
{
    fprintf(stderr, "usage: %s ", progname);
    fprintf(stderr, "-h home -l host:port -n nsites\n");
    fprintf(stderr, "\t\t[-r host:port][-p priority]\n");
    fprintf(stderr, "where:\n");
    fprintf(stderr, "\t-h identifies the environment home directory ");
    fprintf(stderr, "(required).\n");
    fprintf(stderr, "\t-l identifies the host and port used by this ");
    fprintf(stderr, "site (required).\n");
    fprintf(stderr, "\t-n identifies the number of sites in this ");
    fprintf(stderr, "replication group (required).\n");
    fprintf(stderr, "\t-r identifies another site participating in "); 
    fprintf(stderr, "this replication group\n");
    fprintf(stderr, "\t-p identifies the election priority used by ");
    fprintf(stderr, "this replica.\n");
    exit(EXIT_FAILURE);
} 

int
main(int argc, char *argv[])
{
    DB_ENV *dbenv;
    extern char *optarg;
    const char *home;
    char ch, *host, *portstr;
    int local_is_set, ret, totalsites;
    u_int16_t port;
    /* Used to track whether this is a replica or a master. */
    APP_DATA my_app_data;

    dbenv = NULL;
    ret = local_is_set = totalsites = 0;
    home = NULL;

    my_app_data.is_master = 0;  /* Assume that we start as a replica */

    if ((ret = create_env(progname, &dbenv)) != 0)
	goto err;

    /* Make APP_DATA available through the environment handle. */
    dbenv->app_private = &my_app_data;

    /* Default priority is 100. */
    dbenv->rep_set_priority(dbenv, 100);
    /* Permanent messages require at least one ack. */
    dbenv->repmgr_set_ack_policy(dbenv, DB_REPMGR_ACKS_ONE);
    /* Give 500 microseconds to receive the ack. */
    dbenv->rep_set_timeout(dbenv, DB_REP_ACK_TIMEOUT, 500);

    /* Collect the command line options. */
    while ((ch = getopt(argc, argv, "h:l:n:p:r:")) != EOF)
	switch (ch) {
	case 'h':
	    home = optarg;
	    break;
	/* Set the host and port used by this environment. */
	case 'l':
	    host = strtok(optarg, ":");
	    if ((portstr = strtok(NULL, ":")) == NULL) {
		fprintf(stderr, "Bad host specification.\n");
		goto err;
	    }
	    port = (unsigned short)atoi(portstr);
	    if (dbenv->repmgr_set_local_site(dbenv, host, port, 0) != 0) {
		fprintf(stderr,
		    "Could not set local address %s.\n", host);
		goto err;
	    }
	    local_is_set = 1;
	    break;
	/* Set the number of sites in this replication group. */
	case 'n':
	    totalsites = atoi(optarg);
	    if ((ret = dbenv->rep_set_nsites(dbenv, totalsites)) != 0)
		dbenv->err(dbenv, ret, "set_nsites");
	    break;
	/* Set this replica's election priority. */
	case 'p':
	    dbenv->rep_set_priority(dbenv, atoi(optarg));
	    break;
	/* Identify another site in the replication group. */
	case 'r':
	    host = strtok(optarg, ":");
	    if ((portstr = strtok(NULL, ":")) == NULL) {
		fprintf(stderr, "Bad host specification.\n");
		goto err;
	    }
	    port = (unsigned short)atoi(portstr);
	    if (dbenv->repmgr_add_remote_site(dbenv, host, port, 0, 0) != 0) {
		fprintf(stderr,
		    "Could not add site %s.\n", host);
		goto err;
	    }
	    break;
	case '?':
	default:
	    usage();
	}

    /* Error check command line. */
    if (home == NULL || !local_is_set || !totalsites)
	usage();

    if ((ret = env_init(dbenv, home)) != 0)
	goto err;

    if ((ret = dbenv->repmgr_start(dbenv, 3, DB_REP_ELECTION)) != 0)
	goto err;

    if ((ret = doloop(dbenv)) != 0) {
	dbenv->err(dbenv, ret, "Application failed");
	goto err;
    }

err: if (dbenv != NULL)
	(void)dbenv->close(dbenv, 0);

    return (ret);

}

/* Create and configure an environment handle. */
int
create_env(const char *progname, DB_ENV **dbenvp)
{
    DB_ENV *dbenv;
    int ret;

    if ((ret = db_env_create(&dbenv, 0)) != 0) {
	fprintf(stderr, "can't create env handle: %s\n",
	    db_strerror(ret));
	return (ret);
    }

    dbenv->set_errfile(dbenv, stderr);
    dbenv->set_errpfx(dbenv, progname);
    (void)dbenv->set_event_notify(dbenv, event_callback);

    *dbenvp = dbenv;
    return (0);
}

/* Open and configure an environment. */
int
env_init(DB_ENV *dbenv, const char *home)
{
    u_int32_t flags;
    int ret;

    (void)dbenv->set_cachesize(dbenv, 0, CACHESIZE, 0);
    (void)dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1);

    flags = DB_CREATE |
	DB_INIT_LOCK | 
	DB_INIT_LOG |
	DB_INIT_MPOOL |
	DB_INIT_REP |
	DB_INIT_TXN |
	DB_RECOVER |
	DB_THREAD;
    if ((ret = dbenv->open(dbenv, home, flags, 0)) != 0)
	dbenv->err(dbenv, ret, "can't open environment");
    return (ret);
}

/*
 * A callback used to determine whether the local environment is a
 * replica or a master. This is called by the Replication Manager
 * when the local environment changes state.
 */
static void
event_callback(DB_ENV *dbenv, u_int32_t which, void *info)
{
    APP_DATA *app = dbenv->app_private;

    info = NULL;                /* Currently unused. */

    switch (which) {
    case DB_EVENT_REP_MASTER:
	app->is_master = 1;
	break;

    case DB_EVENT_REP_CLIENT:
	app->is_master = 0;
	break;

    case DB_EVENT_REP_STARTUPDONE: /* FALLTHROUGH */
    case DB_EVENT_REP_NEWMASTER:
	/* Ignore. */
	break;

    default:
	dbenv->errx(dbenv, "ignoring event %d", which);
    }
}

/*
 * Provides the main data processing function for our application.
 * This function provides a command line prompt to which the user
 * can provide a ticker string and a stock price.  Once a value is
 * entered to the application, the application writes the value to
 * the database and then displays the entire database.
 */
#define	   BUFSIZE 1024

int
doloop(DB_ENV *dbenv)
{
    DB *dbp;
    APP_DATA *app_data;
    DBT key, data;
    char buf[BUFSIZE], *rbuf;
    int ret;
    u_int32_t flags;

    dbp = NULL;
    ret = 0;
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    app_data = dbenv->app_private;

    for (;;) {
	if (dbp == NULL) {
	    if ((ret = db_create(&dbp, dbenv, 0)) != 0)
		return (ret);

	    flags = DB_AUTO_COMMIT;
	    if (app_data->is_master)
		flags |= DB_CREATE;
	    if ((ret = dbp->open(dbp,
		NULL, DATABASE, NULL, DB_BTREE, flags, 0)) != 0) {
		if (ret == ENOENT) {
		    printf(
		      "No stock database yet available.\n");
		    if ((ret = dbp->close(dbp, 0)) != 0) {
			dbenv->err(dbenv, ret, "DB->close");
			goto err;
		    }
		    dbp = NULL;
		    sleep(SLEEPTIME);
		    continue;
		}
		dbenv->err(dbenv, ret, "DB->open");
		goto err;
	    }
	}

	printf("QUOTESERVER%s> ",
	    app_data->is_master ? "" : " (read-only)");
	fflush(stdout);

	if (fgets(buf, sizeof(buf), stdin) == NULL)
	    break;
	if (strtok(&buf[0], " \t\n") == NULL) {
	    switch ((ret = print_stocks(dbp))) {
	    case 0:
		continue;
	    case DB_REP_HANDLE_DEAD:
		(void)dbp->close(dbp, DB_NOSYNC);
		dbp = NULL;
                dbenv->errx(dbenv, "Got a dead replication handle");
		continue;
	    default:
		dbp->err(dbp, ret, "Error traversing data");
		goto err;
	    }
	}
	rbuf = strtok(NULL, " \t\n");
	if (rbuf == NULL || rbuf[0] == '\0') {
	    if (strncmp(buf, "exit", 4) == 0 ||
		strncmp(buf, "quit", 4) == 0)
		break;
	    dbenv->errx(dbenv, "Format: TICKER VALUE");
	    continue;
	}

	if (!app_data->is_master) {
	    dbenv->errx(dbenv, "Can't update at client");
	    continue;
	}

	key.data = buf;
	key.size = (u_int32_t)strlen(buf);

	data.data = rbuf;
	data.size = (u_int32_t)strlen(rbuf);

	if ((ret = dbp->put(dbp,
	    NULL, &key, &data, 0)) != 0) {
	    dbp->err(dbp, ret, "DB->put");
	    goto err;
	}
    }

err: if (dbp != NULL)
	(void)dbp->close(dbp, DB_NOSYNC);

    return (ret);
}

/* Display all the stock quote information in the database. */
int
print_stocks(DB *dbp)
{
    DBC *dbc;
    DBT key, data;
#define	   MAXKEYSIZE    10
#define	   MAXDATASIZE    20
    char keybuf[MAXKEYSIZE + 1], databuf[MAXDATASIZE + 1];
    int ret, t_ret;
    u_int32_t keysize, datasize;

    if ((ret = dbp->cursor(dbp, NULL, &dbc, 0)) != 0) {
	dbp->err(dbp, ret, "can't open cursor");
	return (ret);
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    printf("\tSymbol\tPrice\n");
    printf("\t======\t=====\n");

    for (ret = dbc->c_get(dbc, &key, &data, DB_FIRST);
	ret == 0;
	ret = dbc->c_get(dbc, &key, &data, DB_NEXT)) {
	keysize = key.size > MAXKEYSIZE ? MAXKEYSIZE : key.size;
	memcpy(keybuf, key.data, keysize);
	keybuf[keysize] = '\0';

	datasize = data.size >= MAXDATASIZE ? MAXDATASIZE : data.size;
	memcpy(databuf, data.data, datasize);
	databuf[datasize] = '\0';

	printf("\t%s\t%s\n", keybuf, databuf);
    }
    printf("\n");
    fflush(stdout);

    if ((t_ret = dbc->c_close(dbc)) != 0 && ret == 0)
	ret = t_ret;

    switch (ret) {
    case 0:
    case DB_NOTFOUND:
	return (0);
    case DB_LOCK_DEADLOCK:
	return (0);
    default:
	return (ret);
    }
}

