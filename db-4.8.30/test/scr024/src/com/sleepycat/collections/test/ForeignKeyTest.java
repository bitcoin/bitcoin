/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections.test;

import java.util.Map;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.bind.serial.TupleSerialMarshalledKeyCreator;
import com.sleepycat.bind.serial.test.MarshalledObject;
import com.sleepycat.collections.CurrentTransaction;
import com.sleepycat.collections.TupleSerialFactory;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.ForeignKeyDeleteAction;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryDatabase;
import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.RuntimeExceptionWrapper;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestEnv;

/**
 * @author Mark Hayes
 */
public class ForeignKeyTest extends TestCase {

    private static final ForeignKeyDeleteAction[] ACTIONS = {
        ForeignKeyDeleteAction.ABORT,
        ForeignKeyDeleteAction.NULLIFY,
        ForeignKeyDeleteAction.CASCADE,
    };
    private static final String[] ACTION_LABELS = {
        "ABORT",
        "NULLIFY",
        "CASCADE",
    };

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
            for (int j = 0; j < ACTIONS.length; j += 1) {
                suite.addTest(new ForeignKeyTest(TestEnv.ALL[i],
                                                 ACTIONS[j],
                                                 ACTION_LABELS[j]));
            }
        }
        return suite;
    }

    private TestEnv testEnv;
    private Environment env;
    private StoredClassCatalog catalog;
    private TupleSerialFactory factory;
    private Database store1;
    private Database store2;
    private SecondaryDatabase index1;
    private SecondaryDatabase index2;
    private Map storeMap1;
    private Map storeMap2;
    private Map indexMap1;
    private Map indexMap2;
    private final ForeignKeyDeleteAction onDelete;

    public ForeignKeyTest(TestEnv testEnv, ForeignKeyDeleteAction onDelete,
                          String onDeleteLabel) {

        super("ForeignKeyTest-" + testEnv.getName() + '-' + onDeleteLabel);

        this.testEnv = testEnv;
        this.onDelete = onDelete;
    }

    @Override
    public void setUp()
        throws Exception {

        SharedTestUtils.printTestName(getName());
        env = testEnv.open(getName());

        createDatabase();
    }

    @Override
    public void tearDown() {

        try {
            if (index1 != null) {
                index1.close();
            }
            if (index2 != null) {
                index2.close();
            }
            if (store1 != null) {
                store1.close();
            }
            if (store2 != null) {
                store2.close();
            }
            if (catalog != null) {
                catalog.close();
            }
            if (env != null) {
                env.close();
            }
        } catch (Exception e) {
            System.out.println("Ignored exception during tearDown: " + e);
	} finally {
            /* Ensure that GC can cleanup. */
            env = null;
	    testEnv = null;
            catalog = null;
            store1 = null;
            store2 = null;
            index1 = null;
            index2 = null;
            factory = null;
            storeMap1 = null;
            storeMap2 = null;
            indexMap1 = null;
            indexMap2 = null;
        }
    }

    @Override
    public void runTest()
        throws Exception {

        try {
            createViews();
            writeAndRead();
        } catch (Exception e) {
            throw ExceptionUnwrapper.unwrap(e);
        }
    }

    private void createDatabase()
        throws Exception {

        catalog = new StoredClassCatalog(openDb("catalog.db"));
        factory = new TupleSerialFactory(catalog);
        assertSame(catalog, factory.getCatalog());

        store1 = openDb("store1.db");
        store2 = openDb("store2.db");
        index1 = openSecondaryDb(factory, "1", store1, "index1.db", null);
        index2 = openSecondaryDb(factory, "2", store2, "index2.db", store1);
    }

    private Database openDb(String file)
        throws Exception {

        DatabaseConfig config = new DatabaseConfig();
        DbCompat.setTypeBtree(config);
        config.setTransactional(testEnv.isTxnMode());
        config.setAllowCreate(true);

        return DbCompat.testOpenDatabase(env, null, file, null, config);
    }

    private SecondaryDatabase openSecondaryDb(TupleSerialFactory factory,
                                              String keyName,
                                              Database primary,
                                              String file,
                                              Database foreignStore)
        throws Exception {

        TupleSerialMarshalledKeyCreator keyCreator =
                factory.getKeyCreator(MarshalledObject.class, keyName);

        SecondaryConfig secConfig = new SecondaryConfig();
        DbCompat.setTypeBtree(secConfig);
        secConfig.setTransactional(testEnv.isTxnMode());
        secConfig.setAllowCreate(true);
        secConfig.setKeyCreator(keyCreator);
        if (foreignStore != null) {
            secConfig.setForeignKeyDatabase(foreignStore);
            secConfig.setForeignKeyDeleteAction(onDelete);
            if (onDelete == ForeignKeyDeleteAction.NULLIFY) {
                secConfig.setForeignKeyNullifier(keyCreator);
            }
        }

        return DbCompat.testOpenSecondaryDatabase
            (env, null, file, null, primary, secConfig);
    }

    private void createViews() {
        storeMap1 = factory.newMap(store1, String.class,
                                   MarshalledObject.class, true);
        storeMap2 = factory.newMap(store2, String.class,
                                   MarshalledObject.class, true);
        indexMap1 = factory.newMap(index1, String.class,
                                   MarshalledObject.class, true);
        indexMap2 = factory.newMap(index2, String.class,
                                   MarshalledObject.class, true);
    }

    private void writeAndRead()
        throws Exception {

        CurrentTransaction txn = CurrentTransaction.getInstance(env);
        if (txn != null) {
            txn.beginTransaction(null);
        }

        MarshalledObject o1 = new MarshalledObject("data1", "pk1", "ik1", "");
        assertNull(storeMap1.put(null, o1));

        assertEquals(o1, storeMap1.get("pk1"));
        assertEquals(o1, indexMap1.get("ik1"));

        MarshalledObject o2 = new MarshalledObject("data2", "pk2", "", "pk1");
        assertNull(storeMap2.put(null, o2));

        assertEquals(o2, storeMap2.get("pk2"));
        assertEquals(o2, indexMap2.get("pk1"));

        if (txn != null) {
            txn.commitTransaction();
            txn.beginTransaction(null);
        }

        /*
         * store1 contains o1 with primary key "pk1" and index key "ik1".
         *
         * store2 contains o2 with primary key "pk2" and foreign key "pk1",
         * which is the primary key of store1.
         */

        if (onDelete == ForeignKeyDeleteAction.ABORT) {

            /* Test that we abort trying to delete a referenced key. */

            try {
                storeMap1.remove("pk1");
                fail();
            } catch (RuntimeExceptionWrapper expected) {
                assertTrue(expected.getCause() instanceof DatabaseException);
                assertTrue(!DbCompat.NEW_JE_EXCEPTIONS);
            }
            if (txn != null) {
                txn.abortTransaction();
                txn.beginTransaction(null);
            }

            /* Test that we can put a record into store2 with a null foreign
             * key value. */

            o2 = new MarshalledObject("data2", "pk2", "", "");
            assertNotNull(storeMap2.put(null, o2));
            assertEquals(o2, storeMap2.get("pk2"));

            /* The index2 record should have been deleted since the key was set
             * to null above. */

            assertNull(indexMap2.get("pk1"));

            /* Test that now we can delete the record in store1, since it is no
             * longer referenced. */

            assertNotNull(storeMap1.remove("pk1"));
            assertNull(storeMap1.get("pk1"));
            assertNull(indexMap1.get("ik1"));

        } else if (onDelete == ForeignKeyDeleteAction.NULLIFY) {

            /* Delete the referenced key. */

            assertNotNull(storeMap1.remove("pk1"));
            assertNull(storeMap1.get("pk1"));
            assertNull(indexMap1.get("ik1"));

            /* The store2 record should still exist, but should have an empty
             * secondary key since it was nullified. */

            o2 = (MarshalledObject) storeMap2.get("pk2");
            assertNotNull(o2);
            assertEquals("data2", o2.getData());
            assertEquals("pk2", o2.getPrimaryKey());
            assertEquals("", o2.getIndexKey1());
            assertEquals("", o2.getIndexKey2());

        } else if (onDelete == ForeignKeyDeleteAction.CASCADE) {

            /* Delete the referenced key. */

            assertNotNull(storeMap1.remove("pk1"));
            assertNull(storeMap1.get("pk1"));
            assertNull(indexMap1.get("ik1"));

            /* The store2 record should have deleted also. */

            assertNull(storeMap2.get("pk2"));
            assertNull(indexMap2.get("pk1"));

        } else {
            throw new IllegalStateException();
        }

        /*
         * Test that a foreign key value may not be used that is not present
         * in the foreign store. "pk2" is not in store1 in this case.
         */
        assertNull(storeMap1.get("pk2"));
        MarshalledObject o3 = new MarshalledObject("data3", "pk3", "", "pk2");
        try {
            storeMap2.put(null, o3);
            fail();
        } catch (RuntimeExceptionWrapper expected) {
            assertTrue(expected.getCause() instanceof DatabaseException);
            assertTrue(!DbCompat.NEW_JE_EXCEPTIONS);
        }

        if (txn != null) {
            txn.abortTransaction();
        }
    }
}
