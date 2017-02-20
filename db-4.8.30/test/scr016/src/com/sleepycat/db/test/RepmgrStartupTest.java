/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 */

/*
 * A test case that brings up the replication
 * manager infrastructure as master. Then shut
 * the master down cleanly.
 * This case does not have any replication clients
 * or even update the underlying DB.
 */

package com.sleepycat.db.test;

import com.sleepycat.db.test.TestUtils;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import junit.framework.JUnit4TestAdapter;

import java.io.File;
import java.io.FileNotFoundException;

import com.sleepycat.db.*;

public class RepmgrStartupTest extends EventHandlerAdapter
{
    static String address = "localhost";
    static int    port = 4242;
    static int    priority = 100;
    static String homedirName = "TESTDIR";
    File homedir;
    EnvironmentConfig envConfig;
    Environment dbenv;
 
    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
    }

    @AfterClass public static void ClassShutdown() {
    }

    @Before public void PerTestInit()
    {
        TestUtils.removeDir(homedirName);
        try {
            homedir = new File(homedirName);
            homedir.mkdir();
        } catch (Exception e) {
            TestUtils.DEBUGOUT(2, "Warning: initialization had a problem creating a clean directory.\n" + e);
        }
        try {
            homedir = new File(homedirName);
        } catch (NullPointerException npe) {
            // can't really happen :)
        }
        envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(TestUtils.getErrorStream());
        envConfig.setErrorPrefix("RepmgrStartupTest test");
        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setThreaded(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);
        envConfig.setTxnNoSync(true);
        envConfig.setInitializeReplication(true);
        envConfig.setVerboseReplication(false);

        ReplicationHostAddress haddr = new ReplicationHostAddress(address, port);
        envConfig.setReplicationManagerLocalSite(haddr);
        envConfig.setReplicationPriority(priority);
        envConfig.setEventHandler(this);
        envConfig.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ALL);

        try {
            dbenv = new Environment(homedir, envConfig);
        } catch(FileNotFoundException e) {
            fail("Unexpected FNFE in standard environment creation." + e);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from environment create." + dbe);
        }
    }

    @After public void PerTestShutdown()
        throws Exception {
	    try {
            File homedir = new File(homedirName);
         
            if (homedir.exists()) {
                // The following will fail if the directory contains sub-dirs.
                if (homedir.isDirectory()) {
                    File[] contents = homedir.listFiles();
                    for (int i = 0; i < contents.length; i++)
                        contents[i].delete();
                }
                homedir.delete();
            }
        } catch (Exception e) {
            TestUtils.DEBUGOUT(2, "Warning: shutdown had a problem cleaning up test directory.\n" + e);
        }
    }

 
    @Test (timeout=3000) public void startMaster()
    {
        try {
            // start replication manager
            dbenv.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from replicationManagerStart." + dbe);
        }
        try {
            java.lang.Thread.sleep(1000);
        }catch(InterruptedException ie) {}
     
        try {
            dbenv.close();
            Environment.remove(homedir, false, envConfig);
        } catch(FileNotFoundException fnfe) {
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came during shutdown." + dbe);
        }
    }
 
    @Test (timeout=3000) public void startClient()
    {
        try {
            // start replication manager
            dbenv.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_CLIENT);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from replicationManagerStart." + dbe);
        }
        try {
            java.lang.Thread.sleep(1000);
        }catch(InterruptedException ie) {}
     
        try {
            dbenv.close();
            Environment.remove(homedir, false, envConfig);
        } catch(FileNotFoundException fnfe) {
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came during shutdown." + dbe);
        }
    }

    @Test (timeout=3000) public void startElection()
    {
        try {
            // start replication manager
            dbenv.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_ELECTION);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from replicationManagerStart." + dbe);
        }
        try {
            java.lang.Thread.sleep(1000);
        }catch(InterruptedException ie) {}
     
        try {
            dbenv.close();
            Environment.remove(homedir, false, envConfig);
        } catch(FileNotFoundException fnfe) {
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came during shutdown." + dbe);
        }
    }

    @Test (timeout=15000) public void startMasterWaitBeforeShutdown()
    {
        try {
            // start replication manager
            dbenv.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from replicationManagerStart." + dbe.toString());
        }
        try {
            /*
             * NOTE! This is a bit alarming - I have seen shutdown failures with the following message:
             *
             * RepmgrStartupTest test: Waiting for handle count (1) or msg_th (0) to complete replication lockout
             *
             * When the sleep is over 10 seconds.
             */
            java.lang.Thread.sleep(12000);
        }catch(InterruptedException ie) {}
     
        try {
            dbenv.close();
            Environment.remove(homedir, false, envConfig);
        } catch(FileNotFoundException fnfe) {
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came during shutdown." + dbe.toString());
        }
    }

    public void handleRepMasterEvent() {
        TestUtils.DEBUGOUT(1, "Got a REP_MASTER message");
    }

    public void handleRepClientEvent() {
        TestUtils.DEBUGOUT(1, "Got a REP_CLIENT message");         
    }

    public void handleRepNewMasterEvent() {
        TestUtils.DEBUGOUT(1, "Got a REP_NEW_MASTER message");
    }
}
