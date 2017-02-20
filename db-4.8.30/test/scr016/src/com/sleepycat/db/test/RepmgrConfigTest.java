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

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Ignore;
import org.junit.Test;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import junit.framework.JUnit4TestAdapter;

import java.io.File;
import java.io.FileNotFoundException;

import com.sleepycat.db.*;

public class RepmgrConfigTest extends EventHandlerAdapter
{
    private class ConfigOptions {
        public ConfigOptions(
            boolean txnSync,
            boolean initializeReplication,
            boolean verboseReplication,
            boolean setLocalSiteEnable,
            boolean setReplicationPriority,
            int replicationPriority,
            boolean setValidEventHandler,
            boolean setAckPolicy,
            ReplicationManagerAckPolicy ackPolicyToSet,
            int nsitesToStart,
            boolean validStartPolicy,
	    int requestMin,
	    int requestMax,
            boolean validPolicy
            )
        {
            this.txnSync                 = txnSync;
            this.initializeReplication   = initializeReplication;
            this.verboseReplication      = verboseReplication;
            this.setLocalSiteEnable      = setLocalSiteEnable;
            this.setReplicationPriority  = setReplicationPriority;
            this.replicationPriority     = replicationPriority;
            this.setValidEventHandler    = setValidEventHandler;
            this.setAckPolicy            = setAckPolicy;
            this.ackPolicyToSet          = ackPolicyToSet;
            this.nsitesToStart           = nsitesToStart;
            this.validStartPolicy        = validStartPolicy;
            this.validPolicy             = validPolicy;
	    this.requestMin              = requestMin;
	    this.requestMax              = requestMax;
	}
         
        boolean txnSync;
        boolean initializeReplication;
        boolean verboseReplication;
        boolean setLocalSiteEnable;
        boolean setValidEventHandler;
        boolean setReplicationPriority;
        int replicationPriority;
        boolean setAckPolicy;
        ReplicationManagerAckPolicy ackPolicyToSet;
        int nsitesToStart;
        boolean validStartPolicy;
	int requestMin;
        int requestMax;
	    
        // should this set of options work or not?
        boolean validPolicy;
    }
    static String address     = "localhost";
    static int    port        = 4242;
    static int    priority    = 100;
    static String homedirName = "";
	
