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

#include <db_cxx.h>
#include <iostream.h>

int main(int argc, char *argv[])
{
    try {
        Db *db = new Db(NULL, 0);
        db->open(NULL, "my.db", NULL, DB_BTREE, DB_CREATE, 0644);

        // populate our massive database.
        // all our strings include null for convenience.
        // Note we have to cast for idiomatic
        // usage, since newer gcc requires it.
        Dbt *keydbt = new Dbt((char*)"key", 4);
        Dbt *datadbt = new Dbt((char*)"data", 5);
        db->put(NULL, keydbt, datadbt, 0);

        // Now, retrieve.  We could use keydbt over again,
        // but that wouldn't be typical in an application.
        Dbt *goodkeydbt = new Dbt((char*)"key", 4);
        Dbt *badkeydbt = new Dbt((char*)"badkey", 7);
        Dbt *resultdbt = new Dbt();
        resultdbt->set_flags(DB_DBT_MALLOC);

        int ret;

        if ((ret = db->get(NULL, goodkeydbt, resultdbt, 0)) != 0) {
            cout << "get: " << DbEnv::strerror(ret) << "\n";
        }
        else {
            char *result = (char *)resultdbt->get_data();
            cout << "got data: " << result << "\n";
        }

        if ((ret = db->get(NULL, badkeydbt, resultdbt, 0)) != 0) {
            // We expect this...
            cout << "get using bad key: "
                 << DbEnv::strerror(ret) << "\n";
        }
        else {
            char *result = (char *)resultdbt->get_data();
            cout << "*** got data using bad key!!: "
                 << result << "\n";
        }

        // Now, truncate and make sure that it's really gone.
        cout << "truncating data...\n";
        u_int32_t nrecords;
        db->truncate(NULL, &nrecords, 0);
        cout << "truncate returns " << nrecords << "\n";
        if ((ret = db->get(NULL, goodkeydbt, resultdbt, 0)) != 0) {
            // We expect this...
            cout << "after truncate get: "
                 << DbEnv::strerror(ret) << "\n";
        }
        else {
            char *result = (char *)resultdbt->get_data();
            cout << "got data: " << result << "\n";
        }

        db->close(0);
        cout << "finished test\n";
    }
    catch (DbException &dbe) {
        cerr << "Db Exception: " << dbe.what();
    }
    return 0;
}
