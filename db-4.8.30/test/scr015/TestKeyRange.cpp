/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * NOTE: AccessExample changed to test Db.key_range.  We made a global
 * change of /AccessExample/TestKeyRange/, the only other changes are
 * marked with comments that are notated as 'ADDED'.
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

class TestKeyRange
{
public:
    TestKeyRange();
    void run();

private:
    static const char FileName[];

    // no need for copy and assignment
    TestKeyRange(const TestKeyRange &);
    void operator = (const TestKeyRange &);
};

static void usage();          // forward

int main(int argc, char *argv[])
{
    if (argc > 1) {
        usage();
    }

    // Use a try block just to report any errors.
    // An alternate approach to using exceptions is to
    // use error models (see DbEnv::set_error_model()) so
    // that error codes are returned for all Berkeley DB methods.
    //
    try {
        TestKeyRange app;
        app.run();
        return 0;
    }
    catch (DbException &dbe) {
        cerr << "TestKeyRange: " << dbe.what() << "\n";
        return 1;
    }
}

static void usage()
{
    cerr << "usage: TestKeyRange\n";
    exit(1);
}

const char TestKeyRange::FileName[] = "access.db";

TestKeyRange::TestKeyRange()
{
}

void TestKeyRange::run()
{
    // Remove the previous database.
    (void)unlink(FileName);

    // Create the database object.
    // There is no environment for this simple example.
    Db db(0, 0);

    db.set_error_stream(&cerr);
    db.set_errpfx("TestKeyRange");
    db.set_pagesize(1024);      /* Page size: 1K. */
    db.set_cachesize(0, 32 * 1024, 0);
    db.open(NULL, FileName, NULL, DB_BTREE, DB_CREATE, 0664);

    //
    // Insert records into the database, where the key is the user
    // input and the data is the user input in reverse order.
    //
    char buf[1024];
    char rbuf[1024];
    char *t;
    char *p;
    int ret;
    int len;
    Dbt *firstkey = NULL;
    char firstbuf[1024];

    for (;;) {
        cout << "input>";
        cout.flush();

        cin.getline(buf, sizeof(buf));
        if (cin.eof())
            break;

        if ((len = strlen(buf)) <= 0)
            continue;
        for (t = rbuf, p = buf + (len - 1); p >= buf;)
            *t++ = *p--;
        *t++ = '\0';

        Dbt key(buf, len + 1);
        Dbt data(rbuf, len + 1);
        if (firstkey == NULL) {
            strcpy(firstbuf, buf);
            firstkey = new Dbt(firstbuf, len + 1);
        }

        ret = db.put(0, &key, &data, DB_NOOVERWRITE);
        if (ret == DB_KEYEXIST) {
            cout << "Key " << buf << " already exists.\n";
        }
        cout << "\n";
    }

    // We put a try block around this section of code
    // to ensure that our database is properly closed
    // in the event of an error.
    //
    try {
        // Acquire a cursor for the table.
        Dbc *dbcp;
        db.cursor(NULL, &dbcp, 0);

        /*ADDED...*/
        DB_KEY_RANGE range;
        memset(&range, 0, sizeof(range));

        db.key_range(NULL, firstkey, &range, 0);
        printf("less: %f\n", range.less);
        printf("equal: %f\n", range.equal);
        printf("greater: %f\n", range.greater);
        /*end ADDED*/

        Dbt key;
        Dbt data;

        // Walk through the table, printing the key/data pairs.
        while (dbcp->get(&key, &data, DB_NEXT) == 0) {
            char *key_string = (char *)key.get_data();
            char *data_string = (char *)data.get_data();
            cout << key_string << " : " << data_string << "\n";
        }
        dbcp->close();
    }
    catch (DbException &dbe) {
        cerr << "TestKeyRange: " << dbe.what() << "\n";
    }

    db.close(0);
}
