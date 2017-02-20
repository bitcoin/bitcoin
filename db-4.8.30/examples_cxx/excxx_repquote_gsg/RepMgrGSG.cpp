/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

// NOTE: This example is a simplified version of the RepQuoteExample.cxx
// example that can be found in the db/examples_cxx/excxx_repquote directory.
//
// This example is intended only as an aid in learning Replication Manager
// concepts. It is not complete in that many features are not exercised 
// in it, nor are many error conditions properly handled.

#include <iostream>

#include <db_cxx.h>
#include "RepConfigInfo.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::flush;

#define CACHESIZE (10 * 1024 * 1024)
#define DATABASE "quote.db"
#define SLEEPTIME 3

const char *progname = "excxx_repquote_gsg_repmgr";

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#define    sleep(s)        Sleep(1000 * (s))

extern "C" {
  extern int getopt(int, char * const *, const char *);
  extern char *optarg;
}
#else
#include <errno.h>
#endif

// Struct used to store information in Db app_private field.
typedef struct {
    int is_master;
} APP_DATA;

class RepMgrGSG
{
public:
    RepMgrGSG();
    int init(RepConfigInfo* config);
    int doloop();
    int terminate();

    static void event_callback(DbEnv * dbenv, u_int32_t which, void *info);

private:
    // Disable copy constructor.
    RepMgrGSG(const RepMgrGSG &);
    void operator = (const RepMgrGSG &);

    // Internal data members.
    APP_DATA        app_data;
    RepConfigInfo   *app_config;
    DbEnv           dbenv;

    // Private methods.
    static int print_stocks(Db *dbp);
};

static void usage()
{
    cerr << "usage: " << progname << endl
        << "-h home -l host:port [-r host:port]"
        << "[-n nsites][-p priority]" << endl;

    cerr 
        << "\t -h home directory (required)" << endl
        << "\t -l host:port (required; l stands for local)" << endl
        << "\t -r host:port (optional; r stands for remote; any "
        << "number of these" << endl
        << "\t    may be specified)" << endl
        << "\t -n nsites (optional; number of sites in replication "
        << "group; defaults" << endl
        << "\t    to 0 to try to dynamically compute nsites)" << endl
        << "\t -p priority (optional; defaults to 100)" << endl;

    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    RepConfigInfo config;
    char ch, *portstr, *tmphost;
    int tmpport;
    int ret;

    // Extract the command line parameters.
    while ((ch = getopt(argc, argv, "h:l:n:p:r:")) != EOF) {
        switch (ch) {
        case 'h':
            config.home = optarg;
            break;
        case 'l':
            config.this_host.host = strtok(optarg, ":");
            if ((portstr = strtok(NULL, ":")) == NULL) {
                cerr << "Bad host specification." << endl;
                usage();
            }
            config.this_host.port = (unsigned short)atoi(portstr);
            config.got_listen_address = true;
            break;
        case 'n':
            config.totalsites = atoi(optarg);
            break;
        case 'p':
            config.priority = atoi(optarg);
            break;
        case 'r':
            tmphost = strtok(optarg, ":");
            if ((portstr = strtok(NULL, ":")) == NULL) {
                cerr << "Bad host specification." << endl;
                usage();
            }
            tmpport = (unsigned short)atoi(portstr);
            config.addOtherHost(tmphost, tmpport);
            break;
        case '?':
        default:
            usage();
        }
    }

    // Error check command line.
    if ((!config.got_listen_address) || config.home == NULL)
        usage();

    RepMgrGSG runner;
    try {
        if((ret = runner.init(&config)) != 0)
            goto err;
        if((ret = runner.doloop()) != 0)
            goto err;
    } catch (DbException dbe) {
        cerr << "Caught an exception during initialization or"
            << " processing: " << dbe.what() << endl;
    }
err:
    runner.terminate();
    return 0;
}

RepMgrGSG::RepMgrGSG() : app_config(0), dbenv(0)
{
    app_data.is_master = 0; // By default, assume this site is not a master.
}

