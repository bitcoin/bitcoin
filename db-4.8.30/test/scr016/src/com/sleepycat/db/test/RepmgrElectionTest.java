/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

package com.sleepycat.db.test;

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
import java.util.Vector;

import com.sleepycat.db.*;

public class RepmgrElectionTest extends EventHandlerAdapter implements Runnable
{
    static String address = "localhost";
    static int    basePort = 4242;
    static String baseDirName = "";
    File homedir;
    EnvironmentConfig envConfig;
    Environment dbenv;

    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
	    baseDirName = TestUtils.BASETEST_DBDIR + "/TESTDIR";
    }

    @AfterClass public static void ClassShutdown() {
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
        for(int j = 0; j < NUM_WORKER_THREADS; j++)
        {
            String homedirName = baseDirName+j;
            TestUtils.removeDir(homedirName);
        }
    }
  
    private static boolean lastSiteStarted = false;
    private static int NUM_WORKER_THREADS = 5;
    @Test(timeout=180000) public void startConductor()
    {
        Vector<RepmgrElectionTest> workers = new Vector<RepmgrElectionTest>(NUM_WORKER_THREADS);
        // start the worker threads
        for (int i = 0; i < NUM_WORKER_THREADS; i++) {
            RepmgrElectionTest worker = new RepmgrElectionTest(i);
            worker.run();
            workers.add(worker);
            /*
            while (!lastSiteStarted) {
            try {
            java.lang.Thread.sleep(10);
            }catch(InterruptedException e){}
            }
            lastSiteStarted = false;
            */
        }
     
        // stop the master - ensure the client with the highest priority is elected.
     
        // re-start original master. Call election ensure correct client is elected
    }

    /*
     * Worker thread implementation
     */
    private final static int priorities[] = {100, 75, 50, 50, 25};
    private int threadNumber;
    public RepmgrElectionTest() {
        // needed to comply with JUnit, since there is also another constructor.
    }
    RepmgrElectionTest(int threadNumber) {
        this.threadNumber = threadNumber;
    }
 
    public void run() {
        EnvironmentConfig envConfig;
        Environment dbenv = null;
        TestUtils.DEBUGOUT(1, "Creating worker: " + threadNumber);
        try {
            File homedir = new File(baseDirName + threadNumber);

            if (homedir.exists()) {
                // The following will fail if the directory contains sub-dirs.
                if (homedir.isDirectory()) {
                    File[] contents = homedir.listFiles();
                    for (int i = 0; i < contents.length; i++)
                        contents[i].delete();
                }
                homedir.delete();
            }
            homedir.mkdir();
        } catch (Exception e) {
            TestUtils.DEBUGOUT(2, "Warning: initialization had a problem creating a clean directory.\n"+e);
        }
        try {
            homedir = new File(baseDirName+threadNumber);
        } catch (NullPointerException npe) {
            // can't really happen :)
        }
        envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(TestUtils.getErrorStream());
        envConfig.setErrorPrefix("RepmgrElectionTest test("+threadNumber+")");
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

        ReplicationHostAddress haddr = new ReplicationHostAddress(address, basePort+threadNumber);
        envConfig.setReplicationManagerLocalSite(haddr);
        envConfig.setReplicationPriority(priorities[threadNumber]);
        envConfig.setEventHandler(this);
        envConfig.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ALL);
     
     
        try {
            dbenv = new Environment(homedir, envConfig);
	    
        } catch(FileNotFoundException e) {
            fail("Unexpected FNFE in standard environment creation." + e);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from environment create." + dbe);
        }
     
        try {
            /*
             * If all threads are started with REP_ELECTION flag
             * The whole system freezes, and I get:
             * RepmgrElectionTest test(0): Waiting for handle count (1) or msg_th (0) to complete replication lockout
             * Repeated every minute.
             */
	    envConfig = dbenv.getConfig();
	    for(int existingSites = 0; existingSites < threadNumber; existingSites++)
	    {
		/*
                 * This causes warnings to be produced - it seems only
                 * able to make a connection to the master site, not other
                 * client sites.
                 * The documentation and code lead me to believe this is not
                 * as expected - so leaving in here for now.
                 */
                ReplicationHostAddress host = new ReplicationHostAddress(
		    address, basePort+existingSites);
                envConfig.replicationManagerAddRemoteSite(host, false);
	    }
	    dbenv.setConfig(envConfig);            
	    if(threadNumber == 0)
                dbenv.replicationManagerStart(NUM_WORKER_THREADS, ReplicationManagerStartPolicy.REP_MASTER);
            else
                dbenv.replicationManagerStart(NUM_WORKER_THREADS, ReplicationManagerStartPolicy.REP_CLIENT);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from replicationManagerStart." + dbe);
        }
        TestUtils.DEBUGOUT(1, "Started replication site: " + threadNumber);
        lastSiteStarted = true;
        try {
            java.lang.Thread.sleep(10000);
        }catch(InterruptedException ie) {}
        try {
            dbenv.close();
            Environment.remove(homedir, false, envConfig);
        } catch(FileNotFoundException fnfe) {
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came during shutdown." + dbe);
        }
    }

    /*
     * End worker thread implementation
     */
    public void handleRepMasterEvent() {
        TestUtils.DEBUGOUT(1, "Got a REP_MASTER message");
        TestUtils.DEBUGOUT(1, "My priority: " + priorities[threadNumber]);
    }

    public void handleRepClientEvent() {
        TestUtils.DEBUGOUT(1, "Got a REP_CLIENT message");         
    }

    public void handleRepNewMasterEvent() {
        TestUtils.DEBUGOUT(1, "Got a REP_NEW_MASTER message");
        TestUtils.DEBUGOUT(1, "My priority: " + priorities[threadNumber]);
    }
}
