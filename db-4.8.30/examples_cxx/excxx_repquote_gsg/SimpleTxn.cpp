/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <cstdlib>
#include <cstring>

#include <iostream>

#include <db_cxx.h>
#include "SimpleConfigInfo.h"

using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::flush;

#define CACHESIZE (10 * 1024 * 1024)
#define DATABASE "quote.db"

const char *progname = "excxx_repquote_gsg_simple";

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>

extern "C" {
    extern int getopt(int, char * const *, const char *);
    extern char *optarg;
}
#else
#include <errno.h>
#endif

class RepMgr
{
public:
    // Constructor.
    RepMgr();
    // Initialization method. Creates and opens our environment handle.
    int init(SimpleConfigInfo* config);
    // The doloop is where all the work is performed.
    int doloop();
    // terminate() provides our shutdown code.
    int terminate();

private:
    // Disable copy constructor.
    RepMgr(const RepMgr &);
    void operator = (const RepMgr &);

    // Internal data members.
    SimpleConfigInfo   *app_config;
    DbEnv           dbenv;

    // Private methods.
    // print_stocks() is used to display the contents of our database.
    static int print_stocks(Db *dbp);
};

static void usage()
{
    cerr << "usage: " << progname << " -h home" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    SimpleConfigInfo config;
    char ch;
    int ret;

    // Extract the command line parameters
    while ((ch = getopt(argc, argv, "h:")) != EOF) {
        switch (ch) {
        case 'h':
            config.home = optarg;
            break;
        case '?':
        default:
            usage();
        }
    }

    // Error check command line.
    if (config.home == NULL)
        usage();

    RepMgr runner;
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

RepMgr::RepMgr() : app_config(0), dbenv(0)
{
}

int RepMgr::init(SimpleConfigInfo *config)
{
    int ret = 0;

    app_config = config;

    dbenv.set_errfile(stderr);
    dbenv.set_errpfx(progname);


    // We can now open our environment.
    dbenv.set_cachesize(0, CACHESIZE, 0);
    dbenv.set_flags(DB_TXN_NOSYNC, 1);

    try {
        dbenv.open(app_config->home, DB_CREATE | DB_RECOVER |
            DB_THREAD | DB_INIT_LOCK | DB_INIT_LOG | 
            DB_INIT_MPOOL | DB_INIT_TXN, 0);
    } catch(DbException dbe) {
        cerr << "Caught an exception during DB environment open." << endl
            << "Ensure that the home directory is created prior to starting"
            << " the application." << endl;
        ret = ENOENT;
        goto err;
    }

err:
    return ret;
}

int RepMgr::terminate()
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
int RepMgr::doloop()
{
    Db *dbp;
    Dbt key, data;
    char buf[BUFSIZE], *rbuf;
    int ret;

    dbp = NULL;
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    ret = 0;

    for (;;) {
        if (dbp == NULL) {
            dbp = new Db(&dbenv, 0);

            try {
                dbp->open(NULL, DATABASE, NULL, DB_BTREE,
                    DB_CREATE | DB_AUTO_COMMIT, 0);
            } catch(DbException dbe) {
                dbenv.err(ret, "DB->open");
                throw dbe;
            }
        }

        cout << "QUOTESERVER" ;
        cout << "> " << flush;

        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        if (strtok(&buf[0], " \t\n") == NULL) {
            switch ((ret = print_stocks(dbp))) {
            case 0:
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

err:
    if (dbp != NULL)
        (void)dbp->close(DB_NOSYNC);

    return (ret);
}

// Display all the stock quote information in the database.
int RepMgr::print_stocks(Db *dbp)
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
        return (0);
    default:
        return (ret);
    }
}