int RepMgrGSG::init(RepConfigInfo *config)
{
    int ret = 0;

    app_config = config;

    dbenv.set_errfile(stderr);
    dbenv.set_errpfx(progname);
    dbenv.set_app_private(&app_data);
    dbenv.set_event_notify(event_callback);
    dbenv.repmgr_set_ack_policy(DB_REPMGR_ACKS_ALL);

    if ((ret = dbenv.repmgr_set_local_site(app_config->this_host.host,
        app_config->this_host.port, 0)) != 0) {
        // Should throw an exception anyway.
        cerr << "Could not set listen address to host:port "
            << app_config->this_host.host << ":"
            << app_config->this_host.port
            << "error: " << ret << endl;
        cerr << "WARNING: This should have been an exception." << endl;
    }

    for ( REP_HOST_INFO *cur = app_config->other_hosts; cur != NULL;
        cur = cur->next) {
        if ((ret = dbenv.repmgr_add_remote_site(cur->host, cur->port, 
            0, 0)) != 0) {
            // Should have resulted in an exception.
            cerr << "could not add site." << endl
                << "WARNING: This should have been an exception." << endl;
        }
    }

    if (app_config->totalsites > 0) {
        try {
            if ((ret = dbenv.rep_set_nsites(app_config->totalsites)) != 0)
                dbenv.err(ret, "set_nsites");
        } catch (DbException dbe) {
            cerr << "rep_set_nsites call failed. Continuing." << endl;
            // Non-fatal to the test app.
        }
    }

    dbenv.rep_set_priority(app_config->priority);

    // Permanent messages require at least one ack.
    dbenv.repmgr_set_ack_policy(DB_REPMGR_ACKS_ONE);
    // Give 500 microseconds to receive the ack.
    dbenv.rep_set_timeout(DB_REP_ACK_TIMEOUT, 5);

    // We can now open our environment, although we're not ready to
    // begin replicating.  However, we want to have a dbenv around
    // so that we can send it into any of our message handlers.
    dbenv.set_cachesize(0, CACHESIZE, 0);
    dbenv.set_flags(DB_TXN_NOSYNC, 1);

    try {
        dbenv.open(app_config->home, DB_CREATE | DB_RECOVER |
            DB_THREAD | DB_INIT_REP | DB_INIT_LOCK | DB_INIT_LOG | 
            DB_INIT_MPOOL | DB_INIT_TXN, 0);
    } catch(DbException dbe) {
        cerr << "Caught an exception during DB environment open." << endl
            << "Ensure that the home directory is created prior to starting"
            << " the application." << endl;
        ret = ENOENT;
        goto err;
    }

    if ((ret = dbenv.repmgr_start(3, app_config->start_policy)) != 0)
        goto err;

err:
    return ret;
}

int RepMgrGSG::terminate()
{
    try {
        dbenv.close(0);
    } catch (DbException dbe) {
        cerr << "error closing environment: " << dbe.what() << endl;
    }
    return 0;
}

