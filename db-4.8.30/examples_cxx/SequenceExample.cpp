/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include <sys/types.h>

#include <iostream>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern "C" {
  extern int getopt(int, char * const *, const char *);
  extern int optind;
}
#else
#include <unistd.h>
#endif

#include <db_cxx.h>

#define DATABASE    "sequence.db"
#define SEQUENCE    "my_sequence"

using std::cout;
using std::cerr;

class SequenceExample
{
public:
    SequenceExample();
    void run(bool removeExistingDatabase, const char *fileName);

private:
    // no need for copy and assignment
    SequenceExample(const SequenceExample &);
    void operator = (const SequenceExample &);
};

int
usage()
{
    (void)fprintf(stderr, "usage: SequenceExample [-r] [database]\n");
    return (EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    int ch, rflag;
    const char *database;

    rflag = 0;
    while ((ch = getopt(argc, argv, "r")) != EOF)
        switch (ch) {
        case 'r':
            rflag = 1;
            break;
        case '?':
        default:
            return (usage());
        }
    argc -= optind;
    argv += optind;

    /* Accept optional database name. */
    database = *argv == NULL ? DATABASE : argv[0];

    // Use a try block just to report any errors.
    // An alternate approach to using exceptions is to
    // use error models (see DbEnv::set_error_model()) so
    // that error codes are returned for all Berkeley DB methods.
    //
    try {
        SequenceExample app;
        app.run((bool)(rflag == 1 ? true : false), database);
        return (EXIT_SUCCESS);
    }
    catch (DbException &dbe) {
        cerr << "SequenceExample: " << dbe.what() << "\n";
        return (EXIT_FAILURE);
    }
}

SequenceExample::SequenceExample()
{
}

void SequenceExample::run(bool removeExistingDatabase, const char *fileName)
{
    // Remove the previous database.
    if (removeExistingDatabase)
        (void)remove(fileName);

    // Create the database object.
    // There is no environment for this simple example.
    Db db(0, 0);

    db.set_error_stream(&cerr);
    db.set_errpfx("SequenceExample");
    db.open(NULL, fileName, NULL, DB_BTREE, DB_CREATE, 0664);

    // We put a try block around this section of code
    // to ensure that our database is properly closed
    // in the event of an error.
    //
    try {
        Dbt key((void *)SEQUENCE, (u_int32_t)strlen(SEQUENCE));
        DbSequence seq(&db, 0);
        seq.open(0, &key, DB_CREATE);

        for (int i = 0; i < 10; i++) {
            db_seq_t seqnum;
            seq.get(0, 1, &seqnum, 0);

            // We don't have a portable way to print 64-bit numbers.
            cout << "Got sequence number (" <<
                (int)(seqnum >> 32) << ", " << (unsigned)seqnum <<
                ")\n";
        }

        seq.close(0);
    } catch (DbException &dbe) {
        cerr << "SequenceExample: " << dbe.what() << "\n";
    }

    db.close(0);
}
