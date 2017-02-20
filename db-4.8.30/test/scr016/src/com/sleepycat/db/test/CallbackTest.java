/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

/*
 * Alexg 23-4-06
 * Based on scr016 TestCallback test application.
 *
 * Simple tests for DbErrorHandler, DbFeedbackHandler, DbPanicHandler
 */

package com.sleepycat.db.test;

import org.junit.Before;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.FileNotFoundException;
import com.sleepycat.db.*;

import com.sleepycat.db.DatabaseException;

import com.sleepycat.db.test.TestUtils;

public class CallbackTest
    implements FeedbackHandler, PanicHandler, ErrorHandler {

    public static final String CALLBACKTEST_DBNAME  =  "callbacktest.db";

    int callback_count = 0;
    boolean callback_throws = false;

    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(CALLBACKTEST_DBNAME), true, true);
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(CALLBACKTEST_DBNAME), true, true);
    }

    @Before public void PerTestInit()
        throws Exception {
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(CALLBACKTEST_DBNAME));
    }

    @After public void PerTestShutdown()
        throws Exception {
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(CALLBACKTEST_DBNAME));
    }

    /*
     * Test creating a new database.
     */
    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.debug_level = 2;
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setTransactional(true);
        envc.setInitializeLocking(true);
        envc.setCacheSize(64 * 1024);
        envc.setFeedbackHandler(this);
        envc.setPanicHandler(this);
        envc.setErrorHandler(this);
    	Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // set up a transaction DB.
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database db = dbEnv.openDatabase(null, CALLBACKTEST_DBNAME, null, dbConfig);

        DatabaseEntry key1 = new DatabaseEntry("key".getBytes());
        DatabaseEntry data1 = new DatabaseEntry("data".getBytes());
        // populate was doing some more than this (validating that not retrieving things that were not added)
        db.putNoOverwrite(null, key1, data1);
//        TestUtil.populate(db);

        CheckpointConfig cpcfg = new CheckpointConfig();
        cpcfg.setForce(true);
        dbEnv.checkpoint(cpcfg);

        try {
            dbConfig.setBtreeComparator(null);
        }
        catch (IllegalArgumentException dbe)
        {
            TestUtils.DEBUGOUT(1, "got expected exception: " + dbe);
            // ignore
        }

        /*
        // Pretend we crashed, and reopen the environment
        db = null;
        dbenv = null;

        dbenv = new DbEnv(0);
        dbenv.setFeedbackHandler(this);
        dbenv.open(".", Db.DB_INIT_LOCK | Db.DB_INIT_MPOOL | Db.DB_INIT_LOG
                   | Db.DB_INIT_TXN | Db.DB_RECOVER, 0);
        */

        dbEnv.panic(true);
        try {
            DatabaseEntry key = new DatabaseEntry("foo".getBytes());
            DatabaseEntry data = new DatabaseEntry();
            db.get(null, key, data, null);
        }
        catch (DatabaseException dbe2)
        {
            TestUtils.DEBUGOUT(2, "got expected exception: " + dbe2);
            // ignore
        }

    }

    /*
     * FeedbackHandler interface implementation.
     */
    public void recoveryFeedback(Environment dbenv, int percent)
    {
        TestUtils.DEBUGOUT(2, "recoveryFeedback callback invoked. percent: " + percent);
    }
    public void upgradeFeedback(Database db, int percent)
    {
        TestUtils.DEBUGOUT(2, "upgradeFeedback callback invoked. percent: " + percent);
    }
    public void verifyFeedback(Database db, int percent)
    {
        TestUtils.DEBUGOUT(2, "verifyFeedback callback invoked. percent: " + percent);
    }
 
    /*
     * Panic handler interface implementation.
     */
    public void panic(Environment dbenv, DatabaseException e)
    {
        TestUtils.DEBUGOUT(2, "panic callback invoked. exception: " + e);
    }

     /*
      * Error handler interface implementation.
      */
    public void error(Environment dbenv, String errpfx, String msg)
    {
        TestUtils.DEBUGOUT(2, "error callback invoked, errpfx: \"" + errpfx + "\", msg: \"" + msg + "\"");
    }
}