// Provides the main data processing function for our application.
// This function provides a command line prompt to which the user
// can provide a ticker string and a stock price.  Once a value is
// entered to the application, the application writes the value to
// the database and then displays the entire database.
#define BUFSIZE 1024
int RepMgrGSG::doloop()
{
    Dbt key, data;
    Db *dbp;
    char buf[BUFSIZE], *rbuf;
    int ret;

    dbp = 0;
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    ret = 0;

    for (;;) {
        if (dbp == 0) {
            dbp = new Db(&dbenv, 0);

            try {
                dbp->open(NULL, DATABASE, NULL, DB_BTREE,
                    app_data.is_master ? DB_CREATE | DB_AUTO_COMMIT :
                    DB_AUTO_COMMIT, 0);
            } catch(DbException dbe) {
                // It is expected that this condition will be triggered
                // when client sites start up.  It can take a while for 
                // the master site to be found and synced, and no DB will
                // be available until then.
                if (dbe.get_errno() == ENOENT) {
                    cout << "No stock db available yet - retrying." << endl;
                    try {
                        dbp->close(0);
                    } catch (DbException dbe2) {
                        cout << "Unexpected error closing after failed" <<
                            " open, message: " << dbe2.what() << endl;
                        dbp = NULL;
                        goto err;
                    }
                    dbp = NULL;
                    sleep(SLEEPTIME);
                    continue;
                } else {
                    dbenv.err(ret, "DB->open");
                    throw dbe;
                }
            }
        }

        cout << "QUOTESERVER" ;
        if (!app_data.is_master)
            cout << "(read-only)";
        cout << "> " << flush;

        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        if (strtok(&buf[0], " \t\n") == NULL) {
            switch ((ret = print_stocks(dbp))) {
            case 0:
                continue;
            case DB_REP_HANDLE_DEAD:
                (void)dbp->close(DB_NOSYNC);
                cout << "closing db handle due to rep handle dead" << endl;
                dbp = NULL;
                continue;
            default:
                dbp->err(ret, "Error traversing data");
                goto err;
            }
        }
        rbuf = strtok(NULL, " \t\n");
        if (rbuf == NULL || rbuf[0] == '\0') {
            if (strncmp(buf, "exit", 4) == 0 ||
                strncmp(buf, "quit", 4) == 0)
                break;
            dbenv.errx("Format: TICKER VALUE");
            continue;
        }

        if (!app_data.is_master) {
            dbenv.errx("Can't update at client");
            continue;
        }

        key.set_data(buf);
        key.set_size((u_int32_t)strlen(buf));

        data.set_data(rbuf);
        data.set_size((u_int32_t)strlen(rbuf));

        if ((ret = dbp->put(NULL, &key, &data, 0)) != 0)
        {
            dbp->err(ret, "DB->put");
            if (ret != DB_KEYEXIST)
                goto err;
        }
    }

err:    if (dbp != 0) {
        (void)dbp->close(DB_NOSYNC);
        }

    return (ret);
}

// Handle replication events of interest to this application.
void RepMgrGSG::event_callback(DbEnv* dbenv, u_int32_t which, void *info)
{
    APP_DATA *app = (APP_DATA*)dbenv->get_app_private();

    info = 0;                // Currently unused.

    switch (which) {
    case DB_EVENT_REP_MASTER:
        app->is_master = 1;
        break;

    case DB_EVENT_REP_CLIENT:
        app->is_master = 0;
        break;

    case DB_EVENT_REP_STARTUPDONE: // FALLTHROUGH
    case DB_EVENT_REP_NEWMASTER:
        // Ignore.
        break;

    default:
        dbenv->errx("ignoring event %d", which);
    }
}

// Display all the stock quote information in the database.
int RepMgrGSG::print_stocks(Db *dbp)
{
    Dbc *dbc;
    Dbt key, data;
#define    MAXKEYSIZE    10
#define    MAXDATASIZE    20
    char keybuf[MAXKEYSIZE + 1], databuf[MAXDATASIZE + 1];
    int ret, t_ret;
    u_int32_t keysize, datasize;

     if ((ret = dbp->cursor(NULL, &dbc, 0)) != 0) {
        dbp->err(ret, "can't open cursor");
        return (ret);
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    cout << "\tSymbol\tPrice" << endl
        << "\t======\t=====" << endl;

    for (ret = dbc->get(&key, &data, DB_FIRST);
        ret == 0;
        ret = dbc->get(&key, &data, DB_NEXT)) {
        keysize = key.get_size() > MAXKEYSIZE ? MAXKEYSIZE : key.get_size();
        memcpy(keybuf, key.get_data(), keysize);
        keybuf[keysize] = '\0';

        datasize = data.get_size() >=
            MAXDATASIZE ? MAXDATASIZE : data.get_size();
        memcpy(databuf, data.get_data(), datasize);
        databuf[datasize] = '\0';

        cout << "\t" << keybuf << "\t" << databuf << endl;
    }
    cout << endl << flush;

    if ((t_ret = dbc->close()) != 0 && ret == 0) {
        cout << "closed cursor" << endl;
        ret = t_ret;
    }

    switch (ret) {
    case 0:
    case DB_NOTFOUND:
    case DB_LOCK_DEADLOCK:
        return (0);
    default:
        return (ret);
    }
}

