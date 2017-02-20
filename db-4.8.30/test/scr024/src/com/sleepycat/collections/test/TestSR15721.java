/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections.test;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.collections.CurrentTransaction;
import com.sleepycat.db.Environment;
import com.sleepycat.util.test.TestEnv;

/**
 * @author Chao Huang
 */
public class TestSR15721 extends TestCase {

    /**
     * Runs a command line collection test.
     * @see #usage
     */
    public static void main(String[] args) {
        if (args.length == 1 &&
            (args[0].equals("-h") || args[0].equals("-help"))) {
            usage();
        } else {
            junit.framework.TestResult tr =
                junit.textui.TestRunner.run(suite());
            if (tr.errorCount() > 0 ||
                tr.failureCount() > 0) {
                System.exit(1);
            } else {
                System.exit(0);
            }
        }
    }

    private static void usage() {

        System.out.println(
              "Usage: java com.sleepycat.collections.test.TestSR15721"
            + " [-h | -help]\n");
        System.exit(2);
    }

    public static Test suite() {
        TestSuite suite = new TestSuite(TestSR15721.class);
        return suite;
    }

    private Environment env;
    private CurrentTransaction currentTxn;

    @Override
    public void setUp()
        throws Exception {

        env = TestEnv.TXN.open("TestSR15721");
        currentTxn = CurrentTransaction.getInstance(env);
    }

    @Override
    public void tearDown() {
        try {
            if (env != null) {
                env.close();
            }
        } catch (Exception e) {
            System.out.println("Ignored exception during tearDown: " + e);
        } finally {
            /* Ensure that GC can cleanup. */
            env = null;
            currentTxn = null;
        }
    }

    /**
     * Tests that the CurrentTransaction instance doesn't indeed allow GC to
     * reclaim while attached environment is open. [#15721]
     */
    public void testSR15721Fix()
        throws Exception {

        int hash = currentTxn.hashCode();
        int hash2 = -1;

        currentTxn = CurrentTransaction.getInstance(env);
        hash2 = currentTxn.hashCode();
        assertTrue(hash == hash2);

        currentTxn.beginTransaction(null);
        currentTxn = null;
        hash2 = -1;

        for (int i = 0; i < 10; i += 1) {
            byte[] x = null;
            try {
                 x = new byte[Integer.MAX_VALUE - 1];
                 fail();
            } catch (OutOfMemoryError expected) {
            }
            assertNull(x);

            System.gc();
        }

        currentTxn = CurrentTransaction.getInstance(env);
        hash2 = currentTxn.hashCode();
        currentTxn.commitTransaction();

        assertTrue(hash == hash2);
    }
}
