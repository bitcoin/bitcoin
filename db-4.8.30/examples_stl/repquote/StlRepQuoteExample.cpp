/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * In this application, we specify all communication via the command line.  In
 * a real application, we would expect that information about the other sites
 * in the system would be maintained in some sort of configuration file.  The
 * critical part of this interface is that we assume at startup that we can
 * find out
 *  1) what our Berkeley DB home environment is,
 *  2) what host/port we wish to listen on for connections; and
 *  3) an optional list of other sites we should attempt to connect to.
 *
 * These pieces of information are expressed by the following flags.
 * -h home (required; h stands for home directory)
 * -l host:port (required; l stands for local)
 * -C or -M (optional; start up as client or master)
 * -r host:port (optional; r stands for remote; any number of these may be
 *  specified)
 * -R host:port (optional; R stands for remote peer; only one of these may
 *      be specified)
 * -a all|quorum (optional; a stands for ack policy)
 * -b (optional; b stands for bulk)
 * -n nsites (optional; number of sites in replication group; defaults to 0
 *  to try to dynamically compute nsites)
 * -p priority (optional; defaults to 100)
 * -v (optional; v stands for verbose)
 */

#include <iostream>
#include <string>
#include <sstream>

#include <db_cxx.h>
#include "StlRepConfigInfo.h"
#include "dbstl_map.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::flush;
using std::istream;
using std::istringstream;
using std::string;
using std::getline;
using namespace dbstl;
#define CACHESIZE   (10 * 1024 * 1024)
#define DATABASE    "quote.db"

const char *progname = "exstl_repquote";

#include <errno.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define snprintf        _snprintf
#define sleep(s)        Sleep(1000 * (s))

extern "C" {
extern int getopt(int, char * const *, const char *);
extern char *optarg;
extern int optind;
}

