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
#include <stdlib.h>
#include <string.h>

#include <db_cxx.h>

using std::cin;
using std::cout;
using std::cerr;

const char *progname = "LockExample";               // Program name.

//
// An example of a program using DBLock and related classes.
//
class LockExample : public DbEnv
{
public:
    void run();
    int error_code() { return (ecode); }

    LockExample(const char *home, u_int32_t maxlocks, int do_unlink);

private:
    static const char FileName[];
    int ecode;

    // no need for copy and assignment
    LockExample(const LockExample &);
    void operator = (const LockExample &);
};

static int usage();          // forward

int
main(int argc, char *argv[])
{
    const char *home;
    int do_unlink;
    u_int32_t maxlocks;
    int i;

    home = "TESTDIR";
    maxlocks = 0;
    do_unlink = 0;
    for (int argnum = 1; argnum < argc; ++argnum) {
        if (strcmp(argv[argnum], "-h") == 0) {
            if (++argnum >= argc)
                return (usage());
            home = argv[argnum];
        }
        else if (strcmp(argv[argnum], "-m") == 0) {
            if (++argnum >= argc)
                return (usage());
            if ((i = atoi(argv[argnum])) <= 0)
                return (usage());
            maxlocks = (u_int32_t)i;  /* XXX: possible overflow. */
        }
        else if (strcmp(argv[argnum], "-u") == 0) {
            do_unlink = 1;
        }
        else {
            return (usage());
        }
    }

    try {
        int ecode;

        if (do_unlink) {
            // Create an environment that immediately
            // removes all files.
            LockExample tmp(home, maxlocks, do_unlink);
            if ((ecode = tmp.error_code()) != 0)
                return (ecode);
        }

        LockExample app(home, maxlocks, do_unlink);
        if ((ecode = app.error_code()) != 0)
            return (ecode);
        app.run();
        app.close(0);
        return (EXIT_SUCCESS);
    }
    catch (DbException &dbe) {
        cerr << "LockExample: " << dbe.what() << "\n";
        return (EXIT_FAILURE);
    }
}

LockExample::LockExample(const char *home, u_int32_t maxlocks, int do_unlink)
:   DbEnv(0)
,   ecode(0)
{
    int ret;

    if (do_unlink) {
        if ((ret = remove(home, DB_FORCE)) != 0) {
            cerr << progname << ": DbEnv::remove: "
                 << strerror(errno) << "\n";
            ecode = EXIT_FAILURE;
        }
    }
    else {
        set_error_stream(&cerr);
        set_errpfx("LockExample");
        if (maxlocks != 0)
            set_lk_max_locks(maxlocks);
        open(home, DB_CREATE | DB_INIT_LOCK, 0);
    }
}

void LockExample::run()
{
    long held;
    size_t len;
    u_int32_t locker;
    int did_get, ret;
    DbLock *locks = 0;
    int lockcount = 0;
    int lockid = 0;
    char objbuf[1024];

    //
    // Accept lock requests.
    //
    lock_id(&locker);
    for (held = 0;;) {
        cout << "Operation get/release [get]> ";
        cout.flush();

        char opbuf[16];
        cin.getline(opbuf, sizeof(opbuf));
        if (cin.eof())
            break;
        if ((len = strlen(opbuf)) <= 1 || strcmp(opbuf, "get") == 0) {
            // Acquire a lock.
            cout << "input object (text string) to lock> ";
            cout.flush();
            cin.getline(objbuf, sizeof(objbuf));
            if (cin.eof())
                break;
            if ((len = strlen(objbuf)) <= 0)
                continue;

            char lockbuf[16];
            do {
                cout << "lock type read/write [read]> ";
                cout.flush();
                cin.getline(lockbuf, sizeof(lockbuf));
                if (cin.eof())
                    break;
                len = strlen(lockbuf);
            } while (len >= 1 &&
                 strcmp(lockbuf, "read") != 0 &&
                 strcmp(lockbuf, "write") != 0);

            db_lockmode_t lock_type;
            if (len <= 1 || strcmp(lockbuf, "read") == 0)
                lock_type = DB_LOCK_READ;
            else
                lock_type = DB_LOCK_WRITE;

            Dbt dbt(objbuf, (u_int32_t)strlen(objbuf));

            DbLock lock;
            ret = lock_get(locker, DB_LOCK_NOWAIT, &dbt,
                       lock_type, &lock);
            did_get = 1;
            lockid = lockcount++;
            if (locks == NULL) {
                locks = new DbLock[1];
            }
            else {
                DbLock *newlocks = new DbLock[lockcount];
                for (int lockno = 0;
                    lockno < lockid; lockno++) {
                    newlocks[lockno] = locks[lockno];
                }
                delete locks;
                locks = newlocks;
            }
            locks[lockid] = lock;
        } else {
            // Release a lock.
            do {
                cout << "input lock to release> ";
                cout.flush();
                cin.getline(objbuf, sizeof(objbuf));
                if (cin.eof())
                    break;
            } while ((len = strlen(objbuf)) <= 0);
            lockid = strtol(objbuf, NULL, 16);
            if (lockid < 0 || lockid >= lockcount) {
                cout << "Lock #" << lockid << " out of range\n";
                continue;
            }
            DbLock lock = locks[lockid];
            ret = lock_put(&lock);
            did_get = 0;
        }

        switch (ret) {
        case 0:
            cout << "Lock #" << lockid << " "
                 <<  (did_get ? "granted" : "released")
                 << "\n";
            held += did_get ? 1 : -1;
            break;
        case DB_LOCK_NOTGRANTED:
            cout << "Lock not granted\n";
            break;
        case DB_LOCK_DEADLOCK:
            cerr << "LockExample: lock_"
                 << (did_get ? "get" : "put")
                 << ": " << "returned DEADLOCK";
            break;
        default:
            cerr << "LockExample: lock_get: %s",
                strerror(errno);
        }
    }
    cout << "\n";
    cout << "Closing lock region " << held << " locks held\n";
    if (locks != 0)
        delete locks;
}

static int
usage()
{
    cerr << "usage: LockExample [-u] [-h home] [-m maxlocks]\n";
    return (EXIT_FAILURE);
}