    ConfigOptions[] optionVariations =
        { new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, true ), //0:
          new ConfigOptions(false, true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, true ), //1: disable txnSync
          new ConfigOptions(true,  false, false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, false ), //2: don't call initRep
          new ConfigOptions(true,  true,  true,  true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, true ), //3: enable verbose rep
          new ConfigOptions(true,  true,  false, false, true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, false ), //4: don't set a local site
          new ConfigOptions(true,  true,  false, true,  false, 50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, true ), //5: don't assign priority explicitly
          new ConfigOptions(true,  true,  false, true,  true,  -1, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, true ), //6: ??assign an invalid priority.
          new ConfigOptions(true,  true,  false, true,  true,  50, false, true,  ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, false ), //7: don't set the event handler.
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  false, ReplicationManagerAckPolicy.ALL,       1, true, 3, 9, true ), //8: ?? don't set ack policy
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL_PEERS, 1, true, 3, 9, true ), //9:
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.NONE,      1, true, 3, 9, true ), //10:
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ONE,       1, true, 3, 9, true ), //11:
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ONE_PEER,  1, true, 3, 9, true ), //12:
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  null,                                  1, true, 3, 9, false ), //13: set an invalid ack policy
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,      -1, true, 3, 9, false ), //14: set nsites negative
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       0, true, 3, 9, false ), //15:
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, false, 3, 9, false ), //16: dont set a valid start policy.
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 0, 9, false ), //17: set requestMin as 0
          new ConfigOptions(true,  true,  false, true,  true,  50, true,  true,  ReplicationManagerAckPolicy.ALL,       1, true, 9, 3, false ), //18: set requestMax < requestMin
        };
    File homedir;
    EnvironmentConfig envConfig;
 
    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
	    homedirName = TestUtils.BASETEST_DBDIR + "/TESTDIR";
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
        envConfig.setErrorPrefix("RepmgrConfigTest test");
        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setThreaded(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);

        // Linux seems to have problems cleaning up its sockets.
		// so start each test at a new address.
		++port;
    }

    @After public void PerTestShutdown()
        throws Exception {
        TestUtils.removeDir(homedirName);
    }

    @Test (timeout=3000) public void TestOptions0()
    {
        assertTrue(runTestWithOptions(optionVariations[0]));
    }
    @Test (timeout=3000) public void TestOptions1()
    {
        assertTrue(runTestWithOptions(optionVariations[1]));
    }
    @Test (timeout=3000) public void TestOptions2()
    {
        assertTrue(runTestWithOptions(optionVariations[2]));
    }
    @Test (timeout=3000) public void TestOptions3()
    {
        assertTrue(runTestWithOptions(optionVariations[3]));
    }
    @Test (timeout=3000) public void TestOptions4()
    {
        assertTrue(runTestWithOptions(optionVariations[4]));
    }
    @Test (timeout=3000) public void TestOptions5()
    {
        assertTrue(runTestWithOptions(optionVariations[5]));
    }
    @Test (timeout=3000) public void TestOptions6()
    {
        assertTrue(runTestWithOptions(optionVariations[6]));
    }
    @Ignore("Currently failing") @Test (timeout=3000) public void TestOptions7()
    {
        assertTrue(runTestWithOptions(optionVariations[7]));
    }
    @Test (timeout=3000) public void TestOptions8()
    {
        assertTrue(runTestWithOptions(optionVariations[8]));
    }
    @Test (timeout=3000) public void TestOptions9()
    {
        assertTrue(runTestWithOptions(optionVariations[9]));
    }
    @Test (timeout=3000) public void TestOptions10()
    {
        assertTrue(runTestWithOptions(optionVariations[10]));
    }
    @Test (timeout=3000) public void TestOptions11()
    {
        assertTrue(runTestWithOptions(optionVariations[11]));
    }
    @Test (timeout=3000) public void TestOptions12()
    {
        assertTrue(runTestWithOptions(optionVariations[12]));
    }
    @Test (timeout=3000) public void TestOptions13()
    {
        assertTrue(runTestWithOptions(optionVariations[13]));
    }
    @Test (timeout=3000) public void TestOptions14()
    {
        assertTrue(runTestWithOptions(optionVariations[14]));
    }
    @Test (timeout=3000) public void TestOptions15()
    {
        assertTrue(runTestWithOptions(optionVariations[15]));
    }
    @Test (timeout=3000) public void TestOptions16()
    {
        assertTrue(runTestWithOptions(optionVariations[16]));
    }
    @Test (timeout=3000) public void TestOptions17()
    {
        assertTrue(runTestWithOptions(optionVariations[17]));
    }
    @Test (timeout=3000) public void TestOptions18()
    {
        assertTrue(runTestWithOptions(optionVariations[18]));
    }

    // returns true if failure matches options failure spec.
    boolean runTestWithOptions(ConfigOptions opts)
    {
        boolean retval = true;
        boolean gotexcept = false;
        Environment dbenv = null;
        try {
 
            envConfig.setTxnNoSync(opts.txnSync);
            if (opts.initializeReplication)
                envConfig.setInitializeReplication(true);
            if (opts.verboseReplication)
                envConfig.setVerboseReplication(false);
 
            if (opts.setLocalSiteEnable) {
                ReplicationHostAddress haddr = new ReplicationHostAddress(address, port);
                envConfig.setReplicationManagerLocalSite(haddr);
            }
            if (opts.setReplicationPriority)
                envConfig.setReplicationPriority(opts.replicationPriority);
            if (opts.setValidEventHandler)
                envConfig.setEventHandler(this);
         
            if (opts.setAckPolicy)
                envConfig.setReplicationManagerAckPolicy(opts.ackPolicyToSet);
         
	    envConfig.setReplicationRequestMin(opts.requestMin);
	    envConfig.setReplicationRequestMax(opts.requestMax);

            try {
                dbenv = new Environment(homedir, envConfig);
            } catch(FileNotFoundException e) {
                TestUtils.DEBUGOUT(3, "Unexpected FNFE in standard environment creation." + e);
                gotexcept = true;
                retval = false; // never expect a FNFE
            } catch(DatabaseException dbe) {
                    gotexcept = true;
                if (opts.validPolicy)
                    TestUtils.DEBUGOUT(3, "Unexpected DB exception from Environment create." + dbe);
	    } catch(IllegalArgumentException iae){
	    	    gotexcept = true;
	       if (opts.validPolicy)
		    TestUtils.DEBUGOUT(3, "Unexpected DB exception from setRepRequest." + iae);
	    }
	             
            if (!gotexcept) {
                try {
                    // start replication manager
                    if (opts.validStartPolicy)
                        dbenv.replicationManagerStart(opts.nsitesToStart, ReplicationManagerStartPolicy.REP_MASTER);
                    else
                        dbenv.replicationManagerStart(opts.nsitesToStart, null);
                } catch(DatabaseException dbe) {
                        gotexcept = true;
                    if (opts.validPolicy)
                        TestUtils.DEBUGOUT(3, "Unexpected database exception came from replicationManagerStart." + dbe);
                } catch (IllegalArgumentException iae) {
                        gotexcept = true;
                    if (opts.validPolicy)
                        TestUtils.DEBUGOUT(3, "Unexpected IllegalArgumentException came from replicationManagerStart." + iae);
                } catch (NullPointerException npe) {
                        gotexcept = true;
                    if (opts.validPolicy)
                        TestUtils.DEBUGOUT(3, "Unexpected NullPointerException came from replicationManagerStart." + npe);
                } catch (AssertionError ae) {
                        gotexcept = true;
                    if (opts.validPolicy)
                        TestUtils.DEBUGOUT(3, "Unexpected AssertionError came from replicationManagerStart." + ae);
                }
                 
            }
        } catch (IllegalArgumentException iae) {
                gotexcept = true;
	    if (opts.validPolicy)
                TestUtils.DEBUGOUT(3, "Unexpected IllegalArgumentException." + iae);
        } catch (AssertionError ae) {
                gotexcept = true;
            if (opts.validPolicy)
                TestUtils.DEBUGOUT(3, "Unexpected AssertionError." + ae);
        } catch (NullPointerException npe) {
                gotexcept = true;
            if (opts.validPolicy)
                TestUtils.DEBUGOUT(3, "Unexpected NullPointerException." + npe);
        }
        if (dbenv != null) {
            try {
                java.lang.Thread.sleep(1000);
            }catch(InterruptedException ie) {}
            try {
                dbenv.close();
                Environment.remove(homedir, true, envConfig);
            } catch(FileNotFoundException fnfe) {
                gotexcept = true;
                retval = false;
            } catch(DatabaseException dbe) {
                TestUtils.DEBUGOUT(3, "Unexpected database exception came during shutdown." + dbe);
                gotexcept = true; // never expect a shutdown failure.
            }
        }
        if (retval) {
            if (gotexcept == opts.validPolicy)
                retval = false;
        }
        return retval;
    }
 
    /*
     * TODO: Maybe move this into a general TestEventHandler?
     */
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
