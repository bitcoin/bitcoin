/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import com.sleepycat.db.test.TestUtils;
public class VerboseConfigTest {
    public static final String VERBOSECONFIGTEST_DBNAME = "verboseconfigtest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(VERBOSECONFIGTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(VERBOSECONFIGTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(VERBOSECONFIGTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(VERBOSECONFIGTEST_DBNAME));
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
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
		envc.setVerbose(VerboseConfig.DEADLOCK, true);
		envc.setVerbose(VerboseConfig.FILEOPS, true);
		envc.setVerbose(VerboseConfig.FILEOPS_ALL, true);
		envc.setVerbose(VerboseConfig.RECOVERY, true);
		envc.setVerbose(VerboseConfig.REGISTER, true);
		envc.setVerbose(VerboseConfig.REPLICATION, true);
		envc.setVerbose(VerboseConfig.WAITSFOR, true);
		envc.setMessageStream(new FileOutputStream(new File("messages.txt")));
    	Environment db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

		new File("messages.txt").delete();
    }

    /*
	 * Tests for old (now deprecated) API.
	 */
    @Test public void test2()
        throws DatabaseException, FileNotFoundException
    {
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
		envc.setVerboseDeadlock(true);
		envc.setVerboseRecovery(true);
		envc.setVerboseRegister(true);
		envc.setVerboseReplication(true);
		envc.setVerboseWaitsFor(true);
		envc.setMessageStream(new FileOutputStream(new File("messages.txt")));
    	Environment db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

		new File("messages.txt").delete();
    }
}
