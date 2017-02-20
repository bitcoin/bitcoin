/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * Do some regression tests for constructors.
 * Run normally (without arguments) it is a simple regression test.
 * Run with a numeric argument, it repeats the regression a number
 * of times, to try to determine if there are memory leaks.
 */
#include <sys/types.h>

#include <iostream.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <iomanip.h>
#include <db_cxx.h>

#define ERR(a)  \
    do { \
      cout << "FAIL: " << (a) << "\n"; sysexit(1); \
    } while (0)

#define ERR2(a1,a2)  \
    do { \
      cout << "FAIL: " << (a1) << ": " << (a2) << "\n"; sysexit(1); \
    } while (0)

#define ERR3(a1,a2,a3)  \
    do { \
      cout << "FAIL: " << (a1) << ": " << (a2) << ": " << (a3) << "\n"; sysexit(1); \
    } while (0)

#define CHK(a)   \
    do { \
      int _ret; \
      if ((_ret = (a)) != 0) { \
     ERR3("DB function " #a " has bad return", _ret, DbEnv::strerror(_ret)); \
      } \
    } while (0)

#ifdef VERBOSE
#define DEBUGOUT(a)          cout << a << "\n"
#else
#define DEBUGOUT(a)
#endif

#define CONSTRUCT01_DBNAME         "construct01.db"
#define CONSTRUCT01_DBDIR          "."
#define CONSTRUCT01_DBFULLPATH     (CONSTRUCT01_DBDIR "/" CONSTRUCT01_DBNAME)

int itemcount;          // count the number of items in the database

// A good place to put a breakpoint...
//
void sysexit(int status)
{
    exit(status);
}

void check_file_removed(const char *name, int fatal)
{
    unlink(name);
#if 0
    if (access(name, 0) == 0) {
        if (fatal)
            cout << "FAIL: ";
        cout << "File \"" << name << "\" still exists after run\n";
        if (fatal)
            sysexit(1);
    }
#endif
}

// Check that key/data for 0 - count-1 are already present,
// and write a key/data for count.  The key and data are
// both "0123...N" where N == count-1.
//
// For some reason on Windows, we need to open using the full pathname
// of the file when there is no environment, thus the 'has_env'
// variable.
//
void rundb(Db *db, int count, int has_env)
{
    const char *name;

    if (has_env)
        name = CONSTRUCT01_DBNAME;
    else
        name = CONSTRUCT01_DBFULLPATH;

    db->set_error_stream(&cerr);

    // We don't really care about the pagesize, but we do want
    // to make sure adjusting Db specific variables works before
    // opening the db.
    //
    CHK(db->set_pagesize(1024));
    CHK(db->open(NULL, name, NULL, DB_BTREE, count ? 0 : DB_CREATE, 0664));

    // The bit map of keys we've seen
    long bitmap = 0;

    // The bit map of keys we expect to see
    long expected = (1 << (count+1)) - 1;

    char outbuf[10];
    int i;
    for (i=0; i<count; i++) {
        outbuf[i] = '0' + i;
    }
    outbuf[i++] = '\0';
    Dbt key(outbuf, i);
    Dbt data(outbuf, i);

    DEBUGOUT("Put: " << outbuf);
    CHK(db->put(0, &key, &data, DB_NOOVERWRITE));

    // Acquire a cursor for the table.
    Dbc *dbcp;
    CHK(db->cursor(NULL, &dbcp, 0));

    // Walk through the table, checking
    Dbt readkey;
    Dbt readdata;
    while (dbcp->get(&readkey, &readdata, DB_NEXT) == 0) {
        char *key_string = (char *)readkey.get_data();
        char *data_string = (char *)readdata.get_data();
        DEBUGOUT("Got: " << key_string << ": " << data_string);
        int len = strlen(key_string);
        long bit = (1 << len);
        if (len > count) {
            ERR("reread length is bad");
        }
        else if (strcmp(data_string, key_string) != 0) {
            ERR("key/data don't match");
        }
        else if ((bitmap & bit) != 0) {
            ERR("key already seen");
        }
        else if ((expected & bit) == 0) {
            ERR("key was not expected");
        }
        else {
            bitmap |= bit;
            expected &= ~(bit);
            for (i=0; i<len; i++) {
                if (key_string[i] != ('0' + i)) {
                    cout << " got " << key_string
                         << " (" << (int)key_string[i] << ")"
                         << ", wanted " << i
                         << " (" << (int)('0' + i) << ")"
                         << " at position " << i << "\n";
                    ERR("key is corrupt");
                }
            }
        }
    }
    if (expected != 0) {
        cout << " expected more keys, bitmap is: " << expected << "\n";
        ERR("missing keys in database");
    }
    CHK(dbcp->close());
    CHK(db->close(0));
}

void t1(int except_flag)
{
    cout << "  Running test 1:\n";
    Db db(0, except_flag);
    rundb(&db, itemcount++, 0);
    cout << "  finished.\n";
}

void t2(int except_flag)
{
    cout << "  Running test 2:\n";
    Db db(0, except_flag);
    rundb(&db, itemcount++, 0);
    cout << "  finished.\n";
}

void t3(int except_flag)
{
    cout << "  Running test 3:\n";
    Db db(0, except_flag);
    rundb(&db, itemcount++, 0);
    cout << "  finished.\n";
}

void t4(int except_flag)
{
    cout << "  Running test 4:\n";
    DbEnv env(except_flag);
    CHK(env.open(CONSTRUCT01_DBDIR, DB_CREATE | DB_INIT_MPOOL, 0));
    Db db(&env, 0);
    CHK(db.close(0));
    CHK(env.close(0));
    cout << "  finished.\n";
}

void t5(int except_flag)
{
    cout << "  Running test 5:\n";
    DbEnv env(except_flag);
    CHK(env.open(CONSTRUCT01_DBDIR, DB_CREATE | DB_INIT_MPOOL, 0));
    Db db(&env, 0);
    rundb(&db, itemcount++, 1);
    // Note we cannot reuse the old Db!
    Db anotherdb(&env, 0);

    anotherdb.set_errpfx("test5");
    rundb(&anotherdb, itemcount++, 1);
    CHK(env.close(0));
    cout << "  finished.\n";
}

void t6(int except_flag)
{
    cout << "  Running test 6:\n";

    /* From user [#2939] */
    int err;

    DbEnv* penv = new DbEnv(DB_CXX_NO_EXCEPTIONS);
    penv->set_cachesize(0, 32 * 1024, 0);
    penv->open(CONSTRUCT01_DBDIR, DB_CREATE | DB_PRIVATE | DB_INIT_MPOOL, 0);

    //LEAK: remove this block and leak disappears
    Db* pdb = new Db(penv,0);
    if ((err = pdb->close(0)) != 0) {
        fprintf(stderr, "Error closing Db: %s\n", db_strerror(err));
    }
    delete pdb;
    //LEAK: remove this block and leak disappears

    if ((err = penv->close(0)) != 0) {
        fprintf(stderr, "Error closing DbEnv: %s\n", db_strerror(err));
    }
    delete penv;

    cout << "  finished.\n";
}

// remove any existing environment or database
void removeall()
{
    {
    DbEnv tmpenv(DB_CXX_NO_EXCEPTIONS);
    (void)tmpenv.remove(CONSTRUCT01_DBDIR, DB_FORCE);
    }

    check_file_removed(CONSTRUCT01_DBFULLPATH, 1);
    for (int i=0; i<8; i++) {
        char buf[20];
        sprintf(buf, "__db.00%d", i);
        check_file_removed(buf, 1);
    }
}

int doall(int except_flag)
{
    itemcount = 0;
    try {
        // before and after the run, removing any
        // old environment/database.
        //
        removeall();
        t1(except_flag);
        t2(except_flag);
        t3(except_flag);
        t4(except_flag);
        t5(except_flag);
        t6(except_flag);

        removeall();
        return 0;
    }
    catch (DbException &dbe) {
        ERR2("EXCEPTION RECEIVED", dbe.what());
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int iterations = 1;
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations < 0) {
            ERR("Usage:  construct01 count");
        }
    }
    for (int i=0; i<iterations; i++) {
        if (iterations != 0) {
            cout << "(" << i << "/" << iterations << ") ";
        }
        cout << "construct01 running:\n";
        if (doall(DB_CXX_NO_EXCEPTIONS) != 0) {
            ERR("SOME TEST FAILED FOR NO-EXCEPTION TEST");
        }
        else if (doall(0) != 0) {
            ERR("SOME TEST FAILED FOR EXCEPTION TEST");
        }
        else {
            cout << "\nALL TESTS SUCCESSFUL\n";
        }
    }
    return 0;
}
