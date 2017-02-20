/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections.test;

import java.util.Iterator;
import java.util.ListIterator;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.ByteArrayBinding;
import com.sleepycat.collections.StoredIterator;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.Environment;
import com.sleepycat.db.DeadlockException;
import com.sleepycat.util.test.TestEnv;

/**
 * Tests the fix for [#10516], where the StoredIterator constructor was not
 * closing the cursor when an exception occurred. For example, a deadlock
 * exception might occur if the constructor was unable to move the cursor to
 * the first element.
 * @author Mark Hayes
 */
public class IterDeadlockTest extends TestCase {

    private static final byte[] ONE = { 1 };

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
        TestSuite suite = new TestSuite(IterDeadlockTest.class);
        return suite;
    }

    private Environment env;
    private Database store1;
    private Database store2;
    private StoredSortedMap map1;
    private StoredSortedMap map2;
    private final ByteArrayBinding binding = new ByteArrayBinding();

    public IterDeadlockTest(String name) {

        super(name);
    }

    @Override
    public void setUp()
        throws Exception {

        env = TestEnv.TXN.open("IterDeadlockTest");
        store1 = openDb("store1.db");
        store2 = openDb("store2.db");
        map1 = new StoredSortedMap(store1, binding, binding, true);
        map2 = new StoredSortedMap(store2, binding, binding, true);
    }

    @Override
    public void tearDown() {

        if (store1 != null) {
            try {
                store1.close();
            } catch (Exception e) {
                System.out.println("Ignored exception during tearDown: " + e);
            }
        }
        if (store2 != null) {
            try {
                store2.close();
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
        store1 = null;
        store2 = null;
        map1 = null;
        map2 = null;
    }

    private Database openDb(String file)
        throws Exception {

        DatabaseConfig config = new DatabaseConfig();
        DbCompat.setTypeBtree(config);
        config.setTransactional(true);
        config.setAllowCreate(true);

        return DbCompat.testOpenDatabase(env, null, file, null, config);
    }

    public void testIterDeadlock()
        throws Exception {

        final Object parent = new Object();
        final Object child1 = new Object();
        final Object child2 = new Object();
        final TransactionRunner runner = new TransactionRunner(env);
        runner.setMaxRetries(0);

        /* Write a record in each db. */
        runner.run(new TransactionWorker() {
            public void doWork() {
                assertNull(map1.put(ONE, ONE));
                assertNull(map2.put(ONE, ONE));
            }
        });

        /*
         * A thread to open iterator 1, then wait to be notified, then open
         * iterator 2.
         */
        final Thread thread1 = new Thread(new Runnable() {
            public void run() {
                try {
                    runner.run(new TransactionWorker() {
                        public void doWork() throws Exception {
                            synchronized (child1) {
                                ListIterator i1 =
                                    (ListIterator) map1.values().iterator();
                                i1.next();
                                i1.set(ONE); /* Write lock. */
                                StoredIterator.close(i1);
                                synchronized (parent) { parent.notify(); }
                                child1.wait();
                                Iterator i2 = map2.values().iterator();
                                assertTrue(i2.hasNext());
                                StoredIterator.close(i2);
                            }
                        }
                    });
                } catch (DeadlockException expected) {
                } catch (Exception e) {
                    e.printStackTrace();
                    fail(e.toString());
                }
            }
        });

        /*
         * A thread to open iterator 2, then wait to be notified, then open
         * iterator 1.
         */
        final Thread thread2 = new Thread(new Runnable() {
            public void run() {
                try {
                    runner.run(new TransactionWorker() {
                        public void doWork() throws Exception {
                            synchronized (child2) {
                                ListIterator i2 =
                                    (ListIterator) map2.values().iterator();
                                i2.next();
                                i2.set(ONE); /* Write lock. */
                                StoredIterator.close(i2);
                                synchronized (parent) { parent.notify(); }
                                child2.wait();
                                Iterator i1 = map1.values().iterator();
                                assertTrue(i1.hasNext());
                                StoredIterator.close(i1);
                            }
                        }
                    });
                } catch (DeadlockException expected) {
                } catch (Exception e) {
                    e.printStackTrace();
                    fail(e.toString());
                }
            }
        });

        /*
         * Open iterator 1 in thread 1, then iterator 2 in thread 2, then let
         * the threads run to open the other iterators and cause a deadlock.
         */
        synchronized (parent) {
            thread1.start();
            parent.wait();
            thread2.start();
            parent.wait();
            synchronized (child1) { child1.notify(); }
            synchronized (child2) { child2.notify(); }
            thread1.join();
            thread2.join();
        }

        /*
         * Before the fix for [#10516] we would get an exception indicating
         * that cursors were not closed, when closing the stores below.
         */
        store1.close();
        store1 = null;
        store2.close();
        store2 = null;
        env.close();
        env = null;
    }
}
