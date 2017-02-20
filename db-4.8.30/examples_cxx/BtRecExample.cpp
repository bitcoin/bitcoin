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
#include <iomanip>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <db_cxx.h>

using std::cout;
using std::cerr;

#define DATABASE    "access.db"
#define WORDLIST    "../test/wordlist"

const char *progname = "BtRecExample";      // Program name.

class BtRecExample
{
public:
    BtRecExample(FILE *fp);
    ~BtRecExample();
    void run();
    void stats();
    void show(const char *msg, Dbt *key, Dbt *data);

private:
    Db *dbp;
    Dbc *dbcp;
};

BtRecExample::BtRecExample(FILE *fp)
{
    char *p, *t, buf[1024], rbuf[1024];
    int ret;

    // Remove the previous database.
    (void)remove(DATABASE);

    dbp = new Db(NULL, 0);

    dbp->set_error_stream(&cerr);
    dbp->set_errpfx(progname);
    dbp->set_pagesize(1024);            // 1K page sizes.

    dbp->set_flags(DB_RECNUM);          // Record numbers.
    dbp->open(NULL, DATABASE, NULL, DB_BTREE, DB_CREATE, 0664);

    //
    // Insert records into the database, where the key is the word
    // preceded by its record number, and the data is the same, but
    // in reverse order.
    //

    for (int cnt = 1; cnt <= 1000; ++cnt) {
        (void)sprintf(buf, "%04d_", cnt);
        if (fgets(buf + 4, sizeof(buf) - 4, fp) == NULL)
            break;
        u_int32_t len = (u_int32_t)strlen(buf);
        buf[len - 1] = '\0';
        for (t = rbuf, p = buf + (len - 2); p >= buf;)
            *t++ = *p--;
        *t++ = '\0';

        // As a convenience for printing, we include the null terminator
        // in the stored data.
        //
        Dbt key(buf, len);
        Dbt data(rbuf, len);

        if ((ret = dbp->put(NULL, &key, &data, DB_NOOVERWRITE)) != 0) {
            dbp->err(ret, "Db::put");
            if (ret != DB_KEYEXIST)
                throw DbException(ret);
        }
    }
}

BtRecExample::~BtRecExample()
{
    if (dbcp != 0)
        dbcp->close();
    dbp->close(0);
    delete dbp;
}

//
// Print out the number of records in the database.
//
void BtRecExample::stats()
{
    DB_BTREE_STAT *statp;

    dbp->stat(NULL, &statp, 0);
    cout << progname << ": database contains "
         << (u_long)statp->bt_ndata << " records\n";

    // Note: must use free, not delete.
    // This struct is allocated by C.
    //
    free(statp);
}

void BtRecExample::run()
{
    db_recno_t recno;
    int ret;
    char buf[1024];

    // Acquire a cursor for the database.
    dbp->cursor(NULL, &dbcp, 0);

    //
    // Prompt the user for a record number, then retrieve and display
    // that record.
    //
    for (;;) {
        // Get a record number.
        cout << "recno #> ";
        cout.flush();
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;
        recno = atoi(buf);

        //
        // Start with a fresh key each time,
        // the dbp->get() routine returns
        // the key and data pair, not just the key!
        //
        Dbt key(&recno, sizeof(recno));
        Dbt data;

        if ((ret = dbcp->get(&key, &data, DB_SET_RECNO)) != 0) {
            dbp->err(ret, "DBcursor->get");
            throw DbException(ret);
        }

        // Display the key and data.
        show("k/d\t", &key, &data);

        // Move the cursor a record forward.
        if ((ret = dbcp->get(&key, &data, DB_NEXT)) != 0) {
            dbp->err(ret, "DBcursor->get");
            throw DbException(ret);
        }

        // Display the key and data.
        show("next\t", &key, &data);

        //
        // Retrieve the record number for the following record into
        // local memory.
        //
        data.set_data(&recno);
        data.set_size(sizeof(recno));
        data.set_ulen(sizeof(recno));
        data.set_flags(data.get_flags() | DB_DBT_USERMEM);

        if ((ret = dbcp->get(&key, &data, DB_GET_RECNO)) != 0) {
            if (ret != DB_NOTFOUND && ret != DB_KEYEMPTY) {
                dbp->err(ret, "DBcursor->get");
                throw DbException(ret);
            }
        }
        else {
            cout << "retrieved recno: " << (u_long)recno << "\n";
        }
    }

    dbcp->close();
    dbcp = NULL;
}

//
// show --
//  Display a key/data pair.
//
void BtRecExample::show(const char *msg, Dbt *key, Dbt *data)
{
    cout << msg << (char *)key->get_data()
         << " : " << (char *)data->get_data() << "\n";
}

int
main()
{
    FILE *fp;

    // Open the word database.
    if ((fp = fopen(WORDLIST, "r")) == NULL) {
        fprintf(stderr, "%s: open %s: %s\n",
            progname, WORDLIST, db_strerror(errno));
        return (EXIT_FAILURE);
    }

    try {
        BtRecExample app(fp);

        // Close the word database.
        (void)fclose(fp);
        fp = NULL;

        app.stats();
        app.run();
    }
    catch (DbException &dbe) {
        cerr << "Exception: " << dbe.what() << "\n";
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}
