/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <db.h>

#include "rep_common.h"

#define	CACHESIZE	(10 * 1024 * 1024)
#define	DATABASE	"quote.db"
#define	SLEEPTIME	3

static int print_stocks __P((DB *));

/*
 * Perform command line parsing and common replication setup for the repmgr
 * and base replication example programs.
 */
int
common_rep_setup(dbenv, argc, argv, setup_info)
	DB_ENV *dbenv;
	int argc;
	char *argv[];
	SETUP_DATA *setup_info;
{
	repsite_t site;
	extern char *optarg;
	char ch, *portstr;
	int ack_policy, got_self, is_repmgr, maxsites, priority, ret;

	got_self = is_repmgr = maxsites = ret = site.peer = 0;

	priority = 100;
	ack_policy = DB_REPMGR_ACKS_QUORUM;
	setup_info->role = UNKNOWN;
	if (strncmp(setup_info->progname, "ex_rep_mgr", 10) == 0)
		is_repmgr = 1;

	/*
	 * Replication setup calls that are only needed if a command
	 * line option is specified are made within this while/switch
	 * statement.  Replication setup calls that should be made
	 * whether or not a command line option is specified are after
	 * this while/switch statement.
	 */
	while ((ch = getopt(argc, argv, "a:bCh:l:Mn:p:R:r:v")) != EOF) {
		switch (ch) {
		case 'a':
			if (!is_repmgr)
				usage(is_repmgr, setup_info->progname);
			if (strncmp(optarg, "all", 3) == 0)
				ack_policy = DB_REPMGR_ACKS_ALL;
			else if (strncmp(optarg, "quorum", 6) != 0)
				usage(is_repmgr, setup_info->progname);
			break;
		case 'b':
			/*
			 * Configure bulk transfer to send groups of records
			 * to clients in a single network transfer.  This is
			 * useful for master sites and clients participating
			 * in client-to-client synchronization.
			 */
			if ((ret = dbenv->rep_set_config(dbenv,
			    DB_REP_CONF_BULK, 1)) != 0) {
				dbenv->err(dbenv, ret,
				    "Could not configure bulk transfer.\n");
				goto err;
			}
			break;
		case 'C':
			setup_info->role = CLIENT;
			break;
		case 'h':
			setup_info->home = optarg;
			break;
		case 'l':
			setup_info->self.host = strtok(optarg, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			setup_info->self.port = (unsigned short)atoi(portstr);
			setup_info->self.peer = 0;
			got_self = 1;
			break;
		case 'M':
			setup_info->role = MASTER;
			break;
		case 'n':
			setup_info->nsites = atoi(optarg);
			/*
			 * For repmgr, set the total number of sites in the
			 * replication group.  This is used by repmgr internal
			 * election processing.  For base replication, nsites
			 * is simply passed back to main for use in its
			 * communications and election processing.
			 */
			if (is_repmgr && setup_info->nsites > 0 &&
			    (ret = dbenv->rep_set_nsites(dbenv,
			    setup_info->nsites)) != 0) {
				dbenv->err(dbenv, ret,
				    "Could not set nsites.\n");
				goto err;
			}
			break;
		case 'p':
			priority = atoi(optarg);
			break;
		case 'R':
			if (!is_repmgr)
				usage(is_repmgr, setup_info->progname);
			site.peer = 1; /* FALLTHROUGH */
		case 'r':
			site.host = optarg;
			site.host = strtok(site.host, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			site.port = (unsigned short)atoi(portstr);
			if (setup_info->site_list == NULL ||
			    setup_info->remotesites >= maxsites) {
				maxsites = maxsites == 0 ? 10 : 2 * maxsites;
				if ((setup_info->site_list =
				    realloc(setup_info->site_list,
				    maxsites * sizeof(repsite_t))) == NULL) {
					fprintf(stderr, "System error %s\n",
					    strerror(errno));
					goto err;
				}
			}
			(setup_info->site_list)[(setup_info->remotesites)++] =
				site;
			site.peer = 0;
			break;
		case 'v':
			if ((ret = dbenv->set_verbose(dbenv,
			    DB_VERB_REPLICATION, 1)) != 0)
				goto err;
			break;
		case '?':
		default:
			usage(is_repmgr, setup_info->progname);
		}
	}

	/* Error check command line. */
	if (!got_self || setup_info->home == NULL)
		usage(is_repmgr, setup_info->progname);
	if (!is_repmgr && setup_info->role == UNKNOWN) {
		fprintf(stderr, "Must specify -M or -C.\n");
		goto err;
	}

	/*
	 * Set replication group election priority for this environment.
	 * An election first selects the site with the most recent log
	 * records as the new master.  If multiple sites have the most
	 * recent log records, the site with the highest priority value
	 * is selected as master.
	 */
	if ((ret = dbenv->rep_set_priority(dbenv, priority)) != 0) {
		dbenv->err(dbenv, ret, "Could not set priority.\n");
		goto err;
	}

	/*
	 * For repmgr, set the policy that determines how master and client
	 * sites handle acknowledgement of replication messages needed for
	 * permanent records.  The default policy of "quorum" requires only
	 * a quorum of electable peers sufficient to ensure a permanent
	 * record remains durable if an election is held.  The "all" option
	 * requires all clients to acknowledge a permanent replication
	 * message instead.
	 */
	if (is_repmgr &&
	    (ret = dbenv->repmgr_set_ack_policy(dbenv, ack_policy)) != 0) {
		dbenv->err(dbenv, ret, "Could not set ack policy.\n");
		goto err;
	}

	/*
	 * Set the threshold for the minimum and maximum time the client
	 * waits before requesting retransmission of a missing message.
	 * Base these values on the performance and load characteristics
	 * of the master and client host platforms as well as the round
	 * trip message time.
	 */
	if ((ret = dbenv->rep_set_request(dbenv, 20000, 500000)) != 0) {
		dbenv->err(dbenv, ret,
		    "Could not set client_retransmission defaults.\n");
		goto err;
	}

	/*
	 * Configure deadlock detection to ensure that any deadlocks
	 * are broken by having one of the conflicting lock requests
	 * rejected. DB_LOCK_DEFAULT uses the lock policy specified
	 * at environment creation time or DB_LOCK_RANDOM if none was
	 * specified.
	 */
	if ((ret = dbenv->set_lk_detect(dbenv, DB_LOCK_DEFAULT)) != 0) {
		dbenv->err(dbenv, ret,
		    "Could not configure deadlock detection.\n");
		goto err;
	}

	/* The following base replication features may also be useful to your
	 * application. See Berkeley DB documentation for more details.
	 *   - Master leases: Provide stricter consistency for data reads
	 *     on a master site.
	 *   - Timeouts: Customize the amount of time Berkeley DB waits
	 *     for such things as an election to be concluded or a master
	 *     lease to be granted.
	 *   - Delayed client synchronization: Manage the master site's
	 *     resources by spreading out resource-intensive client
	 *     synchronizations.
	 *   - Blocked client operations: Return immediately with an error
	 *     instead of waiting indefinitely if a client operation is
	 *     blocked by an ongoing client synchronization.
	 */

err:
	return (ret);
}

static int
print_stocks(dbp)
	DB *dbp;
{
	DBC *dbc;
	DBT key, data;
#define	MAXKEYSIZE	10
#define	MAXDATASIZE	20
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

	for (ret = dbc->get(dbc, &key, &data, DB_FIRST);
	    ret == 0;
	    ret = dbc->get(dbc, &key, &data, DB_NEXT)) {
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

	if ((t_ret = dbc->close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	switch (ret) {
	case 0:
	case DB_NOTFOUND:
	case DB_LOCK_DEADLOCK:
		return (0);
	default:
		return (ret);
	}
}

/* Start checkpoint and log archive support threads. */
int
start_support_threads(dbenv, sup_args, ckp_thr, lga_thr)
	DB_ENV *dbenv;
	supthr_args *sup_args;
	thread_t *ckp_thr;
	thread_t *lga_thr;
{
	int ret;

	ret = 0;
	if ((ret = thread_create(ckp_thr, NULL, checkpoint_thread,
	    sup_args)) != 0) {
		dbenv->errx(dbenv, "can't create checkpoint thread");
		goto err;
	}
	if ((ret = thread_create(lga_thr, NULL, log_archive_thread,
	    sup_args)) != 0)
		dbenv->errx(dbenv, "can't create log archive thread");
err:
	return (ret);

}

/* Wait for checkpoint and log archive support threads to finish. */
int
finish_support_threads(ckp_thr, lga_thr)
	thread_t *ckp_thr;
	thread_t *lga_thr;
{
	void *ctstatus, *ltstatus;
	int ret;

	ret = 0;
	if (thread_join(*lga_thr, &ltstatus) ||
	    thread_join(*ckp_thr, &ctstatus)) {
		ret = -1;
		goto err;
	}
	if ((uintptr_t)ltstatus != EXIT_SUCCESS ||
	    (uintptr_t)ctstatus != EXIT_SUCCESS)
		ret = -1;
err:
	return (ret);
}

#define	BUFSIZE 1024

int
doloop(dbenv, shared_data)
	DB_ENV *dbenv;
	SHARED_DATA *shared_data;
{
	DB *dbp;
	DBT key, data;
	char buf[BUFSIZE], *first, *price;
	u_int32_t flags;
	int ret;

	dbp = NULL;
	ret = 0;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	for (;;) {
		printf("QUOTESERVER%s> ",
		    shared_data->is_master ? "" : " (read-only)");
		fflush(stdout);

		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;

#define	DELIM " \t\n"
		if ((first = strtok(&buf[0], DELIM)) == NULL) {
			/* Blank input line. */
			price = NULL;
		} else if ((price = strtok(NULL, DELIM)) == NULL) {
			/* Just one input token. */
			if (strncmp(buf, "exit", 4) == 0 ||
			    strncmp(buf, "quit", 4) == 0) {
				/*
				 * This makes the checkpoint and log
				 * archive threads stop.
				 */
				shared_data->app_finished = 1;
				break;
			}
			dbenv->errx(dbenv, "Format: TICKER VALUE");
			continue;
		} else {
			/* Normal two-token input line. */
			if (first != NULL && !shared_data->is_master) {
				dbenv->errx(dbenv, "Can't update at client");
				continue;
			}
		}

		if (dbp == NULL) {
			if ((ret = db_create(&dbp, dbenv, 0)) != 0)
				return (ret);

			flags = DB_AUTO_COMMIT;
			/*
			 * Open database with DB_CREATE only if this is
			 * a master database.  A client database uses
			 * polling to attempt to open the database without
			 * DB_CREATE until it is successful. 
			 *
			 * This DB_CREATE polling logic can be simplified
			 * under some circumstances.  For example, if the
			 * application can be sure a database is already
			 * there, it would never need to open it with
			 * DB_CREATE.
			 */
			if (shared_data->is_master)
				flags |= DB_CREATE;
			if ((ret = dbp->open(dbp,
			    NULL, DATABASE, NULL, DB_BTREE, flags, 0)) != 0) {
				if (ret == ENOENT) {
					printf(
					  "No stock database yet available.\n");
					if ((ret = dbp->close(dbp, 0)) != 0) {
						dbenv->err(dbenv, ret,
						    "DB->close");
						goto err;
					}
					dbp = NULL;
					continue;
				}
				if (ret == DB_REP_HANDLE_DEAD ||
				    ret == DB_LOCK_DEADLOCK) {
					dbenv->err(dbenv, ret,
					    "please retry the operation");
					dbp->close(dbp, DB_NOSYNC);
					dbp = NULL;
					continue;
				}
				dbenv->err(dbenv, ret, "DB->open");
				goto err;
			}
		}

		if (first == NULL) {
			/*
			 * If this is a client in the middle of
			 * synchronizing with the master, the client data
			 * is possibly stale and won't be displayed until
			 * client synchronization is finished.  It is also
			 * possible to display the stale data if this is
			 * acceptable to the application.
			 */
			if (shared_data->in_client_sync)
				printf(
"Cannot read data during client synchronization - please try again.\n");
			else
				switch ((ret = print_stocks(dbp))) {
				case 0:
					break;
				case DB_REP_HANDLE_DEAD:
					(void)dbp->close(dbp, DB_NOSYNC);
					dbp = NULL;
					break;
				default:
					dbp->err(dbp, ret,
					    "Error traversing data");
					goto err;
				}
		} else {
			key.data = first;
			key.size = (u_int32_t)strlen(first);

			data.data = price;
			data.size = (u_int32_t)strlen(price);

			if ((ret = dbp->put(dbp,
				 NULL, &key, &data, DB_AUTO_COMMIT)) != 0) {
				dbp->err(dbp, ret, "DB->put");
				goto err;
			}
		}
	}

err:	if (dbp != NULL)
		(void)dbp->close(dbp, DB_NOSYNC);
	return (ret);
}

int
create_env(progname, dbenvp)
	const char *progname;
	DB_ENV **dbenvp;
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

	*dbenvp = dbenv;
	return (0);
}

/* Open and configure an environment. */
int
env_init(dbenv, home)
	DB_ENV *dbenv;
	const char *home;
{
	u_int32_t flags;
	int ret;

	(void)dbenv->set_cachesize(dbenv, 0, CACHESIZE, 0);
	(void)dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1);

	flags = DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL |
	    DB_INIT_REP | DB_INIT_TXN | DB_RECOVER | DB_THREAD;
	if ((ret = dbenv->open(dbenv, home, flags, 0)) != 0)
		dbenv->err(dbenv, ret, "can't open environment");
	return (ret);
}

/*
 * In this application, we specify all communication via the command line.  In
 * a real application, we would expect that information about the other sites
 * in the system would be maintained in some sort of configuration file.  The
 * critical part of this interface is that we assume at startup that we can
 * find out
 *	1) what host/port we wish to listen on for connections,
 *	2) a (possibly empty) list of other sites we should attempt to connect
 *	to; and
 *	3) what our Berkeley DB home environment is.
 *
 * These pieces of information are expressed by the following flags.
 * -a all|quorum (optional; repmgr only, a stands for ack policy)
 * -b (optional, b stands for bulk)
 * -C or -M start up as client or master (optional for repmgr, required
 *      for base example)
 * -h home directory (required)
 * -l host:port (required; l stands for local)
 * -n nsites (optional; number of sites in replication group; defaults to 0
 *	in which case we try to dynamically compute the number of sites in
 *	the replication group)
 * -p priority (optional: defaults to 100)
 * -r host:port (optional; r stands for remote; any number of these may be
 *	specified)
 * -R host:port (optional; repmgr only, remote peer)
 * -v (optional; v stands for verbose)
 */
void
usage(is_repmgr, progname)
	const int is_repmgr;
	const char *progname;
{
	fprintf(stderr, "usage: %s ", progname);
	if (is_repmgr)
		fprintf(stderr, "[-CM]-h home -l host:port[-r host:port]%s%s",
		    "[-R host:port][-a all|quorum][-b][-n nsites]",
		    "[-p priority][-v]\n");
	else
		fprintf(stderr, "-CM -h home -l host:port[-r host:port]%s",
		    "[-b][-n nsites][-p priority][-v]\n");
	exit(EXIT_FAILURE);
}

/*
 * This is a very simple thread that performs checkpoints at a fixed
 * time interval.  For a master site, the time interval is one minute
 * plus the duration of the checkpoint_delay timeout (30 seconds by
 * default.)  For a client site, the time interval is one minute.
 */
void *
checkpoint_thread(args)
	void *args;
{
	DB_ENV *dbenv;
	SHARED_DATA *shared;
	supthr_args *ca;
	int i, ret;

	ca = (supthr_args *)args;
	dbenv = ca->dbenv;
	shared = ca->shared;

	for (;;) {
		/*
		 * Wait for one minute, polling once per second to see if
		 * application has finished.  When application has finished,
		 * terminate this thread.
		 */
		for (i = 0; i < 60; i++) {
			sleep(1);
			if (shared->app_finished == 1)
				return ((void *)EXIT_SUCCESS);
		}

		/* Perform a checkpoint. */
		if ((ret = dbenv->txn_checkpoint(dbenv, 0, 0, 0)) != 0) {
			dbenv->err(dbenv, ret,
			    "Could not perform checkpoint.\n");
			return ((void *)EXIT_FAILURE);
		}
	}
}

/*
 * This is a simple log archive thread.  Once per minute, it removes all but
 * the most recent 3 logs that are safe to remove according to a call to
 * DB_ENV->log_archive().
 *
 * Log cleanup is needed to conserve disk space, but aggressive log cleanup
 * can cause more frequent client initializations if a client lags too far
 * behind the current master.  This can happen in the event of a slow client,
 * a network partition, or a new master that has not kept as many logs as the
 * previous master.
 *
 * The approach in this routine balances the need to mitigate against a
 * lagging client by keeping a few more of the most recent unneeded logs
 * with the need to conserve disk space by regularly cleaning up log files.
 * Use of automatic log removal (DB_ENV->log_set_config() DB_LOG_AUTO_REMOVE
 * flag) is not recommended for replication due to the risk of frequent
 * client initializations.
 */
void *
log_archive_thread(args)
	void *args;
{
	DB_ENV *dbenv;
	SHARED_DATA *shared;
	char **begin, **list;
	supthr_args *la;
	int i, listlen, logs_to_keep, minlog, ret;

	la = (supthr_args *)args;
	dbenv = la->dbenv;
	shared = la->shared;
	logs_to_keep = 3;

	for (;;) {
		/*
		 * Wait for one minute, polling once per second to see if
		 * application has finished.  When application has finished,
		 * terminate this thread.
		 */
		for (i = 0; i < 60; i++) {
			sleep(1);
			if (shared->app_finished == 1)
				return ((void *)EXIT_SUCCESS);
		}

		/* Get the list of unneeded log files. */
		if ((ret = dbenv->log_archive(dbenv, &list, DB_ARCH_ABS))
		    != 0) {
			dbenv->err(dbenv, ret,
			    "Could not get log archive list.");
			return ((void *)EXIT_FAILURE);
		}
		if (list != NULL) {
			listlen = 0;
			/* Get the number of logs in the list. */
			for (begin = list; *begin != NULL; begin++, listlen++);
			/*
			 * Remove all but the logs_to_keep most recent
			 * unneeded log files.
			 */
			minlog = listlen - logs_to_keep;
			for (begin = list, i= 0; i < minlog; list++, i++) {
				if ((ret = unlink(*list)) != 0) {
					dbenv->err(dbenv, ret,
					    "logclean: remove %s", *list);
					dbenv->errx(dbenv,
					    "logclean: Error remove %s", *list);
					free(begin);
					return ((void *)EXIT_FAILURE);
				}
			}
			free(begin);
		}
	}
}
