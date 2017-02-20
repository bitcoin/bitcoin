/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

package com.sleepycat.db.test;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.sleepycat.db.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.sleepycat.db.test.TestUtils;
public class ClosedDbTest {
    public static final String CLOSEDDBTEST_DBNAME = "closeddbtest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(CLOSEDDBTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(CLOSEDDBTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(CLOSEDDBTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(CLOSEDDBTEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
    }
    /*
     * Test case implementations.
     * To disable a test mark it with @Ignore
     * To set a timeout(ms) notate like: @Test(timeout=1000)
     * To indicate an expected exception notate like: (expected=Exception)
     */

    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
        DatabaseConfig dbConf = new DatabaseConfig();
        dbConf.setType(DatabaseType.BTREE);
        dbConf.setAllowCreate(true);
        Database db = new Database(TestUtils.getDBFileName(CLOSEDDBTEST_DBNAME), null, dbConf);
     
    	DatabaseEntry key = new DatabaseEntry("key".getBytes());
    	DatabaseEntry data = new DatabaseEntry("data".getBytes());
    	db.putNoOverwrite(null, key, data);
  
    	// Now, retrieve. It would be possible to reuse the
    	// same key object, but that would be a-typical.
    	DatabaseEntry getkey = new DatabaseEntry("key".getBytes());
    	DatabaseEntry badgetkey = new DatabaseEntry("badkey".getBytes());
    	DatabaseEntry getdata = new DatabaseEntry();
    	getdata.setReuseBuffer(false); // TODO: is this enabling DB_DBT_MALLOC?

        int ret;
     
        // close the db - subsequent operations should fail by throwing
        // an exception.
    	db.close();
  
    	try {
    	    db.get(null, getkey, getdata, LockMode.DEFAULT);
    	    fail("Database get on a closed Db should not have completed.");
    	} catch (IllegalArgumentException e) {
    	    TestUtils.DEBUGOUT(1, "Got expected exception from db.get on closed database.");
    	}

    }
}

