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

import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.db.Database;
import com.sleepycat.db.Environment;
import com.sleepycat.db.DeadlockException;
import com.sleepycat.db.TransactionConfig;
import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.test.TestEnv;

/**
 * Tests whether secondary access can cause a self-deadlock when reading via a
 * secondary because the collections API secondary implementation in DB 4.2
 * opens two cursors.   Part of the problem in [#10516] was because the
 * secondary get() was not done in a txn.  This problem should not occur in DB
 * 4.3 and JE -- an ordinary deadlock occurs instead and is detected.
 *
 * @author Mark Hayes
 */
public class SecondaryDeadlockTest extends TestCase {

    private static final Long N_ONE = new Long(1);
    private static final Long N_101 = new Long(101);
    private static final int N_ITERS = 20;
    private static final int MAX_RETRIES = 1000;

    public static void main(String[] args) {
        junit.framework.TestResult tr =
            junit.textui.TestRunner.run(suite());
        if (tr.errorCount() > 0 ||
            tr.failureCount() > 0) {
            System.exit(1);
        } else {
            System.exit(0);
        }
    }

    public static Test suite() {
        TestSuite suite = new TestSuite(SecondaryDeadlockTest.class);
        return suite;
    }

    private Environment env;
    private Database store;
    private Database index;
    private StoredSortedMap storeMap;
    private StoredSortedMap indexMap;
    private Exception exception;

    public SecondaryDeadlockTest(String name) {

        super(name);
    }

    @Override
    public void setUp()
        throws Exception {

        env = TestEnv.TXN.open("SecondaryDeadlockTest");
        store = TestStore.BTREE_UNIQ.open(env, "store.db");
        index = TestStore.BTREE_UNIQ.openIndex(store, "index.db");
        storeMap = new StoredSortedMap(store,
                                       TestStore.BTREE_UNIQ.getKeyBinding(),
                                       TestStore.BTREE_UNIQ.getValueBinding(),
                                       true);
        indexMap = new StoredSortedMap(index,
                                       TestStore.BTREE_UNIQ.getKeyBinding(),
                                       TestStore.BTREE_UNIQ.getValueBinding(),
                                       true);
    }

    @Override
    public void tearDown() {

        if (index != null) {
            try {
                index.close();
            } catch (Exception e) {
                System.out.println("Ignored exception during tearDown: " + e);
            }
        }
        if (store != null) {
            try {
                store.close();
            } catch (Exception e) {
                System.out.println("Ignored exception during tearDown: " + e);
            }
        }
        if (env != null) {
            try {
                env.close();
            } catch (Exception e) {
                System.out.println("Ignored exception during tearDown: " + e);
            }
        }
        /* Allow GC of DB objects in the test case. */
        env = null;
        store = null;
        index = null;
        storeMap = null;
        indexMap = null;
    }

    public void testSecondaryDeadlock()
        throws Exception {

        final TransactionRunner runner = new TransactionRunner(env);
        runner.setMaxRetries(MAX_RETRIES);

        /*
         * This test deadlocks a lot at degree 3 serialization.  In debugging
         * this I discovered it was not due to phantom prevention per se but
         * just to a change in timing.
         */
        TransactionConfig txnConfig = new TransactionConfig();
        runner.setTransactionConfig(txnConfig);

        /*
         * A thread to do put() and delete() via the primary, which will lock
         * the primary first then the secondary.  Uses transactions.
         */
        final Thread thread1 = new Thread(new Runnable() {
            public void run() {
                try {
                    /* The TransactionRunner performs retries. */
                    for (int i = 0; i < N_ITERS; i +=1 ) {
                        runner.run(new TransactionWorker() {
                            public void doWork() {
                                assertEquals(null, storeMap.put(N_ONE, N_101));
                            }
                        });
                        runner.run(new TransactionWorker() {
                            public void doWork() {
                                assertEquals(N_101, storeMap.remove(N_ONE));
                            }
                        });
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    exception = e;
                }
            }
        }, "ThreadOne");

        /*
         * A thread to get() via the secondary, which will lock the secondary
         * first then the primary.  Does not use a transaction.
         */
        final Thread thread2 = new Thread(new Runnable() {
            public void run() {
                try {
                    for (int i = 0; i < N_ITERS; i +=1 ) {
                        for (int j = 0; j < MAX_RETRIES; j += 1) {
                            try {
                                Object value = indexMap.get(N_ONE);
                                assertTrue(value == null ||
                                           N_101.equals(value));
                                break;
                            } catch (Exception e) {
                                e = ExceptionUnwrapper.unwrap(e);
                                if (e instanceof DeadlockException) {
                                    continue; /* Retry on deadlock. */
                                } else {
                                    throw e;
                                }
                            }
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    exception = e;
                }
            }
        }, "ThreadTwo");

        thread1.start();
        thread2.start();
        thread1.join();
        thread2.join();

        index.close();
        index = null;
        store.close();
        store = null;
        env.close();
        env = null;

        if (exception != null) {
            fail(exception.toString());
        }
    }
}