typedef HANDLE thread_t;
typedef DWORD thread_exit_status_t;
#define thread_create(thrp, attr, func, arg)                   \
    (((*(thrp) = CreateThread(NULL, 0,                     \
    (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define thread_join(thr, statusp)                      \
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&        \
    GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)
#else /* !_WIN32 */
#include <pthread.h>

typedef pthread_t thread_t;
typedef void* thread_exit_status_t;
#define thread_create(thrp, attr, func, arg)                   \
    pthread_create((thrp), (attr), (func), (arg))
#define thread_join(thr, statusp) pthread_join((thr), (statusp))
#endif

// Struct used to store information in Db app_private field.
typedef struct {
    bool app_finished;
    bool in_client_sync;
    bool is_master;
} APP_DATA;

static void log(const char *);
void *checkpoint_thread (void *);
void *log_archive_thread (void *);

class RepQuoteExample 
{
public:
    typedef db_map<char *, char *, ElementHolder<char *> > str_map_t;
    RepQuoteExample();
    void init(RepConfigInfo* config);
    void doloop();
    int terminate();

    static void event_callback(DbEnv * dbenv, u_int32_t which, void *info);

private:
    // disable copy constructor.
    RepQuoteExample(const RepQuoteExample &);
    void operator = (const RepQuoteExample &);

    // internal data members.
    APP_DATA        app_data;
    RepConfigInfo   *app_config;
    DbEnv          *cur_env;
    Db *dbp;
    str_map_t *strmap;
    thread_t ckp_thr;
    thread_t lga_thr;

    // private methods.
    void print_stocks();
    void prompt();
    bool open_db(bool creating);
    void close_db(){ 
        delete strmap; 
        strmap = NULL; 
        dbstl::close_db(dbp); 
        dbp = NULL; 
    }
    static void close_db(Db *&);// Close an unregistered Db handle.
};

bool RepQuoteExample::open_db(bool creating)
{
    int ret;

    if (dbp)
        return true;

    dbp = new Db(cur_env, DB_CXX_NO_EXCEPTIONS);

    u_int32_t flags = DB_AUTO_COMMIT | DB_THREAD;
    if (creating)
        flags |= DB_CREATE;

    ret = dbp->open(NULL, DATABASE, NULL, DB_BTREE, flags, 0);
    switch (ret) {
    case 0:
        register_db(dbp);
        if (strmap)
            delete strmap;
        strmap = new str_map_t(dbp, cur_env);
        return (true);
    case DB_LOCK_DEADLOCK: // Fall through
    case DB_REP_HANDLE_DEAD:
        log("\nFailed to open stock db.");
        break;
    default:
        if (ret == DB_REP_LOCKOUT)
            break; // Fall through
        else if (ret == ENOENT && !creating) 
            log("\nStock DB does not yet exist\n");
        else {
            DbException ex(ret);
            throw ex;
        }
    } // switch

    // (All retryable errors fall through to here.)
    // 
    log("\nPlease retry the operation");
    close_db(dbp);
    return (false);
}

void RepQuoteExample::close_db(Db *&dbp) 
{
    if (dbp) {
        try {
            dbp->close(0);
            delete dbp;
            dbp = 0;
        } catch (...) {
            delete dbp;
            dbp = 0;
            throw;
        }
    }

}

RepQuoteExample::RepQuoteExample() : app_config(0), cur_env(NULL) {
    app_data.app_finished = 0;
    app_data.in_client_sync = 0;
    app_data.is_master = 0; // assume I start out as client
    cur_env = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    strmap = NULL;
    dbp = NULL;
}

void RepQuoteExample::init(RepConfigInfo *config) {
    app_config = config;

    cur_env->set_app_private(&app_data);
    cur_env->set_errfile(stderr);
    cur_env->set_errpfx(progname);
    cur_env->set_event_notify(event_callback);

    // Configure bulk transfer to send groups of records to clients 
    // in a single network transfer.  This is useful for master sites 
    // and clients participating in client-to-client synchronization.
    //
    if (app_config->bulk)
        cur_env->rep_set_config(DB_REP_CONF_BULK, 1);


    // Set the total number of sites in the replication group.
    // This is used by repmgr internal election processing.
    //
    if (app_config->totalsites > 0)
        cur_env->rep_set_nsites(app_config->totalsites);

    // Turn on debugging and informational output if requested.
    if (app_config->verbose)
        cur_env->set_verbose(DB_VERB_REPLICATION, 1);

    // Set replication group election priority for this environment.
    // An election first selects the site with the most recent log
    // records as the new master.  If multiple sites have the most
    // recent log records, the site with the highest priority value 
    // is selected as master.
    //
    cur_env->rep_set_priority(app_config->priority);

    // Set the policy that determines how master and client sites
    // handle acknowledgement of replication messages needed for
    // permanent records.  The default policy of "quorum" requires only
    // a quorum of electable peers sufficient to ensure a permanent
    // record remains durable if an election is held.  The "all" option
    // requires all clients to acknowledge a permanent replication
    // message instead.
    //
    cur_env->repmgr_set_ack_policy(app_config->ack_policy);

    // Set the threshold for the minimum and maximum time the client
    // waits before requesting retransmission of a missing message.
    // Base these values on the performance and load characteristics
    // of the master and client host platforms as well as the round
    // trip message time.
    //
    cur_env->rep_set_request(20000, 500000);

    // Configure deadlock detection to ensure that any deadlocks
    // are broken by having one of the conflicting lock requests
    // rejected. DB_LOCK_DEFAULT uses the lock policy specified
    // at environment creation time or DB_LOCK_RANDOM if none was
    // specified.
    //
    cur_env->set_lk_detect(DB_LOCK_DEFAULT);

    // The following base replication features may also be useful to your
    // application. See Berkeley DB documentation for more details.
    //   - Master leases: Provide stricter consistency for data reads
    //     on a master site.
    //   - Timeouts: Customize the amount of time Berkeley DB waits
    //     for such things as an election to be concluded or a master
    //     lease to be granted.
    //   - Delayed client synchronization: Manage the master site's
    //     resources by spreading out resource-intensive client 
    //     synchronizations.
    //   - Blocked client operations: Return immediately with an error
    //     instead of waiting indefinitely if a client operation is
    //     blocked by an ongoing client synchronization.

    cur_env->repmgr_set_local_site(app_config->this_host.host,
        app_config->this_host.port, 0);

    for ( REP_HOST_INFO *cur = app_config->other_hosts; cur != NULL;
        cur = cur->next) {
        cur_env->repmgr_add_remote_site(cur->host, cur->port,
            NULL, cur->peer ? DB_REPMGR_PEER : 0);
    }

    // Configure heartbeat timeouts so that repmgr monitors the
    // health of the TCP connection.  Master sites broadcast a heartbeat
    // at the frequency specified by the DB_REP_HEARTBEAT_SEND timeout.
    // Client sites wait for message activity the length of the 
    // DB_REP_HEARTBEAT_MONITOR timeout before concluding that the 
    // connection to the master is lost.  The DB_REP_HEARTBEAT_MONITOR 
    // timeout should be longer than the DB_REP_HEARTBEAT_SEND timeout.
    //
    cur_env->rep_set_timeout(DB_REP_HEARTBEAT_SEND, 5000000);
    cur_env->rep_set_timeout(DB_REP_HEARTBEAT_MONITOR, 10000000);

    // The following repmgr features may also be useful to your
    // application.  See Berkeley DB documentation for more details.
    //  - Two-site strict majority rule - In a two-site replication
    //    group, require both sites to be available to elect a new
    //    master.
    //  - Timeouts - Customize the amount of time repmgr waits
    //    for such things as waiting for acknowledgements or attempting 
    //    to reconnect to other sites.
    //  - Site list - return a list of sites currently known to repmgr.

    // We can now open our environment, although we're not ready to
    // begin replicating.  However, we want to have a dbenv around
    // so that we can send it into any of our message handlers.
    cur_env->set_cachesize(0, CACHESIZE, 0);
    cur_env->set_flags(DB_TXN_NOSYNC, 1);

    cur_env->open(app_config->home, DB_CREATE | DB_RECOVER |
        DB_THREAD | DB_INIT_REP | DB_INIT_LOCK | DB_INIT_LOG |
        DB_INIT_MPOOL | DB_INIT_TXN, 0);

    // Start checkpoint and log archive support threads.
    (void)thread_create(&ckp_thr, NULL, checkpoint_thread, cur_env);
    (void)thread_create(&lga_thr, NULL, log_archive_thread, cur_env);

    dbstl::register_db_env(cur_env);
    cur_env->repmgr_start(3, app_config->start_policy);
}

int RepQuoteExample::terminate() {
    try {
        // Wait for checkpoint and log archive threads to finish.
        // Windows does not allow NULL pointer for exit code variable.
        thread_exit_status_t exstat; 

        (void)thread_join(lga_thr, &exstat);
        (void)thread_join(ckp_thr, &exstat);

        // We have used the DB_TXN_NOSYNC environment flag for
        // improved performance without the usual sacrifice of
        // transactional durability, as discussed in the
        // "Transactional guarantees" page of the Reference
        // Guide: if one replication site crashes, we can
        // expect the data to exist at another site.  However,
        // in case we shut down all sites gracefully, we push
        // out the end of the log here so that the most
        // recent transactions don't mysteriously disappear.
        cur_env->log_flush(NULL);
    } catch (DbException dbe) {
        cout << "\nerror closing environment: " << dbe.what() << endl;
    }
    return 0;
}

void RepQuoteExample::prompt() {
    cout << "QUOTESERVER";
    if (!app_data.is_master)
        cout << "(read-only)";
    cout << "> " << flush;
}

void log(const char *msg) {
    cerr << msg << endl;
}

// Simple command-line user interface:
//  - enter "<stock symbol> <price>" to insert or update a record in the
//  database;
//  - just press Return (i.e., blank input line) to print out the contents of
//  the database;
//  - enter "quit" or "exit" to quit.
//
void RepQuoteExample::doloop() {
    string input;

    while (prompt(), getline(cin, input)) {
        istringstream is(input);
        string token1, token2;

        // Read 0, 1 or 2 tokens from the input.
        //
        int count = 0;
        if (is >> token1) {
            count++;
            if (is >> token2)
            count++;
        }

        if (count == 1) {
            if (token1 == "exit" || token1 == "quit") {
                app_data.app_finished = 1;
                break;
            } else {
                log("\nFormat: <stock> <price>\n");
                continue;
            }
        }

        // Here we know count is either 0 or 2, so we're about to try a
        // DB operation.
        //
        // Open database with DB_CREATE only if this is a master
        // database.  A client database uses polling to attempt
        // to open the database without DB_CREATE until it is
        // successful.
        //
        // This DB_CREATE polling logic can be simplified under
        // some circumstances.  For example, if the application can
        // be sure a database is already there, it would never need
        // to open it with DB_CREATE.
        //
        if (!open_db(app_data.is_master))
            continue;

        try {
            if (count == 0)
                if (app_data.in_client_sync)
                    log(
    "Cannot read data during client initialization - please try again.");
                else
                    print_stocks();
            else if (!app_data.is_master)
                log("\nCan't update at client\n");
            else {
                char *symbol = new char[token1.length() + 1];
                strcpy(symbol, token1.c_str());
                char *price = new char[token2.length() + 1];
                strcpy(price, token2.c_str());
                begin_txn(0, cur_env);
                strmap->insert(make_pair(symbol, price));
                commit_txn(cur_env);
                delete symbol;
                delete price;
            }
        } catch (DbDeadlockException e) {
            log("\nplease retry the operation\n");
            close_db();
        } catch (DbRepHandleDeadException e) {
            log("\nplease retry the operation\n");
            close_db();
        } catch (DbException e) {
            if (e.get_errno() == DB_REP_LOCKOUT) {
            log("\nplease retry the operation\n");
            close_db();
            } else
            throw;
        }
        
    }

    close_db();
}

void RepQuoteExample::event_callback(DbEnv* dbenv, u_int32_t which, void *info)
{
    APP_DATA *app = (APP_DATA*)dbenv->get_app_private();

    info = NULL;        /* Currently unused. */

    switch (which) {
    case DB_EVENT_REP_MASTER:
        app->in_client_sync = 0;
        app->is_master = 1;
        break;

    case DB_EVENT_REP_CLIENT:
        app->is_master = 0;
        app->in_client_sync = 1;
        break;

    case DB_EVENT_REP_STARTUPDONE:
        app->in_client_sync = 0;
        break;
    case DB_EVENT_REP_NEWMASTER:
        app->in_client_sync = 1;
        break;

    case DB_EVENT_REP_PERM_FAILED:
        // Did not get enough acks to guarantee transaction
        // durability based on the configured ack policy.  This 
        // transaction will be flushed to the master site's
        // local disk storage for durability.
        //
        log(
    "Insufficient acknowledgements to guarantee transaction durability.");
        break;

    default:
        dbenv->errx("\nignoring event %d", which);
    }
}

void RepQuoteExample::print_stocks() {
#define MAXKEYSIZE  10
#define MAXDATASIZE 20

    cout << "\tSymbol\tPrice" << endl
        << "\t======\t=====" << endl;
    str_map_t::iterator itr;
    if (strmap == NULL)
        strmap = new str_map_t(dbp, cur_env);
    begin_txn(0, cur_env);
    for (itr = strmap->begin(); itr != strmap->end(); ++itr) 
        cout<<"\t"<<itr->first<<"\t"<<itr->second<<endl;
    commit_txn(cur_env);
    cout << endl << flush;
}

static void usage() {
    cerr << "usage: " << progname << endl
        << "[-h home][-o host:port][-m host:port][-f host:port]"
        << "[-n nsites][-p priority]" << endl;

    cerr << "\t -h home (required; h stands for home directory)" << endl
        << "\t -l host:port (required; l stands for local)" << endl
        << "\t -C or -M (optional; start up as client or master)" << endl
        << "\t -r host:port (optional; r stands for remote; any "
        << "number of these" << endl
        << "\t               may be specified)" << endl
        << "\t -R host:port (optional; R stands for remote peer; only "
        << "one of" << endl
        << "\t               these may be specified)" << endl
        << "\t -a all|quorum (optional; a stands for ack policy)" << endl
        << "\t -b (optional; b stands for bulk)" << endl
        << "\t -n nsites (optional; number of sites in replication "
        << "group; defaults " << endl
        << "\t      to 0 to try to dynamically compute nsites)" << endl
        << "\t -p priority (optional; defaults to 100)" << endl
        << "\t -v (optional; v stands for verbose)" << endl;

    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    RepConfigInfo config;
    char ch, *portstr, *tmphost;
    int tmpport;
    bool tmppeer;

    // Extract the command line parameters
    while ((ch = getopt(argc, argv, "a:bCh:l:Mn:p:R:r:v")) != EOF) {
        tmppeer = false;
        switch (ch) {
        case 'a':
            if (strncmp(optarg, "all", 3) == 0)
                config.ack_policy = DB_REPMGR_ACKS_ALL;
            else if (strncmp(optarg, "quorum", 6) != 0)
                usage();
            break;
        case 'b':
            config.bulk = true;
            break;
        case 'C':
            config.start_policy = DB_REP_CLIENT;
            break;
        case 'h':
            config.home = optarg;
            break;
        case 'l':
            config.this_host.host = strtok(optarg, ":");
            if ((portstr = strtok(NULL, ":")) == NULL) {
                cerr << "\nBad host specification." << endl;
                usage();
            }
            config.this_host.port = (unsigned short)atoi(portstr);
            config.got_listen_address = true;
            break;
        case 'M':
            config.start_policy = DB_REP_MASTER;
            break;
        case 'n':
            config.totalsites = atoi(optarg);
            break;
        case 'p':
            config.priority = atoi(optarg);
            break;
        case 'R':
            tmppeer = true; // FALLTHROUGH
        case 'r':
            tmphost = strtok(optarg, ":");
            if ((portstr = strtok(NULL, ":")) == NULL) {
                cerr << "Bad host specification." << endl;
                usage();
            }
            tmpport = (unsigned short)atoi(portstr);

            config.addOtherHost(tmphost, tmpport, tmppeer);

            break;
        case 'v':
            config.verbose = true;
            break;
        case '?':
        default:
            usage();
        }
    }

    // Error check command line.
    if ((!config.got_listen_address) || config.home == NULL)
        usage();

    RepQuoteExample runner;
    try {
        runner.init(&config);
        runner.doloop();
    } catch (DbException dbe) {
        cerr << "\nCaught an exception during initialization or"
            << " processing: " << dbe.what() << endl;
    }
    runner.terminate();
    return 0;
}

// This is a very simple thread that performs checkpoints at a fixed
// time interval.  For a master site, the time interval is one minute
// plus the duration of the checkpoint_delay timeout (30 seconds by
// default.)  For a client site, the time interval is one minute.
//
void *checkpoint_thread(void *args)
{
    DbEnv *env;
    APP_DATA *app;
    int i, ret;

    env = (DbEnv *)args;
    app = (APP_DATA *)env->get_app_private();

    for (;;) {
        // Wait for one minute, polling once per second to see if
        // application has finished.  When application has finished,
        // terminate this thread.
        //
        for (i = 0; i < 60; i++) {
            sleep(1);
            if (app->app_finished == 1)
                return ((void *)EXIT_SUCCESS);
        }

        // Perform a checkpoint.
        if ((ret = env->txn_checkpoint(0, 0, 0)) != 0) {
            env->err(ret, "Could not perform checkpoint.\n");
            return ((void *)EXIT_FAILURE);
        }
    }
}

// This is a simple log archive thread.  Once per minute, it removes all but
// the most recent 3 logs that are safe to remove according to a call to
// DBENV->log_archive().
//
// Log cleanup is needed to conserve disk space, but aggressive log cleanup
// can cause more frequent client initializations if a client lags too far
// behind the current master.  This can happen in the event of a slow client,
// a network partition, or a new master that has not kept as many logs as the
// previous master.
//
// The approach in this routine balances the need to mitigate against a
// lagging client by keeping a few more of the most recent unneeded logs
// with the need to conserve disk space by regularly cleaning up log files.
// Use of automatic log removal (DBENV->log_set_config() DB_LOG_AUTO_REMOVE 
// flag) is not recommended for replication due to the risk of frequent 
// client initializations.
//
void *log_archive_thread(void *args)
{
    DbEnv *env;
    APP_DATA *app;
    char **begin, **list;
    int i, listlen, logs_to_keep, minlog, ret;

    env = (DbEnv *)args;
    app = (APP_DATA *)env->get_app_private();
    logs_to_keep = 3;

    for (;;) {
        // Wait for one minute, polling once per second to see if
        // application has finished.  When application has finished,
        // terminate this thread.
        //
        for (i = 0; i < 60; i++) {
            sleep(1);
            if (app->app_finished == 1)
                return ((void *)EXIT_SUCCESS);
        }

        // Get the list of unneeded log files.
        if ((ret = env->log_archive(&list, DB_ARCH_ABS)) != 0) {
            env->err(ret, "Could not get log archive list.");
            return ((void *)EXIT_FAILURE);
        }
        if (list != NULL) {
            listlen = 0;
            // Get the number of logs in the list.
            for (begin = list; *begin != NULL; begin++, listlen++);
            // Remove all but the logs_to_keep most recent
            // unneeded log files.
            //
            minlog = listlen - logs_to_keep;
            for (begin = list, i= 0; i < minlog; list++, i++) {
                if ((ret = unlink(*list)) != 0) {
                    env->err(ret,
                        "logclean: remove %s", *list);
                    env->errx(
                        "logclean: Error remove %s", *list);
                    free(begin);
                    return ((void *)EXIT_FAILURE);
                }
            }
            free(begin);
        }
    }
}

