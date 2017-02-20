/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <db.h>

#include "rep_base.h"

/*
 * Process globals (we could put these in the machtab I suppose).
 */
int master_eid;
char *myaddr;
unsigned short myport;

const char *progname = "ex_rep_base";

static void event_callback __P((DB_ENV *, u_int32_t, void *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	DB_ENV *dbenv;
	SETUP_DATA setup_info;
	DBT local;
	all_args aa;
	connect_args ca;
	supthr_args supa;
	machtab_t *machtab;
	thread_t all_thr, ckp_thr, conn_thr, lga_thr;
	void *astatus, *cstatus;
#ifdef _WIN32
	WSADATA wsaData;
#else
	struct sigaction sigact;
#endif
	APP_DATA my_app_data;
	int ret;

	memset(&setup_info, 0, sizeof(SETUP_DATA));
	setup_info.progname = progname;
	master_eid = DB_EID_INVALID;
	memset(&my_app_data, 0, sizeof(APP_DATA));
	dbenv = NULL;
	machtab = NULL;
	ret = 0;

	if ((ret = create_env(progname, &dbenv)) != 0)
		goto err;
	dbenv->app_private = &my_app_data;
	(void)dbenv->set_event_notify(dbenv, event_callback);

	/* Parse command line and perform common replication setup. */
	if ((ret = common_rep_setup(dbenv, argc, argv, &setup_info)) != 0)
		goto err;

	if (setup_info.role == MASTER)
		master_eid = SELF_EID;

	myaddr = strdup(setup_info.self.host);
	myport = setup_info.self.port;

#ifdef _WIN32
	/* Initialize the Windows sockets DLL. */
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		fprintf(stderr,
		    "Unable to initialize Windows sockets: %d\n", ret);
		goto err;
	}
#else
	/*
	 * Turn off SIGPIPE so that we don't kill processes when they
	 * happen to lose a connection at the wrong time.
	 */
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = SIG_IGN;
	if ((ret = sigaction(SIGPIPE, &sigact, NULL)) != 0) {
		fprintf(stderr,
		    "Unable to turn off SIGPIPE: %s\n", strerror(ret));
		goto err;
	}
#endif

	/*
	 * We are hardcoding priorities here that all clients have the
	 * same priority except for a designated master who gets a higher
	 * priority.
	 */
	if ((ret =
	    machtab_init(&machtab, setup_info.nsites)) != 0)
		goto err;
	my_app_data.comm_infrastructure = machtab;

	if ((ret = env_init(dbenv, setup_info.home)) != 0)
		goto err;

	/*
	 * Now sets up comm infrastructure.  There are two phases.  First,
	 * we open our port for listening for incoming connections.  Then
	 * we attempt to connect to every host we know about.
	 */

	(void)dbenv->rep_set_transport(dbenv, SELF_EID, quote_send);

	ca.dbenv = dbenv;
	ca.home = setup_info.home;
	ca.progname = progname;
	ca.machtab = machtab;
	ca.port = setup_info.self.port;
	if ((ret = thread_create(&conn_thr, NULL, connect_thread, &ca)) != 0) {
		dbenv->errx(dbenv, "can't create connect thread");
		goto err;
	}

	aa.dbenv = dbenv;
	aa.progname = progname;
	aa.home = setup_info.home;
	aa.machtab = machtab;
	aa.sites = setup_info.site_list;
	aa.nsites = setup_info.remotesites;
	if ((ret = thread_create(&all_thr, NULL, connect_all, &aa)) != 0) {
		dbenv->errx(dbenv, "can't create connect-all thread");
		goto err;
	}

	/* Start checkpoint and log archive threads. */
	supa.dbenv = dbenv;
	supa.shared = &my_app_data.shared_data;
	if ((ret = start_support_threads(dbenv, &supa, &ckp_thr, &lga_thr))
	    != 0)
		goto err;

	/*
	 * We have now got the entire communication infrastructure set up.
	 * It's time to declare ourselves to be a client or master.
	 */
	if (setup_info.role == MASTER) {
		if ((ret = dbenv->rep_start(dbenv, NULL, DB_REP_MASTER)) != 0) {
			dbenv->err(dbenv, ret, "dbenv->rep_start failed");
			goto err;
		}
	} else {
		memset(&local, 0, sizeof(local));
		local.data = myaddr;
		local.size = (u_int32_t)strlen(myaddr) + 1;
		if ((ret =
		    dbenv->rep_start(dbenv, &local, DB_REP_CLIENT)) != 0) {
			dbenv->err(dbenv, ret, "dbenv->rep_start failed");
			goto err;
		}
		/* Sleep to give ourselves time to find a master. */
		sleep(5);
	}

	if ((ret = doloop(dbenv, &my_app_data.shared_data)) != 0) {
		dbenv->err(dbenv, ret, "Main loop failed");
		goto err;
	}

	/* Finish checkpoint and log archive threads. */
	if ((ret = finish_support_threads(&ckp_thr, &lga_thr)) != 0)
		goto err;

	/* Wait on the connection threads. */
	if (thread_join(all_thr, &astatus) || thread_join(conn_thr, &cstatus)) {
		ret = -1;
		goto err;
	}
	if ((uintptr_t)astatus != EXIT_SUCCESS ||
	    (uintptr_t)cstatus != EXIT_SUCCESS) {
		ret = -1;
		goto err;
	}

	/*
	 * We have used the DB_TXN_NOSYNC environment flag for improved
	 * performance without the usual sacrifice of transactional durability,
	 * as discussed in the "Transactional guarantees" page of the Reference
	 * Guide: if one replication site crashes, we can expect the data to
	 * exist at another site.  However, in case we shut down all sites
	 * gracefully, we push out the end of the log here so that the most
	 * recent transactions don't mysteriously disappear.
	 */
	if ((ret = dbenv->log_flush(dbenv, NULL)) != 0)
		dbenv->err(dbenv, ret, "log_flush");

err:	if (machtab != NULL)
		free(machtab);
	if (dbenv != NULL)
		(void)dbenv->close(dbenv, 0);
#ifdef _WIN32
	/* Shut down the Windows sockets DLL. */
	(void)WSACleanup();
#endif
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

	switch (which) {
	case DB_EVENT_REP_CLIENT:
		shared->is_master = 0;
		shared->in_client_sync = 1;
		break;

	case DB_EVENT_REP_ELECTED:
		app->elected = 1;
		master_eid = SELF_EID;
		break;

	case DB_EVENT_REP_MASTER:
		shared->is_master = 1;
		shared->in_client_sync = 0;
		break;

	case DB_EVENT_REP_NEWMASTER:
		master_eid = *(int*)info;
		shared->in_client_sync = 1;
		break;

	case DB_EVENT_REP_STARTUPDONE:
		shared->in_client_sync = 0;
		break;

	default:
		dbenv->errx(dbenv, "ignoring event %d", which);
	}
}
