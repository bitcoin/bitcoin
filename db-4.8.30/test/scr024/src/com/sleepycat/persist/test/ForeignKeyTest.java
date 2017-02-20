/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.DeleteAction.ABORT;
import static com.sleepycat.persist.model.DeleteAction.CASCADE;
import static com.sleepycat.persist.model.DeleteAction.NULLIFY;
import static com.sleepycat.persist.model.Relationship.ONE_TO_ONE;

import java.util.Enumeration;

import junit.framework.Test;
import junit.framework.TestSuite;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.DeleteAction;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.util.test.TxnTestCase;

/**
 * @author Mark Hayes
 */
public class ForeignKeyTest extends TxnTestCase {

    private static final DeleteAction[] ACTIONS = {
        ABORT,
        NULLIFY,
        CASCADE,
    };

    private static final String[] ACTION_LABELS = {
        "ABORT",
        "NULLIFY",
        "CASCADE",
    };

    static protected Class<?> testClass = ForeignKeyTest.class;

    public static Test suite() {
        TestSuite suite = new TestSuite();
        for (int i = 0; i < ACTIONS.length; i += 1) {
	    for (int j = 0; j < 2; j++) {
		TestSuite txnSuite = txnTestSuite(testClass, null, null);
		Enumeration e = txnSuite.tests();
		while (e.hasMoreElements()) {
		    ForeignKeyTest test = (ForeignKeyTest) e.nextElement();
		    test.onDelete = ACTIONS[i];
		    test.onDeleteLabel = ACTION_LABELS[i];
		    test.useSubclass = (j == 0);
		    test.useSubclassLabel =
			(j == 0) ? "UseSubclass" : "UseBaseclass";
		    suite.addTest(test);
		}
	    }
        }
        return suite;
    }

    private EntityStore store;
    private PrimaryIndex<String,Entity1> pri1;
    private PrimaryIndex<String,Entity2> pri2;
    private SecondaryIndex<String,String,Entity1> sec1;
    private SecondaryIndex<String,String,Entity2> sec2;
    private DeleteAction onDelete;
    private String onDeleteLabel;
    private boolean useSubclass;
    private String useSubclassLabel;

    @Override
    public void tearDown()
        throws Exception {

        super.tearDown();
        setName(getName() + '-' + onDeleteLabel + "-" + useSubclassLabel);
    }

    private void open()
        throws DatabaseException {

        StoreConfig config = new StoreConfig();
        config.setAllowCreate(envConfig.getAllowCreate());
        config.setTransactional(envConfig.getTransactional());

        store = new EntityStore(env, "test", config);

        pri1 = store.getPrimaryIndex(String.class, Entity1.class);
        sec1 = store.getSecondaryIndex(pri1, String.class, "sk");
        pri2 = store.getPrimaryIndex(String.class, Entity2.class);
        sec2 = store.getSecondaryIndex
            (pri2, String.class, "sk_" + onDeleteLabel);
    }

    private void close()
        throws DatabaseException {

        store.close();
    }

