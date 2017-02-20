/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.collections.test.serial;

import java.util.Map;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.serial.SerialBinding;
import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.collections.StoredMap;
import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.Environment;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestEnv;

/**
 * Runs part one of the StoredClassCatalogTest.  This part is run with the
 * old/original version of TestSerial in the classpath.  It creates a fresh
 * environment and databases containing serialized versions of the old class.
 * When StoredClassCatalogTest is run, it will read these objects from the
 * database created here.
 *
 * @author Mark Hayes
 */
public class StoredClassCatalogTestInit extends TestCase
    implements TransactionWorker {

    static final String CATALOG_FILE = StoredClassCatalogTest.CATALOG_FILE;
    static final String STORE_FILE = StoredClassCatalogTest.STORE_FILE;

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
        TestSuite suite = new TestSuite();
        for (int i = 0; i < TestEnv.ALL.length; i += 1) {
            suite.addTest(new StoredClassCatalogTestInit(TestEnv.ALL[i]));
        }
        return suite;
    }

    private TestEnv testEnv;
    private Environment env;
    private StoredClassCatalog catalog;
    private Database store;
    private Map map;
    private TransactionRunner runner;

    public StoredClassCatalogTestInit(TestEnv testEnv) {

        super("StoredClassCatalogTestInit-" + testEnv.getName());
        this.testEnv = testEnv;
    }

    @Override
    public void setUp()
        throws Exception {

        SharedTestUtils.printTestName(getName());
        env = testEnv.open(StoredClassCatalogTest.makeTestName(testEnv));
        runner = new TransactionRunner(env);

        catalog = new StoredClassCatalog(openDb(CATALOG_FILE));

        SerialBinding keyBinding = new SerialBinding(catalog, String.class);
        SerialBinding valueBinding =
	    new SerialBinding(catalog, TestSerial.class);
        store = openDb(STORE_FILE);

        map = new StoredMap(store, keyBinding, valueBinding, true);
    }

    private Database openDb(String file)
        throws Exception {

        DatabaseConfig config = new DatabaseConfig();
        DbCompat.setTypeBtree(config);
        config.setTransactional(testEnv.isTxnMode());
        config.setAllowCreate(true);

        return DbCompat.testOpenDatabase(env, null, file, null, config);
    }

    @Override
    public void tearDown() {

        try {
            if (catalog != null) {
                catalog.close();
                catalog.close(); // should have no effect
            }
            if (store != null) {
                store.close();
            }
            if (env != null) {
                env.close();
            }
        } catch (Exception e) {
            System.err.println("Ignored exception during tearDown: ");
            e.printStackTrace();
        } finally {
            /* Ensure that GC can cleanup. */
            catalog = null;
            store = null;
            env = null;
            testEnv = null;
            map = null;
            runner = null;
        }
    }

    @Override
    public void runTest()
        throws Exception {

        runner.run(this);
    }

    public void doWork() {
        TestSerial one = new TestSerial(null);
        TestSerial two = new TestSerial(one);
        assertNull("Likely the classpath contains the wrong version of the" +
                   " TestSerial class, the 'original' version is required",
                   one.getStringField());
        assertNull(two.getStringField());
        map.put("one", one);
        map.put("two", two);
        one = (TestSerial) map.get("one");
        two = (TestSerial) map.get("two");
        assertEquals(one, two.getOther());
        assertNull(one.getStringField());
        assertNull(two.getStringField());
    }
}
