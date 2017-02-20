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

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>
#include <sstream>

#include <db_cxx.h>
#include "RepConfigInfo.h"
#include "dbc_auto.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::flush;
using std::istream;
using std::istringstream;
using std::string;
using std::getline;

#define CACHESIZE   (10 * 1024 * 1024)
#define DATABASE    "quote.db"

const char *progname = "excxx_repquote";

#include <errno.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define snprintf        _snprintf
#define sleep(s)        Sleep(1000 * (s))

extern "C" {
  extern int getopt(int, char * const *, const char *);
  extern char *optarg;
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

class RepQuoteExample {
public:
    RepQuoteExample();
    void init(RepConfigInfo* config);
    void doloop();
    int terminate();

    static void event_callback(DbEnv* dbenv, u_int32_t which, void *info);

private:
    // disable copy constructor.
    RepQuoteExample(const RepQuoteExample &);
    void operator = (const RepQuoteExample &);

    // internal data members.
    APP_DATA        app_data;
    RepConfigInfo   *app_config;
    DbEnv          cur_env;
    thread_t ckp_thr;
    thread_t lga_thr;

    // private methods.
    void print_stocks(Db *dbp);
    void prompt();
};

class DbHolder {
public:
    DbHolder(DbEnv *env) : env(env) {
    dbp = 0;
    }

    ~DbHolder() {
    try {
        close();
    } catch (...) {
        // Ignore: this may mean another exception is pending
    }
    }

    bool ensure_open(bool creating) {
    if (dbp)
        return (true);
    dbp = new Db(env, 0);

    u_int32_t flags = DB_AUTO_COMMIT;
    if (creating)
        flags |= DB_CREATE;
    try {
        dbp->open(NULL, DATABASE, NULL, DB_BTREE, flags, 0);
        return (true);
    } catch (DbDeadlockException e) {
    } catch (DbRepHandleDeadException e) {
    } catch (DbException e) {
        if (e.get_errno() == DB_REP_LOCKOUT) {
        // Just fall through.
        } else if (e.get_errno() == ENOENT && !creating) {
        // Provide a bit of extra explanation.
        // 
        log("Stock DB does not yet exist");
        } else
        throw;
    }

    // (All retryable errors fall through to here.)
    // 
    log("please retry the operation");
    close();
    return (false);
    }

    void close() {
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

    operator Db *() {
    return dbp;
    }

    Db *operator->() {
    return dbp;
    }

private:
    Db *dbp;
    DbEnv *env;
};

class StringDbt : public Dbt {
public:
#define GET_STRING_OK 0
#define GET_STRING_INVALID_PARAM 1
#define GET_STRING_SMALL_BUFFER 2
#define GET_STRING_EMPTY_DATA 3
    int get_string(char **buf, size_t buf_len)
    {
        size_t copy_len;
        int ret = GET_STRING_OK;
        if (buf == NULL) {
            cerr << "Invalid input buffer to get_string" << endl;
            return GET_STRING_INVALID_PARAM;
        }

        // make sure the string is null terminated.
        memset(*buf, 0, buf_len);

        // if there is no string, just return.
        if (get_data() == NULL || get_size() == 0)
            return GET_STRING_OK;

        if (get_size() >= buf_len) {
            ret = GET_STRING_SMALL_BUFFER;
            copy_len = buf_len - 1; // save room for a terminator.
        } else
            copy_len = get_size();
        memcpy(*buf, get_data(), copy_len);

        return ret;
    }
    size_t get_string_length()
    {
        if (get_size() == 0)
            return 0;
        return strlen((char *)get_data());
    }
    void set_string(char *string)
    {
        set_data(string);
        set_size((u_int32_t)strlen(string));
    }

    StringDbt(char *string) : 
        Dbt(string, (u_int32_t)strlen(string)) {};
    StringDbt() : Dbt() {};
    ~StringDbt() {};

    // Don't add extra data to this sub-class since we want it to remain
    // compatible with Dbt objects created internally by Berkeley DB.
};

RepQuoteExample::RepQuoteExample() : app_config(0), cur_env(0) {
    app_data.app_finished = 0;
    app_data.in_client_sync = 0;
    app_data.is_master = 0; // assume I start out as client
}

void RepQuoteExample::init(RepConfigInfo *config) {
    app_config = config;

    cur_env.set_app_private(&app_data);
    cur_env.set_errfile(stderr);
    cur_env.set_errpfx(progname);
    cur_env.set_event_notify(event_callback);

    // Configure bulk transfer to send groups of records to clients 
    // in a single network transfer.  This is useful for master sites 
    // and clients participating in client-to-client synchronization.
    //
    if (app_config->bulk)
        cur_env.rep_set_config(DB_REP_CONF_BULK, 1);

    // Set the total number of sites in the replication group.
    // This is used by repmgr internal election processing.
    //
    if (app_config->totalsites > 0)
        cur_env.rep_set_nsites(app_config->totalsites);

    // Turn on debugging and informational output if requested.
    if (app_config->verbose)
        cur_env.set_verbose(DB_VERB_REPLICATION, 1);

    // Set replication group election priority for this environment.
    // An election first selects the site with the most recent log
    // records as the new master.  If multiple sites have the most
    // recent log records, the site with the highest priority value 
    // is selected as master.
    //
    cur_env.rep_set_priority(app_config->priority);

    // Set the policy that determines how master and client sites
    // handle acknowledgement of replication messages needed for
    // permanent records.  The default policy of "quorum" requires only
    // a quorum of electable peers sufficient to ensure a permanent
    // record remains durable if an election is held.  The "all" option
    // requires all clients to acknowledge a permanent replication
    // message instead.
    //
    cur_env.repmgr_set_ack_policy(app_config->ack_policy);

    // Set the threshold for the minimum and maximum time the client
    // waits before requesting retransmission of a missing message.
    // Base these values on the performance and load characteristics
    // of the master and client host platforms as well as the round
    // trip message time.
    //
    cur_env.rep_set_request(20000, 500000);

    // Configure deadlock detection to ensure that any deadlocks
    // are broken by having one of the conflicting lock requests
    // rejected. DB_LOCK_DEFAULT uses the lock policy specified
    // at environment creation time or DB_LOCK_RANDOM if none was
    // specified.
    //
    cur_env.set_lk_detect(DB_LOCK_DEFAULT);

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

    cur_env.repmgr_set_local_site(app_config->this_host.host,
        app_config->this_host.port, 0);

    for ( REP_HOST_INFO *cur = app_config->other_hosts; cur != NULL;
        cur = cur->next) {
        cur_env.repmgr_add_remote_site(cur->host, cur->port,
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
    cur_env.rep_set_timeout(DB_REP_HEARTBEAT_SEND, 5000000);
    cur_env.rep_set_timeout(DB_REP_HEARTBEAT_MONITOR, 10000000);

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
    //
    cur_env.set_cachesize(0, CACHESIZE, 0);
    cur_env.set_flags(DB_TXN_NOSYNC, 1);

    cur_env.open(app_config->home, DB_CREATE | DB_RECOVER |
        DB_THREAD | DB_INIT_REP | DB_INIT_LOCK | DB_INIT_LOG |
        DB_INIT_MPOOL | DB_INIT_TXN, 0);

    // Start checkpoint and log archive support threads.
    (void)thread_create(&ckp_thr, NULL, checkpoint_thread, &cur_env);
    (void)thread_create(&lga_thr, NULL, log_archive_thread, &cur_env);

    cur_env.repmgr_start(3, app_config->start_policy);
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
        //
        cur_env.log_flush(NULL);

        cur_env.close(0);
    } catch (DbException dbe) {
        cout << "error closing environment: " << dbe.what() << endl;
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
    DbHolder dbh(&cur_env);

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
                log("Format: <stock> <price>");
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
        if (!dbh.ensure_open(app_data.is_master))
            continue;

        try {
            if (count == 0)
                if (app_data.in_client_sync)
                    log(
    "Cannot read data during client initialization - please try again.");
                else
                    print_stocks(dbh);
            else if (!app_data.is_master)
                log("Can't update at client");
            else {
                const char *symbol = token1.c_str();
                StringDbt key(const_cast<char*>(symbol));

                const char *price = token2.c_str();
                StringDbt data(const_cast<char*>(price));

                dbh->put(NULL, &key, &data, 0);
            }
        } catch (DbDeadlockException e) {
            log("please retry the operation");
            dbh.close();
        } catch (DbRepHandleDeadException e) {
            log("please retry the operation");
            dbh.close();
        } catch (DbException e) {
            if (e.get_errno() == DB_REP_LOCKOUT) {
            log("please retry the operation");
            dbh.close();
            } else
            throw;
        }
    }

    dbh.close();
}

void RepQuoteExample::event_callback(DbEnv* dbenv, u_int32_t which, void *info)
{
    APP_DATA *app = (APP_DATA*)dbenv->get_app_private();

    info = NULL;        /* Currently unused. */

    switch (which) {
    case DB_EVENT_REP_CLIENT:
        app->is_master = 0;
        app->in_client_sync = 1;
        break;
    case DB_EVENT_REP_MASTER:
        app->is_master = 1;
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
    case DB_EVENT_REP_STARTUPDONE:
        app->in_client_sync = 0;
        break;
    default:
        dbenv->errx("ignoring event %d", which);
    }
}

void RepQuoteExample::print_stocks(Db *dbp) {
    StringDbt key, data;
#define MAXKEYSIZE  10
#define MAXDATASIZE 20
    char keybuf[MAXKEYSIZE + 1], databuf[MAXDATASIZE + 1];
    char *kbuf, *dbuf;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    kbuf = keybuf;
    dbuf = databuf;

    DbcAuto dbc(dbp, 0, 0);
    cout << "\tSymbol\tPrice" << endl
        << "\t======\t=====" << endl;

    for (int ret = dbc->get(&key, &data, DB_FIRST);
        ret == 0;
        ret = dbc->get(&key, &data, DB_NEXT)) {
        key.get_string(&kbuf, MAXKEYSIZE);
        data.get_string(&dbuf, MAXDATASIZE);

        cout << "\t" << keybuf << "\t" << databuf << endl;
    }
    cout << endl << flush;
    dbc.close();
}

static void usage() {
    cerr << "usage: " << progname << " -h home -l host:port [-CM]"
        << "[-r host:port][-R host:port]" << endl
        << "  [-a all|quorum][-b][-n nsites][-p priority][-v]" << endl;

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
                cerr << "Bad host specification." << endl;
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
        cerr << "Caught an exception during initialization or"
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
