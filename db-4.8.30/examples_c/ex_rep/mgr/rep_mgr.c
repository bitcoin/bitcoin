/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <db.h>

#include "../common/rep_common.h"

typedef struct {
	SHARED_DATA shared_data;
} APP_DATA;

const char *progname = "ex_rep_mgr";

static void event_callback __P((DB_ENV *, u_int32_t, void *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	DB_ENV *dbenv;
	SETUP_DATA setup_info;
	repsite_t *site_list;
	APP_DATA my_app_data;
	thread_t ckp_thr, lga_thr;
	supthr_args sup_args;
	u_int32_t start_policy;
	int i, ret, t_ret;

	memset(&setup_info, 0, sizeof(SETUP_DATA));
	setup_info.progname = progname;
	memset(&my_app_data, 0, sizeof(APP_DATA));
	dbenv = NULL;
	ret = 0;

	start_policy = DB_REP_ELECTION;

	if ((ret = create_env(progname, &dbenv)) != 0)
		goto err;
	dbenv->app_private = &my_app_data;
	(void)dbenv->set_event_notify(dbenv, event_callback);

	/* Parse command line and perform common replication setup. */
	if ((ret = common_rep_setup(dbenv, argc, argv, &setup_info)) != 0)
		goto err;

	/* Perform repmgr-specific setup based on command line options. */
	if (setup_info.role == MASTER)
		start_policy = DB_REP_MASTER;
	else if (setup_info.role == CLIENT)
		start_policy = DB_REP_CLIENT;
	if ((ret = dbenv->repmgr_set_local_site(dbenv, setup_info.self.host,
	    setup_info.self.port, 0)) != 0) {
		fprintf(stderr, "Could not set listen address (%d).\n", ret);
		goto err;
	}
	site_list = setup_info.site_list;
	for (i = 0; i < setup_info.remotesites; i++) {
		if ((ret = dbenv->repmgr_add_remote_site(dbenv,
		    site_list[i].host, site_list[i].port, NULL,
		    site_list[i].peer ? DB_REPMGR_PEER : 0)) != 0) {
			dbenv->err(dbenv, ret,
			    "Could not add site %s:%d", site_list[i].host,
			    (int)site_list[i].port);
			goto err;
		}
	}

	/*
	 * Configure heartbeat timeouts so that repmgr monitors the
	 * health of the TCP connection.  Master sites broadcast a heartbeat
	 * at the frequency specified by the DB_REP_HEARTBEAT_SEND timeout.
	 * Client sites wait for message activity the length of the
	 * DB_REP_HEARTBEAT_MONITOR timeout before concluding that the
	 * connection to the master is lost.  The DB_REP_HEARTBEAT_MONITOR
	 * timeout should be longer than the DB_REP_HEARTBEAT_SEND timeout.
	 */
	if ((ret = dbenv->rep_set_timeout(dbenv, DB_REP_HEARTBEAT_SEND,
	    5000000)) != 0)
		dbenv->err(dbenv, ret,
		    "Could not set heartbeat send timeout.\n");
	if ((ret = dbenv->rep_set_timeout(dbenv, DB_REP_HEARTBEAT_MONITOR,
	    10000000)) != 0)
		dbenv->err(dbenv, ret,
		    "Could not set heartbeat monitor timeout.\n");

	/*
	 * The following repmgr features may also be useful to your
	 * application.  See Berkeley DB documentation for more details.
	 *  - Two-site strict majority rule - In a two-site replication
	 *    group, require both sites to be available to elect a new
	 *    master.
	 *  - Timeouts - Customize the amount of time repmgr waits
	 *    for such things as waiting for acknowledgements or attempting
	 *    to reconnect to other sites.
	 *  - Site list - return a list of sites currently known to repmgr.
	 */

	if ((ret = env_init(dbenv, setup_info.home)) != 0)
		goto err;

	/* Start checkpoint and log archive threads. */
	sup_args.dbenv = dbenv;
	sup_args.shared = &my_app_data.shared_data;
	if ((ret = start_support_threads(dbenv, &sup_args, &ckp_thr,
	    &lga_thr)) != 0)
		goto err;

	if ((ret = dbenv->repmgr_start(dbenv, 3, start_policy)) != 0)
		goto err;

	if ((ret = doloop(dbenv, &my_app_data.shared_data)) != 0) {
		dbenv->err(dbenv, ret, "Client failed");
		goto err;
	}

	/* Finish checkpoint and log archive threads. */
	if ((ret = finish_support_threads(&ckp_thr, &lga_thr)) != 0)
		goto err;

	/*
	 * We have used the DB_TXN_NOSYNC environment flag for improved
	 * performance without the usual sacrifice of transactional durability,
	 * as discussed in the "Transactional guarantees" page of the Reference
	 * Guide: if one replication site crashes, we can expect the data to
	 * exist at another site.  However, in case we shut down all sites
	 * gracefully, we push out the end of the log here so that the most
	 * recent transactions don't mysteriously disappear.
	 */
	if ((ret = dbenv->log_flush(dbenv, NULL)) != 0) {
		dbenv->err(dbenv, ret, "log_flush");
		goto err;
	}

err:
	if (dbenv != NULL &&
	    (t_ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "failure closing env: %s (%d)\n",
		    db_strerror(t_ret), t_ret);
		if (ret == 0)
			ret = t_ret;
	}

	return (ret);
}

static void
event_callback(dbenv, which, info)
	DB_ENV *dbenv;
	u_int32_t which;
	void *info;
{
	APP_DATA *app = dbenv->app_private;
	SHARED_DATA *shared = &app->shared_data;

	info = NULL;				/* Currently unused. */

	switch (which) {
	case DB_EVENT_REP_CLIENT:
		shared->is_master = 0;
		shared->in_client_sync = 1;
		break;

	case DB_EVENT_REP_MASTER:
		shared->is_master = 1;
		shared->in_client_sync = 0;
		break;

	case DB_EVENT_REP_NEWMASTER:
		shared->in_client_sync = 1;
		break;

	case DB_EVENT_REP_PERM_FAILED:
		/*
		 * Did not get enough acks to guarantee transaction
		 * durability based on the configured ack policy.  This
		 * transaction will be flushed to the master site's
		 * local disk storage for durability.
		 */
		printf(
    "Insufficient acknowledgements to guarantee transaction durability.\n");
		break;

	case DB_EVENT_REP_STARTUPDONE:
		shared->in_client_sync = 0;
		break;

	default:
		dbenv->errx(dbenv, "ignoring event %d", which);
	}
}
