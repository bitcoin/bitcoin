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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.sleepycat.db.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.sleepycat.db.test.TestUtils;
public class LogCursorTest {
    public static final String LOGCURSORTEST_DBNAME = "logcursortest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(LOGCURSORTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(LOGCURSORTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(LOGCURSORTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(LOGCURSORTEST_DBNAME));
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
        Environment env;
        EnvironmentConfig envCfg;
        Database db;
        DatabaseConfig cfg;
        File home;
	int key_count = 50, lc_count = 0;
        
	envCfg = new EnvironmentConfig();
	envCfg.setAllowCreate(true);
	envCfg.setInitializeCache(true);
	envCfg.setInitializeLocking(true);
	envCfg.setInitializeLogging(true);
	envCfg.setMaxLogFileSize(32768);
	envCfg.setTransactional(true);

	env = new Environment(TestUtils.BASETEST_DBFILE, envCfg);

	cfg = new DatabaseConfig();
	cfg.setAllowCreate(true);
	cfg.setType(DatabaseType.BTREE);
	cfg.setTransactional(true);
	db = env.openDatabase(null, LOGCURSORTEST_DBNAME, null, cfg);

	for (int i =0; i < key_count; i++) {
		DatabaseEntry key = new DatabaseEntry();
		key.setData(String.valueOf(i).getBytes());
		DatabaseEntry data =new DatabaseEntry();
		data.setData(String.valueOf(500-i).getBytes());
		db.put(null, key, data);
	}

	LogCursor lc = env.openLogCursor();
	LogSequenceNumber lsn = new LogSequenceNumber();
	DatabaseEntry dbt = new DatabaseEntry();
	while (lc.getNext(lsn, dbt) == OperationStatus.SUCCESS)
	    lc_count++;
	lc.close();
	db.close();
	env.close();
	// There should be at least as many log entries as there were
	// keys inserted.
	assertTrue(lc_count > key_count);
	    
    }
}
