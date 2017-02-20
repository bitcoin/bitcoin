/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <errno.h>
#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <db_cxx.h>

using std::ostream;
using std::cout;
using std::cerr;

void db_setup(const char *, const char *, ostream&);
void db_teardown(const char *, const char *, ostream&);
static int usage();

const char *progname = "EnvExample";            /* Program name. */

//
// An example of a program creating/configuring a Berkeley DB environment.
//
int
main(int argc, char *argv[])
{
    //
    // Note: it may be easiest to put all Berkeley DB operations in a
    // try block, as seen here.  Alternatively, you can change the
    // ErrorModel in the DbEnv so that exceptions are never thrown
    // and check error returns from all methods.
    //
    try {
        const char *data_dir, *home;
        
        //
        // All of the shared database files live in home,
        // but data files live in data_dir.
        //
        home = "TESTDIR";
        data_dir = "data";

        for (int argnum = 1; argnum < argc; ++argnum) {
            if (strcmp(argv[argnum], "-h") == 0) {
                if (++argnum >= argc)
                    return (usage());
                home = argv[argnum];
            }
            else if (strcmp(argv[argnum], "-d") == 0) {
                if (++argnum >= argc)
                    return (usage());
                data_dir = argv[argnum];
            }
            else {
                return (usage());
            }
        }

        cout << "Setup env\n";
        db_setup(home, data_dir, cerr);

        cout << "Teardown env\n";
        db_teardown(home, data_dir, cerr);
        return (EXIT_SUCCESS);
    }
    catch (DbException &dbe) {
        cerr << "EnvExample: " << dbe.what() << "\n";
        return (EXIT_FAILURE);
    }
}

// Note that any of the db calls can throw DbException
void
db_setup(const char *home, const char *data_dir, ostream& err_stream)
{
    //
    // Create an environment object and initialize it for error
    // reporting.
    //
    DbEnv *dbenv = new DbEnv(0);
    dbenv->set_error_stream(&err_stream);
    dbenv->set_errpfx(progname);

    //
    // We want to specify the shared memory buffer pool cachesize,
    // but everything else is the default.
    //
    dbenv->set_cachesize(0, 64 * 1024, 0);

    // Databases are in a subdirectory.
    (void)dbenv->set_data_dir(data_dir);

    // Open the environment with full transactional support.
    dbenv->open(home,
        DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | 
        DB_INIT_TXN, 0);

    // Open a database in the environment to verify the data_dir
    // has been set correctly.
    // Create a database handle, using the environment. 
    Db *db = new Db(dbenv, 0) ;

    // Open the database. 
    db->open(NULL, "EvnExample_db1.db", NULL, DB_BTREE, DB_CREATE, 0644);

    // Close the database handle.
    db->close(0) ;
    delete db;

    // Close the handle.
    dbenv->close(0);
    delete dbenv;
}

void
db_teardown(const char *home, const char *data_dir, ostream& err_stream)
{
    // Remove the shared database regions.
    DbEnv *dbenv = new DbEnv(0);

    dbenv->set_error_stream(&err_stream);
    dbenv->set_errpfx(progname);

    (void)dbenv->set_data_dir(data_dir);
    dbenv->remove(home, 0);
    delete dbenv;
}

static int
usage()
{
    cerr << "usage: excxx_env [-h home] [-d data_dir]\n";
    return (EXIT_FAILURE);
}