    public void testForeignKeys()
        throws Exception {

        open();
        Transaction txn = txnBegin();

        Entity1 o1 = new Entity1("pk1", "sk1");
        assertNull(pri1.put(txn, o1));

        assertEquals(o1, pri1.get(txn, "pk1", null));
        assertEquals(o1, sec1.get(txn, "sk1", null));

        Entity2 o2 = (useSubclass ?
		      new Entity3("pk2", "pk1", onDelete) :
		      new Entity2("pk2", "pk1", onDelete));
        assertNull(pri2.put(txn, o2));

        assertEquals(o2, pri2.get(txn, "pk2", null));
        assertEquals(o2, sec2.get(txn, "pk1", null));

        txnCommit(txn);
        txn = txnBegin();

        /*
         * pri1 contains o1 with primary key "pk1" and index key "sk1".
         *
         * pri2 contains o2 with primary key "pk2" and foreign key "pk1",
         * which is the primary key of pri1.
         */
        if (onDelete == ABORT) {

            /* Test that we abort trying to delete a referenced key. */

            try {
                pri1.delete(txn, "pk1");
                fail();
            } catch (DatabaseException expected) {
                assertTrue(!DbCompat.NEW_JE_EXCEPTIONS);
                txnAbort(txn);
                txn = txnBegin();
            }

            /*
	     * Test that we can put a record into store2 with a null foreign
             * key value.
	     */
            o2 = (useSubclass ?
		  new Entity3("pk2", null, onDelete) :
		  new Entity2("pk2", null, onDelete));
            assertNotNull(pri2.put(txn, o2));
            assertEquals(o2, pri2.get(txn, "pk2", null));

            /*
	     * The index2 record should have been deleted since the key was set
             * to null above.
	     */
            assertNull(sec2.get(txn, "pk1", null));

            /*
	     * Test that now we can delete the record in store1, since it is no
             * longer referenced.
	     */
            assertNotNull(pri1.delete(txn, "pk1"));
            assertNull(pri1.get(txn, "pk1", null));
            assertNull(sec1.get(txn, "sk1", null));

        } else if (onDelete == NULLIFY) {

            /* Delete the referenced key. */
            assertNotNull(pri1.delete(txn, "pk1"));
            assertNull(pri1.get(txn, "pk1", null));
            assertNull(sec1.get(txn, "sk1", null));

            /*
	     * The store2 record should still exist, but should have an empty
             * secondary key since it was nullified.
	     */
            o2 = pri2.get(txn, "pk2", null);
            assertNotNull(o2);
            assertEquals("pk2", o2.pk);
            assertEquals(null, o2.getSk(onDelete));

        } else if (onDelete == CASCADE) {

            /* Delete the referenced key. */
            assertNotNull(pri1.delete(txn, "pk1"));
            assertNull(pri1.get(txn, "pk1", null));
            assertNull(sec1.get(txn, "sk1", null));

            /* The store2 record should have deleted also. */
            assertNull(pri2.get(txn, "pk2", null));
            assertNull(sec2.get(txn, "pk1", null));

        } else {
            throw new IllegalStateException();
        }

        /*
         * Test that a foreign key value may not be used that is not present in
         * the foreign store. "pk2" is not in store1 in this case.
         */
	Entity2 o3 = (useSubclass ?
		      new Entity3("pk3", "pk2", onDelete) :
		      new Entity2("pk3", "pk2", onDelete));
        try {
            pri2.put(txn, o3);
            fail();
        } catch (DatabaseException expected) {
            assertTrue(!DbCompat.NEW_JE_EXCEPTIONS);
        }

        txnAbort(txn);
        close();
    }

    @Entity
    static class Entity1 {

        @PrimaryKey
        String pk;

        @SecondaryKey(relate=ONE_TO_ONE)
        String sk;

        private Entity1() {}

        Entity1(String pk, String sk) {
            this.pk = pk;
            this.sk = sk;
        }

        @Override
        public boolean equals(Object other) {
            Entity1 o = (Entity1) other;
            return nullOrEqual(pk, o.pk) &&
                   nullOrEqual(sk, o.sk);
        }
    }

    @Entity
    static class Entity2 {

        @PrimaryKey
        String pk;

        @SecondaryKey(relate=ONE_TO_ONE, relatedEntity=Entity1.class,
                                         onRelatedEntityDelete=ABORT)
        String sk_ABORT;

        @SecondaryKey(relate=ONE_TO_ONE, relatedEntity=Entity1.class,
                                         onRelatedEntityDelete=CASCADE)
        String sk_CASCADE;

        @SecondaryKey(relate=ONE_TO_ONE, relatedEntity=Entity1.class,
                                         onRelatedEntityDelete=NULLIFY)
        String sk_NULLIFY;

        private Entity2() {}

        Entity2(String pk, String sk, DeleteAction action) {
            this.pk = pk;
            switch (action) {
            case ABORT:
                sk_ABORT = sk;
                break;
            case CASCADE:
                sk_CASCADE = sk;
                break;
            case NULLIFY:
                sk_NULLIFY = sk;
                break;
            default:
                throw new IllegalArgumentException();
            }
        }

        String getSk(DeleteAction action) {
            switch (action) {
            case ABORT:
                return sk_ABORT;
            case CASCADE:
                return sk_CASCADE;
            case NULLIFY:
                return sk_NULLIFY;
            default:
                throw new IllegalArgumentException();
            }
        }

        @Override
        public boolean equals(Object other) {
            Entity2 o = (Entity2) other;
            return nullOrEqual(pk, o.pk) &&
                   nullOrEqual(sk_ABORT, o.sk_ABORT) &&
                   nullOrEqual(sk_CASCADE, o.sk_CASCADE) &&
                   nullOrEqual(sk_NULLIFY, o.sk_NULLIFY);
        }
    }

    @Persistent
    static class Entity3 extends Entity2 {
	Entity3() {}

        Entity3(String pk, String sk, DeleteAction action) {
	    super(pk, sk, action);
	}
    }

    static boolean nullOrEqual(Object o1, Object o2) {
        if (o1 == null) {
            return o2 == null;
        } else {
            return o1.equals(o2);
        }
    }
}
