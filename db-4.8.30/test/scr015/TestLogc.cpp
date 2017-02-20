/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

/*
 * A basic regression test for the Logc class.
 */

#include <db_cxx.h>
#include <iostream.h>

static void show_dbt(ostream &os, Dbt *dbt)
{
    int i;
    int size = dbt->get_size();
    unsigned char *data = (unsigned char *)dbt->get_data();

    os << "size: " << size << " data: ";
    for (i=0; i<size && i<10; i++) {
        os << (int)data[i] << " ";
    }
    if (i<size)
        os << "...";
}

int main(int argc, char *argv[])
{
    try {
        DbEnv *env = new DbEnv(0);
        DbTxn *dbtxn;
        env->open(".", DB_CREATE | DB_INIT_LOG | 
              DB_INIT_TXN | DB_INIT_MPOOL, 0);

        // Do some database activity to get something into the log.
        Db *db1 = new Db(env, 0);
        env->txn_begin(NULL, &dbtxn, DB_TXN_NOWAIT);
        db1->open(dbtxn, "first.db", NULL, DB_BTREE, DB_CREATE, 0);
        Dbt *key = new Dbt((char *)"a", 1);
        Dbt *data = new Dbt((char *)"b", 1);
        db1->put(dbtxn, key, data, 0);
        key->set_data((char *)"c");
        data->set_data((char *)"d");
        db1->put(dbtxn, key, data, 0);
        dbtxn->commit(0);

        env->txn_begin(NULL, &dbtxn, DB_TXN_NOWAIT);
        key->set_data((char *)"e");
        data->set_data((char *)"f");
        db1->put(dbtxn, key, data, 0);
        key->set_data((char *)"g");
        data->set_data((char *)"h");
        db1->put(dbtxn, key, data, 0);
        dbtxn->commit(0);
        db1->close(0);

         // flush the log
                env->log_flush(NULL);

        // Now get a log cursor and walk through.
        DbLogc *logc;

        env->log_cursor(&logc, 0);
        int ret = 0;
        DbLsn lsn;
        Dbt *dbt = new Dbt();
        u_int32_t flags = DB_FIRST;

        int count = 0;
        while ((ret = logc->get(&lsn, dbt, flags)) == 0) {

            // We ignore the contents of the log record,
            // it's not portable.  Even the exact count
            // is may change when the underlying implementation
            // changes, we'll just make sure at the end we saw
            // 'enough'.
            //
            //      cout << "logc.get: " << count;
            //      show_dbt(cout, dbt);
            //  cout << "\n";
            //
            count++;
            flags = DB_NEXT;
        }
        if (ret != DB_NOTFOUND) {
            cerr << "*** FAIL: logc.get returned: "
                 << DbEnv::strerror(ret) << "\n";
        }
        logc->close(0);

        // There has to be at *least* 12 log records,
        // 2 for each put, plus for commits.
        //
        if (count < 12)
            cerr << "*** FAIL: not enough log records\n";

        cout << "TestLogc done.\n";
    }
    catch (DbException &dbe) {
        cerr << "*** FAIL: " << dbe.what() <<"\n";
    }
    return 0;
}
