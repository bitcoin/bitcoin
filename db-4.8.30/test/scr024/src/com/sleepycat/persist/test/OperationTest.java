/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.DeleteAction.CASCADE;
import static com.sleepycat.persist.model.DeleteAction.NULLIFY;
import static com.sleepycat.persist.model.Relationship.MANY_TO_MANY;
import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;
import static com.sleepycat.persist.model.Relationship.ONE_TO_MANY;
import static com.sleepycat.persist.model.Relationship.ONE_TO_ONE;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import junit.framework.Test;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.StatsConfig;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.EntityCursor;
import com.sleepycat.persist.EntityIndex;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.impl.Store;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.KeyField;
import com.sleepycat.persist.model.NotPersistent;
import com.sleepycat.persist.model.NotTransient;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.persist.raw.RawStore;
import com.sleepycat.util.test.TxnTestCase;

/**
 * Tests misc store and index operations that are not tested by IndexTest.
 *
 * @author Mark Hayes
 */
public class OperationTest extends TxnTestCase {

    private static final String STORE_NAME = "test";

    static protected Class<?> testClass = OperationTest.class;

    public static Test suite() {
        return txnTestSuite(testClass, null, null);
    }

    private EntityStore store;

    private void openReadOnly()
        throws DatabaseException {

        StoreConfig config = new StoreConfig();
        config.setReadOnly(true);
        open(config);
    }

    private void open()
        throws DatabaseException {

        open((Class) null);
    }

    private void open(Class clsToRegister)
        throws DatabaseException {

        StoreConfig config = new StoreConfig();
        config.setAllowCreate(envConfig.getAllowCreate());
        if (clsToRegister != null) {
            com.sleepycat.persist.model.EntityModel model =
                new com.sleepycat.persist.model.AnnotationModel();
            model.registerClass(clsToRegister);
            config.setModel(model);
        }
        open(config);
    }

    private void open(StoreConfig config)
        throws DatabaseException {

        config.setTransactional(envConfig.getTransactional());
        store = new EntityStore(env, STORE_NAME, config);
    }

    private void close()
        throws DatabaseException {

        store.close();
        store = null;
    }

    @Override
    public void setUp()
        throws Exception {

        super.setUp();
    }

    /**
     * The store must be closed before closing the environment.
     */
    @Override
    public void tearDown()
        throws Exception {

        try {
            if (store != null) {
                store.close();
            }
        } catch (Throwable e) {
            System.out.println("During tearDown: " + e);
        }
        store = null;
        super.tearDown();
    }

    public void testReadOnly()
        throws DatabaseException {

        open();
        PrimaryIndex<Integer,SharedSequenceEntity1> priIndex =
            store.getPrimaryIndex(Integer.class, SharedSequenceEntity1.class);
        Transaction txn = txnBegin();
        SharedSequenceEntity1 e = new SharedSequenceEntity1();
        priIndex.put(txn, e);
        assertEquals(1, e.key);
        txnCommit(txn);
        close();

        /*
         * Check that we can open the store read-only and read the records
         * written above.
         */
        openReadOnly();
        priIndex =
            store.getPrimaryIndex(Integer.class, SharedSequenceEntity1.class);
        e = priIndex.get(1);
        assertNotNull(e);
        close();
    }



    public void testUninitializedCursor()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,MyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, MyEntity.class);

        Transaction txn = txnBeginCursor();

        MyEntity e = new MyEntity();
        e.priKey = 1;
        e.secKey = 1;
        priIndex.put(txn, e);

        EntityCursor<MyEntity> entities = priIndex.entities(txn, null);
        try {
            entities.nextDup();
            fail();
        } catch (IllegalStateException expected) {}
        try {
            entities.prevDup();
            fail();
        } catch (IllegalStateException expected) {}
        try {
            entities.current();
            fail();
        } catch (IllegalStateException expected) {}
        try {
            entities.delete();
            fail();
        } catch (IllegalStateException expected) {}
        try {
            entities.update(e);
            fail();
        } catch (IllegalStateException expected) {}
        try {
            entities.count();
            fail();
        } catch (IllegalStateException expected) {}

        entities.close();
        txnCommit(txn);
        close();
    }

    public void testCursorCount()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,MyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, MyEntity.class);

        SecondaryIndex<Integer,Integer,MyEntity> secIndex =
            store.getSecondaryIndex(priIndex, Integer.class, "secKey");

        Transaction txn = txnBeginCursor();

        MyEntity e = new MyEntity();
        e.priKey = 1;
        e.secKey = 1;
        priIndex.put(txn, e);

        EntityCursor<MyEntity> cursor = secIndex.entities(txn, null);
        cursor.next();
        assertEquals(1, cursor.count());
        cursor.close();

        e.priKey = 2;
        priIndex.put(txn, e);
        cursor = secIndex.entities(txn, null);
        cursor.next();
        assertEquals(2, cursor.count());
        cursor.close();

        txnCommit(txn);
        close();
    }

    public void testCursorUpdate()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,MyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, MyEntity.class);

        SecondaryIndex<Integer,Integer,MyEntity> secIndex =
            store.getSecondaryIndex(priIndex, Integer.class, "secKey");

        Transaction txn = txnBeginCursor();

        Integer k;
        MyEntity e = new MyEntity();
        e.priKey = 1;
        e.secKey = 2;
        priIndex.put(txn, e);

        /* update() with primary entity cursor. */
        EntityCursor<MyEntity> entities = priIndex.entities(txn, null);
        e = entities.next();
        assertNotNull(e);
        assertEquals(1, e.priKey);
        assertEquals(Integer.valueOf(2), e.secKey);
        e.secKey = null;
        assertTrue(entities.update(e));
        e = entities.current();
        assertNotNull(e);
        assertEquals(1, e.priKey);
        assertEquals(null, e.secKey);
        e.secKey = 3;
        assertTrue(entities.update(e));
        e = entities.current();
        assertNotNull(e);
        assertEquals(1, e.priKey);
        assertEquals(Integer.valueOf(3), e.secKey);
        entities.close();

        /* update() with primary keys cursor. */
        EntityCursor<Integer> keys = priIndex.keys(txn, null);
        k = keys.next();
        assertNotNull(k);
        assertEquals(Integer.valueOf(1), k);
        try {
            keys.update(2);
            fail();
        } catch (UnsupportedOperationException expected) {
        }
        keys.close();

        /* update() with secondary entity cursor. */
        entities = secIndex.entities(txn, null);
        e = entities.next();
        assertNotNull(e);
        assertEquals(1, e.priKey);
        assertEquals(Integer.valueOf(3), e.secKey);
        try {
            entities.update(e);
            fail();
        } catch (UnsupportedOperationException expected) {
        } catch (IllegalArgumentException expectedForDbCore) {
        }
        entities.close();

        /* update() with secondary keys cursor. */
        keys = secIndex.keys(txn, null);
        k = keys.next();
        assertNotNull(k);
        assertEquals(Integer.valueOf(3), k);
        try {
            keys.update(k);
            fail();
        } catch (UnsupportedOperationException expected) {
        }
        keys.close();

        txnCommit(txn);
        close();
    }

    public void testCursorDelete()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,MyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, MyEntity.class);

        SecondaryIndex<Integer,Integer,MyEntity> secIndex =
            store.getSecondaryIndex(priIndex, Integer.class, "secKey");

        Transaction txn = txnBeginCursor();

        /* delete() with primary and secondary entities cursor. */

        for (EntityIndex index : new EntityIndex[] { priIndex, secIndex }) {

            MyEntity e = new MyEntity();
            e.priKey = 1;
            e.secKey = 1;
            priIndex.put(txn, e);
            e.priKey = 2;
            priIndex.put(txn, e);

            EntityCursor<MyEntity> cursor = index.entities(txn, null);

            e = cursor.next();
            assertNotNull(e);
            assertEquals(1, e.priKey);
            e = cursor.current();
            assertNotNull(e);
            assertEquals(1, e.priKey);
            assertTrue(cursor.delete());
            assertTrue(!cursor.delete());
            assertNull(cursor.current());

            e = cursor.next();
            assertNotNull(e);
            assertEquals(2, e.priKey);
            e = cursor.current();
            assertNotNull(e);
            assertEquals(2, e.priKey);
            assertTrue(cursor.delete());
            assertTrue(!cursor.delete());
            assertNull(cursor.current());

            e = cursor.next();
            assertNull(e);

            if (index == priIndex) {
                e = new MyEntity();
                e.priKey = 2;
                e.secKey = 1;
                assertTrue(!cursor.update(e));
            }

            cursor.close();
        }

        /* delete() with primary and secondary keys cursor. */

        for (EntityIndex index : new EntityIndex[] { priIndex, secIndex }) {

            MyEntity e = new MyEntity();
            e.priKey = 1;
            e.secKey = 1;
            priIndex.put(txn, e);
            e.priKey = 2;
            priIndex.put(txn, e);

            EntityCursor<Integer> cursor = index.keys(txn, null);

            Integer k = cursor.next();
            assertNotNull(k);
            assertEquals(1, k.intValue());
            k = cursor.current();
            assertNotNull(k);
            assertEquals(1, k.intValue());
            assertTrue(cursor.delete());
            assertTrue(!cursor.delete());
            assertNull(cursor.current());

            int expectKey = (index == priIndex) ? 2 : 1;
            k = cursor.next();
            assertNotNull(k);
            assertEquals(expectKey, k.intValue());
            k = cursor.current();
            assertNotNull(k);
            assertEquals(expectKey, k.intValue());
            assertTrue(cursor.delete());
            assertTrue(!cursor.delete());
            assertNull(cursor.current());

            k = cursor.next();
            assertNull(k);

            cursor.close();
        }

        txnCommit(txn);
        close();
    }

    public void testDeleteFromSubIndex()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,MyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, MyEntity.class);

        SecondaryIndex<Integer,Integer,MyEntity> secIndex =
            store.getSecondaryIndex(priIndex, Integer.class, "secKey");

        Transaction txn = txnBegin();
        MyEntity e = new MyEntity();
        e.secKey = 1;
        e.priKey = 1;
        priIndex.put(txn, e);
        e.priKey = 2;
        priIndex.put(txn, e);
        e.priKey = 3;
        priIndex.put(txn, e);
        e.priKey = 4;
        priIndex.put(txn, e);
        txnCommit(txn);

        EntityIndex<Integer,MyEntity> subIndex = secIndex.subIndex(1);
        txn = txnBeginCursor();
        e = subIndex.get(txn, 1, null);
        assertEquals(1, e.priKey);
        assertEquals(Integer.valueOf(1), e.secKey);
        e = subIndex.get(txn, 2, null);
        assertEquals(2, e.priKey);
        assertEquals(Integer.valueOf(1), e.secKey);
        e = subIndex.get(txn, 3, null);
        assertEquals(3, e.priKey);
        assertEquals(Integer.valueOf(1), e.secKey);
        e = subIndex.get(txn, 5, null);
        assertNull(e);

        boolean deleted = subIndex.delete(txn, 1);
        assertTrue(deleted);
        assertNull(subIndex.get(txn, 1, null));
        assertNotNull(subIndex.get(txn, 2, null));

        EntityCursor<MyEntity> cursor = subIndex.entities(txn, null);
        boolean saw4 = false;
        for (MyEntity e2 = cursor.first(); e2 != null; e2 = cursor.next()) {
            if (e2.priKey == 3) {
                cursor.delete();
            }
            if (e2.priKey == 4) {
                saw4 = true;
            }
        }
        cursor.close();
        assertTrue(saw4);
        assertNull(subIndex.get(txn, 1, null));
        assertNull(subIndex.get(txn, 3, null));
        assertNotNull(subIndex.get(txn, 2, null));
        assertNotNull(subIndex.get(txn, 4, null));

        txnCommit(txn);
        close();
    }

    @Entity
    static class MyEntity {

        @PrimaryKey
        private int priKey;

        @SecondaryKey(relate=MANY_TO_ONE)
        private Integer secKey;

        private MyEntity() {}
    }

    public void testSharedSequence()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,SharedSequenceEntity1> priIndex1 =
            store.getPrimaryIndex(Integer.class, SharedSequenceEntity1.class);

        PrimaryIndex<Integer,SharedSequenceEntity2> priIndex2 =
            store.getPrimaryIndex(Integer.class, SharedSequenceEntity2.class);

        Transaction txn = txnBegin();
        SharedSequenceEntity1 e1 = new SharedSequenceEntity1();
        SharedSequenceEntity2 e2 = new SharedSequenceEntity2();
        priIndex1.put(txn, e1);
        assertEquals(1, e1.key);
        priIndex2.putNoOverwrite(txn, e2);
        assertEquals(Integer.valueOf(2), e2.key);
        e1.key = 0;
        priIndex1.putNoOverwrite(txn, e1);
        assertEquals(3, e1.key);
        e2.key = null;
        priIndex2.put(txn, e2);
        assertEquals(Integer.valueOf(4), e2.key);
        txnCommit(txn);

        close();
    }

    @Entity
    static class SharedSequenceEntity1 {

        @PrimaryKey(sequence="shared")
        private int key;
    }

    @Entity
    static class SharedSequenceEntity2 {

        @PrimaryKey(sequence="shared")
        private Integer key;
    }

    public void testSeparateSequence()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,SeparateSequenceEntity1> priIndex1 =
            store.getPrimaryIndex
                (Integer.class, SeparateSequenceEntity1.class);

        PrimaryIndex<Integer,SeparateSequenceEntity2> priIndex2 =
            store.getPrimaryIndex
                (Integer.class, SeparateSequenceEntity2.class);

        Transaction txn = txnBegin();
        SeparateSequenceEntity1 e1 = new SeparateSequenceEntity1();
        SeparateSequenceEntity2 e2 = new SeparateSequenceEntity2();
        priIndex1.put(txn, e1);
        assertEquals(1, e1.key);
        priIndex2.putNoOverwrite(txn, e2);
        assertEquals(Integer.valueOf(1), e2.key);
        e1.key = 0;
        priIndex1.putNoOverwrite(txn, e1);
        assertEquals(2, e1.key);
        e2.key = null;
        priIndex2.put(txn, e2);
        assertEquals(Integer.valueOf(2), e2.key);
        txnCommit(txn);

        close();
    }

    @Entity
    static class SeparateSequenceEntity1 {

        @PrimaryKey(sequence="seq1")
        private int key;
    }

    @Entity
    static class SeparateSequenceEntity2 {

        @PrimaryKey(sequence="seq2")
        private Integer key;
    }

    public void testCompositeSequence()
        throws DatabaseException {

        open();

        PrimaryIndex<CompositeSequenceEntity1.Key,CompositeSequenceEntity1>
            priIndex1 =
            store.getPrimaryIndex
                (CompositeSequenceEntity1.Key.class,
                 CompositeSequenceEntity1.class);

        PrimaryIndex<CompositeSequenceEntity2.Key,CompositeSequenceEntity2>
            priIndex2 =
            store.getPrimaryIndex
                (CompositeSequenceEntity2.Key.class,
                 CompositeSequenceEntity2.class);

        Transaction txn = txnBegin();
        CompositeSequenceEntity1 e1 = new CompositeSequenceEntity1();
        CompositeSequenceEntity2 e2 = new CompositeSequenceEntity2();
        priIndex1.put(txn, e1);
        assertEquals(1, e1.key.key);
        priIndex2.putNoOverwrite(txn, e2);
        assertEquals(Integer.valueOf(1), e2.key.key);
        e1.key = null;
        priIndex1.putNoOverwrite(txn, e1);
        assertEquals(2, e1.key.key);
        e2.key = null;
        priIndex2.put(txn, e2);
        assertEquals(Integer.valueOf(2), e2.key.key);
        txnCommit(txn);

        EntityCursor<CompositeSequenceEntity1> c1 = priIndex1.entities();
        e1 = c1.next();
        assertEquals(2, e1.key.key);
        e1 = c1.next();
        assertEquals(1, e1.key.key);
        e1 = c1.next();
        assertNull(e1);
        c1.close();

        EntityCursor<CompositeSequenceEntity2> c2 = priIndex2.entities();
        e2 = c2.next();
        assertEquals(Integer.valueOf(2), e2.key.key);
        e2 = c2.next();
        assertEquals(Integer.valueOf(1), e2.key.key);
        e2 = c2.next();
        assertNull(e2);
        c2.close();

        close();
    }

    @Entity
    static class CompositeSequenceEntity1 {

        @Persistent
        static class Key implements Comparable<Key> {

            @KeyField(1)
            private int key;

            public int compareTo(Key o) {
                /* Reverse the natural order. */
                return o.key - key;
            }
        }

        @PrimaryKey(sequence="seq1")
        private Key key;
    }

    /**
     * Same as CompositeSequenceEntity1 but using Integer rather than int for
     * the key type.
     */
    @Entity
    static class CompositeSequenceEntity2 {

        @Persistent
        static class Key implements Comparable<Key> {

            @KeyField(1)
            private Integer key;

            public int compareTo(Key o) {
                /* Reverse the natural order. */
                return o.key - key;
            }
        }

        @PrimaryKey(sequence="seq2")
        private Key key;
    }

    /**
     * When opening read-only, secondaries are not opened when the primary is
     * opened, causing a different code path to be used for opening
     * secondaries.  For a RawStore in particular, this caused an unreported
     * NullPointerException in JE 3.0.12.  No SR was created because the use
     * case is very obscure and was discovered by code inspection.
     */
    public void testOpenRawStoreReadOnly()
        throws DatabaseException {

        open();
        store.getPrimaryIndex(Integer.class, MyEntity.class);
        close();

        StoreConfig config = new StoreConfig();
        config.setReadOnly(true);
        config.setTransactional(envConfig.getTransactional());
        RawStore rawStore = new RawStore(env, "test", config);

        String clsName = MyEntity.class.getName();
        rawStore.getSecondaryIndex(clsName, "secKey");

        rawStore.close();
    }

    /**
     * When opening an X_TO_MANY secondary that has a persistent key class, the
     * key class was not recognized as being persistent if it was never before
     * referenced when getSecondaryIndex was called.  This was a bug in JE
     * 3.0.12, reported on OTN.  [#15103]
     */
    public void testToManyKeyClass()
        throws DatabaseException {

        open();

        PrimaryIndex<Integer,ToManyKeyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, ToManyKeyEntity.class);
        SecondaryIndex<ToManyKey,Integer,ToManyKeyEntity> secIndex =
            store.getSecondaryIndex(priIndex, ToManyKey.class, "key2");

        priIndex.put(new ToManyKeyEntity());
        secIndex.get(new ToManyKey());

        close();
    }

    /**
     * Test a fix for a bug where opening a TO_MANY secondary index would fail
     * fail with "IllegalArgumentException: Wrong secondary key class: ..."
     * when the store was opened read-only.  [#15156]
     */
    public void testToManyReadOnly()
        throws DatabaseException {

        open();
        PrimaryIndex<Integer,ToManyKeyEntity> priIndex =
            store.getPrimaryIndex(Integer.class, ToManyKeyEntity.class);
        priIndex.put(new ToManyKeyEntity());
        close();

        openReadOnly();
        priIndex = store.getPrimaryIndex(Integer.class, ToManyKeyEntity.class);
        SecondaryIndex<ToManyKey,Integer,ToManyKeyEntity> secIndex =
            store.getSecondaryIndex(priIndex, ToManyKey.class, "key2");
        secIndex.get(new ToManyKey());
        close();
    }

    @Persistent
    static class ToManyKey {

        @KeyField(1)
        int value = 99;
    }

    @Entity
    static class ToManyKeyEntity {

        @PrimaryKey
        int key = 88;

        @SecondaryKey(relate=ONE_TO_MANY)
        Set<ToManyKey> key2;

        ToManyKeyEntity() {
            key2 = new HashSet<ToManyKey>();
            key2.add(new ToManyKey());
        }
    }



    /**
     * When Y is opened and X has a key with relatedEntity=Y.class, X should
     * be opened automatically.  If X is not opened, foreign key constraints
     * will not be enforced. [#15358]
     */
    public void testAutoOpenRelatedEntity()
        throws DatabaseException {

        PrimaryIndex<Integer,RelatedY> priY;
        PrimaryIndex<Integer,RelatedX> priX;

        /* Opening X should create (and open) Y and enforce constraints. */
        open();
        priX = store.getPrimaryIndex(Integer.class, RelatedX.class);
        PersistTestUtils.assertDbExists
            (true, env, STORE_NAME, RelatedY.class.getName(), null);
        if (isTransactional) {
            /* Constraint enforcement requires transactions. */
            try {
                priX.put(new RelatedX());
                fail();
            } catch (DatabaseException e) {
                assertTrue
                    ("" + e.getMessage(), (e.getMessage().indexOf
                      ("foreign key not allowed: it is not present") >= 0) ||
                     (e.getMessage().indexOf("DB_FOREIGN_CONFLICT") >= 0));
            }
        }
        priY = store.getPrimaryIndex(Integer.class, RelatedY.class);
        priY.put(new RelatedY());
        priX.put(new RelatedX());
        close();

        /* Delete should cascade even when X is not opened explicitly. */
        open();
        priY = store.getPrimaryIndex(Integer.class, RelatedY.class);
        assertEquals(1, priY.count());
        priY.delete(88);
        assertEquals(0, priY.count());
        priX = store.getPrimaryIndex(Integer.class, RelatedX.class);
        assertEquals(0, priX.count()); /* Failed prior to [#15358] fix. */
        close();
    }

    @Entity
    static class RelatedX {

        @PrimaryKey
        int key = 99;

        @SecondaryKey(relate=ONE_TO_ONE,
                      relatedEntity=RelatedY.class,
                      onRelatedEntityDelete=CASCADE)
        int key2 = 88;

        RelatedX() {
        }
    }

    @Entity
    static class RelatedY {

        @PrimaryKey
        int key = 88;

        RelatedY() {
        }
    }

    public void testSecondaryBulkLoad1()
        throws DatabaseException {

        doSecondaryBulkLoad(true);
    }

    public void testSecondaryBulkLoad2()
        throws DatabaseException {

        doSecondaryBulkLoad(false);
    }

    private void doSecondaryBulkLoad(boolean closeAndOpenNormally)
        throws DatabaseException {

        PrimaryIndex<Integer,RelatedX> priX;
        PrimaryIndex<Integer,RelatedY> priY;
        SecondaryIndex<Integer,Integer,RelatedX> secX;

        /* Open priX with SecondaryBulkLoad=true. */
        StoreConfig config = new StoreConfig();
        config.setAllowCreate(true);
        config.setSecondaryBulkLoad(true);
        open(config);

        /* Getting priX should not create the secondary index. */
        priX = store.getPrimaryIndex(Integer.class, RelatedX.class);
        PersistTestUtils.assertDbExists
            (false, env, STORE_NAME, RelatedX.class.getName(), "key2");

        /* We can put records that violate the secondary key constraint. */
        priX.put(new RelatedX());

        if (closeAndOpenNormally) {
            /* Open normally and attempt to populate the secondary. */
            close();
            open();
            if (isTransactional && DbCompat.POPULATE_ENFORCES_CONSTRAINTS) {
                /* Constraint enforcement requires transactions. */
                try {
                    /* Before adding the foreign key, constraint is violated. */
                    priX = store.getPrimaryIndex(Integer.class,
                                                 RelatedX.class);
                    fail();
                } catch (DatabaseException e) {
                    assertTrue
                        (e.toString(),
                         e.toString().contains("foreign key not allowed"));
                }
            }
            /* Open priX with SecondaryBulkLoad=true. */
            close();
            open(config);
            /* Add the foreign key to avoid the constraint error. */
            priY = store.getPrimaryIndex(Integer.class, RelatedY.class);
            priY.put(new RelatedY());
            /* Open normally and the secondary will be populated. */
            close();
            open();
            priX = store.getPrimaryIndex(Integer.class, RelatedX.class);
            PersistTestUtils.assertDbExists
                (true, env, STORE_NAME, RelatedX.class.getName(), "key2");
            secX = store.getSecondaryIndex(priX, Integer.class, "key2");
        } else {
            /* Get secondary index explicitly and it will be populated. */
            if (isTransactional && DbCompat.POPULATE_ENFORCES_CONSTRAINTS) {
                /* Constraint enforcement requires transactions. */
                try {
                    /* Before adding the foreign key, constraint is violated. */
                    secX = store.getSecondaryIndex(priX, Integer.class,
                                                   "key2");
                    fail();
                } catch (DatabaseException e) {
                    assertTrue
                        (e.toString(),
                         e.toString().contains("foreign key not allowed"));
                }
            }
            /* Add the foreign key. */
            priY = store.getPrimaryIndex(Integer.class, RelatedY.class);
            priY.put(new RelatedY());
            secX = store.getSecondaryIndex(priX, Integer.class, "key2");
            PersistTestUtils.assertDbExists
                (true, env, STORE_NAME, RelatedX.class.getName(), "key2");
        }

        RelatedX x = secX.get(88);
        assertNotNull(x);
        close();
    }

    public void testPersistentFields()
        throws DatabaseException {

        open();
        PrimaryIndex<Integer, PersistentFields> pri =
            store.getPrimaryIndex(Integer.class, PersistentFields.class);
        PersistentFields o1 = new PersistentFields(-1, 1, 2, 3, 4, 5, 6);
        assertNull(pri.put(o1));
        PersistentFields o2 = pri.get(-1);
        assertNotNull(o2);
        assertEquals(0, o2.transient1);
        assertEquals(0, o2.transient2);
        assertEquals(0, o2.transient3);
        assertEquals(4, o2.persistent1);
        assertEquals(5, o2.persistent2);
        assertEquals(6, o2.persistent3);
        close();
    }

    @Entity
    static class PersistentFields {

        @PrimaryKey int key;

        transient int transient1;
        @NotPersistent int transient2;
        @NotPersistent transient int transient3;

        int persistent1;
        @NotTransient int persistent2;
        @NotTransient transient int persistent3;

        PersistentFields(int k,
                         int t1,
                         int t2,
                         int t3,
                         int p1,
                         int p2,
                         int p3) {
            key = k;
            transient1 = t1;
            transient2 = t2;
            transient3 = t3;
            persistent1 = p1;
            persistent2 = p2;
            persistent3 = p3;
        }

        private PersistentFields() {}
    }

    /**
     * When a primary or secondary has a persistent key class, the key class
     * was not recognized as being persistent when getPrimaryConfig,
     * getSecondaryConfig, or getSubclassIndex was called, if that key class
     * was not previously referenced.  All three cases are tested by calling
     * getSecondaryConfig.  This was a bug in JE 3.3.69, reported on OTN.
     * [#16407]
     */
    public void testKeyClassInitialization()
        throws DatabaseException {

        open();
        store.getSecondaryConfig(ToManyKeyEntity.class, "key2");
        close();
    }

    public void testKeyName()
        throws DatabaseException {

        open();

        PrimaryIndex<Long, BookEntity> pri1 =
            store.getPrimaryIndex(Long.class, BookEntity.class);
        PrimaryIndex<Long, AuthorEntity> pri2 =
            store.getPrimaryIndex(Long.class, AuthorEntity.class);

        BookEntity book = new BookEntity();
        pri1.put(book);
        AuthorEntity author = new AuthorEntity();
        author.bookIds.add(book.bookId);
        pri2.put(author);

        close();

        open();
        pri1 = store.getPrimaryIndex(Long.class, BookEntity.class);
        pri2 = store.getPrimaryIndex(Long.class, AuthorEntity.class);
        book = pri1.get(1L);
        assertNotNull(book);
        author = pri2.get(1L);
        assertNotNull(author);
        close();
    }

    @Entity
    static class AuthorEntity {

        @PrimaryKey(sequence="authorSeq")
        long authorId;

        @SecondaryKey(relate=MANY_TO_MANY, relatedEntity=BookEntity.class,
                      name="bookId", onRelatedEntityDelete=NULLIFY)
        Set<Long> bookIds = new HashSet<Long>();
    }

    @Entity
    static class BookEntity {

        @PrimaryKey(sequence="bookSeq")
        long bookId;
    }

    /**
     * Checks that we get an appropriate exception when storing an entity
     * subclass instance, which contains a secondary key, without registering
     * the subclass up front. [#16399]
     */
    public void testPutEntitySubclassWithoutRegisterClass()
        throws DatabaseException {

        open();

        final PrimaryIndex<Long, Statement> pri =
            store.getPrimaryIndex(Long.class, Statement.class);

        final Transaction txn = txnBegin();
        pri.put(txn, new Statement(1));
        try {
            pri.put(txn, new ExtendedStatement(2, null));
            fail();
        } catch (IllegalArgumentException expected) {
            assertTrue(expected.toString(), expected.getMessage().contains
                ("Entity subclasses defining a secondary key must be " +
                 "registered by calling EntityModel.registerClass or " +
                 "EntityStore.getSubclassIndex before storing an instance " +
                 "of the subclass: " + ExtendedStatement.class.getName()));
        }
        txnAbort(txn);

        close();
    }

    /**
     * Checks that registerClass avoids an exception when storing an entity
     * subclass instance, which defines a secondary key. [#16399]
     */
    public void testPutEntitySubclassWithRegisterClass()
        throws DatabaseException {

        open(ExtendedStatement.class);

        final PrimaryIndex<Long, Statement> pri =
            store.getPrimaryIndex(Long.class, Statement.class);

        final Transaction txn = txnBegin();
        pri.put(txn, new Statement(1));
        pri.put(txn, new ExtendedStatement(2, "abc"));
        txnCommit(txn);

        final SecondaryIndex<String, Long, ExtendedStatement> sec =
            store.getSubclassIndex(pri, ExtendedStatement.class,
                                   String.class, "name");

        ExtendedStatement o = sec.get("abc");
        assertNotNull(o);
        assertEquals(2, o.id);

        close();
    }

    /**
     * Same as testPutEntitySubclassWithRegisterClass but store the first
     * instance of the subclass after closing and reopening the store,
     * *without* calling registerClass.  This ensures that a single call to
     * registerClass is sufficient and subsequent use of the store does not
     * require it.  [#16399]
     */
    public void testPutEntitySubclassWithRegisterClass2()
        throws DatabaseException {

        open(ExtendedStatement.class);

        PrimaryIndex<Long, Statement> pri =
            store.getPrimaryIndex(Long.class, Statement.class);

        Transaction txn = txnBegin();
        pri.put(txn, new Statement(1));
        txnCommit(txn);

        close();
        open();

        pri = store.getPrimaryIndex(Long.class, Statement.class);

        txn = txnBegin();
        pri.put(txn, new ExtendedStatement(2, "abc"));
        txnCommit(txn);

        final SecondaryIndex<String, Long, ExtendedStatement> sec =
            store.getSubclassIndex(pri, ExtendedStatement.class,
                                   String.class, "name");

        ExtendedStatement o = sec.get("abc");
        assertNotNull(o);
        assertEquals(2, o.id);

        close();
    }

    /**
     * Checks that getSubclassIndex can be used instead of registerClass to
     * avoid an exception when storing an entity subclass instance, which
     * defines a secondary key. [#16399]
     */
    public void testPutEntitySubclassWithGetSubclassIndex()
        throws DatabaseException {

        open();

        final PrimaryIndex<Long, Statement> pri =
            store.getPrimaryIndex(Long.class, Statement.class);

        final SecondaryIndex<String, Long, ExtendedStatement> sec =
            store.getSubclassIndex(pri, ExtendedStatement.class,
                                   String.class, "name");

        final Transaction txn = txnBegin();
        pri.put(txn, new Statement(1));
        pri.put(txn, new ExtendedStatement(2, "abc"));
        txnCommit(txn);

        ExtendedStatement o = sec.get("abc");
        assertNotNull(o);
        assertEquals(2, o.id);

        close();
    }

    /**
     * Same as testPutEntitySubclassWithGetSubclassIndex2 but store the first
     * instance of the subclass after closing and reopening the store,
     * *without* calling getSubclassIndex.  This ensures that a single call to
     * getSubclassIndex is sufficient and subsequent use of the store does not
     * require it.  [#16399]
     */
    public void testPutEntitySubclassWithGetSubclassIndex2()
        throws DatabaseException {

        open();

        PrimaryIndex<Long, Statement> pri =
            store.getPrimaryIndex(Long.class, Statement.class);

        SecondaryIndex<String, Long, ExtendedStatement> sec =
            store.getSubclassIndex(pri, ExtendedStatement.class,
                                   String.class, "name");

        Transaction txn = txnBegin();
        pri.put(txn, new Statement(1));
        txnCommit(txn);

        close();
        open();

        pri = store.getPrimaryIndex(Long.class, Statement.class);

        txn = txnBegin();
        pri.put(txn, new ExtendedStatement(2, "abc"));
        txnCommit(txn);

        sec = store.getSubclassIndex(pri, ExtendedStatement.class,
                                     String.class, "name");

        ExtendedStatement o = sec.get("abc");
        assertNotNull(o);
        assertEquals(2, o.id);

        close();
    }

    /**
     * Checks that secondary population occurs only once when an index is
     * created, not every time it is opened, even when it is empty.  This is a
     * JE-only test because we don't have a portable way to get stats that
     * indicate whether primary reads were performed.  [#16399]
     */

    @Entity
    static class Statement {
        
        @PrimaryKey
        long id;

        Statement(long id) {
            this.id = id;
        }

        private Statement() {}
    }

    @Persistent
    static class ExtendedStatement extends Statement {
        
        @SecondaryKey(relate=MANY_TO_ONE)
        String name;

        ExtendedStatement(long id, String name) {
            super(id);
            this.name = name;
        }

        private ExtendedStatement() {}
    } 

    public void testCustomCompare()
        throws DatabaseException {

        open();

        PrimaryIndex<ReverseIntKey, CustomCompareEntity>
            priIndex = store.getPrimaryIndex
                (ReverseIntKey.class, CustomCompareEntity.class);

        SecondaryIndex<ReverseIntKey, ReverseIntKey, CustomCompareEntity>
            secIndex1 = store.getSecondaryIndex(priIndex, ReverseIntKey.class,
                                                "secKey1");

        SecondaryIndex<ReverseIntKey, ReverseIntKey, CustomCompareEntity>
            secIndex2 = store.getSecondaryIndex(priIndex, ReverseIntKey.class,
                                                "secKey2");

        Transaction txn = txnBegin();
        for (int i = 1; i <= 5; i += 1) {
            assertTrue(priIndex.putNoOverwrite(txn,
                                               new CustomCompareEntity(i)));
        }
        txnCommit(txn);

        txn = txnBeginCursor();
        EntityCursor<CustomCompareEntity> c = priIndex.entities(txn, null);
        for (int i = 5; i >= 1; i -= 1) {
            CustomCompareEntity e = c.next();
            assertNotNull(e);
            assertEquals(new ReverseIntKey(i), e.key);
        }
        c.close();
        txnCommit(txn);

        txn = txnBeginCursor();
        c = secIndex1.entities(txn, null);
        for (int i = -1; i >= -5; i -= 1) {
            CustomCompareEntity e = c.next();
            assertNotNull(e);
            assertEquals(new ReverseIntKey(-i), e.key);
            assertEquals(new ReverseIntKey(i), e.secKey1);
        }
        c.close();
        txnCommit(txn);

        txn = txnBeginCursor();
        c = secIndex2.entities(txn, null);
        for (int i = -1; i >= -5; i -= 1) {
            CustomCompareEntity e = c.next();
            assertNotNull(e);
            assertEquals(new ReverseIntKey(-i), e.key);
            assertTrue(e.secKey2.contains(new ReverseIntKey(i)));
        }
        c.close();
        txnCommit(txn);

        close();
    }

    @Entity
    static class CustomCompareEntity {

        @PrimaryKey
        private ReverseIntKey key;

        @SecondaryKey(relate=MANY_TO_ONE)
        private ReverseIntKey secKey1;

        @SecondaryKey(relate=ONE_TO_MANY)
        private Set<ReverseIntKey> secKey2 = new HashSet<ReverseIntKey>();

        private CustomCompareEntity() {}

        CustomCompareEntity(int i) {
            key = new ReverseIntKey(i);
            secKey1 = new ReverseIntKey(-i);
            secKey2.add(new ReverseIntKey(-i));
        }
    }

    @Persistent
    static class ReverseIntKey implements Comparable<ReverseIntKey> {

        @KeyField(1)
        private int key;

        public int compareTo(ReverseIntKey o) {
            /* Reverse the natural order. */
            return o.key - key;
        }

        private ReverseIntKey() {}

        ReverseIntKey(int key) {
            this.key = key;
        }

        @Override
        public boolean equals(Object o) {
            return key == ((ReverseIntKey) o).key;
        }

        @Override
        public int hashCode() {
            return key;
        }

        @Override
        public String toString() {
            return "Key = " + key;
        }
    }

    /**
     * Ensures that custom comparators are persisted and work correctly during
     * recovery.  JE recovery uses comparators, so they are serialized and
     * stored in the DatabaseImpl.  They are deserialized during recovery prior
     * to opening the EntityStore and its format catalog.  But the formats are
     * needed by the comparator, so they are specially created when needed.
     *
     * In particular we need to ensure that enum key fields work correctly,
     * since their formats are not static (like simple type formats are).
     * [#17140]
     *
     * Note that we don't need to actually cause a recovery in order to test
     * the deserialization and subsequent use of comparators.  The JE
     * DatabaseConfig.setBtreeComparator method serializes and deserializes the
     * comparator.  The comparator is initialized on its first use, just as if
     * recovery were run.
     */
    public void testStoredComparators()
        throws DatabaseException {

        open();

        PrimaryIndex<StoredComparatorEntity.Key,
                     StoredComparatorEntity> priIndex =
            store.getPrimaryIndex(StoredComparatorEntity.Key.class,
                                  StoredComparatorEntity.class);

        SecondaryIndex<StoredComparatorEntity.MyEnum,
                       StoredComparatorEntity.Key,
                       StoredComparatorEntity> secIndex =
            store.getSecondaryIndex
                (priIndex, StoredComparatorEntity.MyEnum.class, "secKey");

        final StoredComparatorEntity.Key[] priKeys =
            new StoredComparatorEntity.Key[] {
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.A, 1,
                     StoredComparatorEntity.MyEnum.A),
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.A, 1,
                     StoredComparatorEntity.MyEnum.B),
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.A, 2,
                     StoredComparatorEntity.MyEnum.A),
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.A, 2,
                     StoredComparatorEntity.MyEnum.B),
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.B, 1,
                     StoredComparatorEntity.MyEnum.A),
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.B, 1,
                     StoredComparatorEntity.MyEnum.B),
                new StoredComparatorEntity.Key
                    (StoredComparatorEntity.MyEnum.C, 0,
                     StoredComparatorEntity.MyEnum.C),
            };

        final StoredComparatorEntity.MyEnum[] secKeys =
            new StoredComparatorEntity.MyEnum[] {
                StoredComparatorEntity.MyEnum.C,
                StoredComparatorEntity.MyEnum.B,
                StoredComparatorEntity.MyEnum.A,
                null,
                StoredComparatorEntity.MyEnum.A,
                StoredComparatorEntity.MyEnum.B,
                StoredComparatorEntity.MyEnum.C,
            };

        assertEquals(priKeys.length, secKeys.length);
        final int nEntities = priKeys.length;

        Transaction txn = txnBegin();
        for (int i = 0; i < nEntities; i += 1) {
            priIndex.put(txn,
                         new StoredComparatorEntity(priKeys[i], secKeys[i]));
        }
        txnCommit(txn);

        txn = txnBeginCursor();
        EntityCursor<StoredComparatorEntity> entities =
            priIndex.entities(txn, null);
        for (int i = nEntities - 1; i >= 0; i -= 1) {
            StoredComparatorEntity e = entities.next();
            assertNotNull(e);
            assertEquals(priKeys[i], e.key);
            assertEquals(secKeys[i], e.secKey);
        }
        assertNull(entities.next());
        entities.close();
        txnCommit(txn);

        txn = txnBeginCursor();
        entities = secIndex.entities(txn, null);
        for (StoredComparatorEntity.MyEnum myEnum :
             EnumSet.allOf(StoredComparatorEntity.MyEnum.class)) {
            for (int i = 0; i < nEntities; i += 1) {
                if (secKeys[i] == myEnum) {
                    StoredComparatorEntity e = entities.next();
                    assertNotNull(e);
                    assertEquals(priKeys[i], e.key);
                    assertEquals(secKeys[i], e.secKey);
                }
            }
        }
        assertNull(entities.next());
        entities.close();
        txnCommit(txn);

        close();
    }

    @Entity
    static class StoredComparatorEntity {

        enum MyEnum { A, B, C };

        @Persistent
        static class Key implements Comparable<Key> {

            @KeyField(1)
            MyEnum f1;

            @KeyField(2)
            Integer f2;

            @KeyField(3)
            MyEnum f3;

            private Key() {}

            Key(MyEnum f1, Integer f2, MyEnum f3) {
                this.f1 = f1;
                this.f2 = f2;
                this.f3 = f3;
            }

            public int compareTo(Key o) {
                /* Reverse the natural order. */
                int i = f1.compareTo(o.f1);
                if (i != 0) return -i;
                i = f2.compareTo(o.f2);
                if (i != 0) return -i;
                i = f3.compareTo(o.f3);
                if (i != 0) return -i;
                return 0;
            }

            @Override
            public boolean equals(Object other) {
                if (!(other instanceof Key)) {
                    return false;
                }
                Key o = (Key) other;
                return f1 == o.f1 &&
                       f2.equals(o.f2) &&
                       f3 == o.f3;
            }

            @Override
            public int hashCode() {
                return f1.ordinal() + f2 + f3.ordinal();
            }

            @Override
            public String toString() {
                return "[Key " + f1 + ' ' + f2 + ' ' + f3 + ']';
            }
        }

        @PrimaryKey
        Key key;

        @SecondaryKey(relate=MANY_TO_ONE)
        private MyEnum secKey;

        private StoredComparatorEntity() {}

        StoredComparatorEntity(Key key, MyEnum secKey) {
            this.key = key;
            this.secKey = secKey;
        }

        @Override
        public String toString() {
            return "[pri = " + key + " sec = " + secKey + ']';
        }
    }
}
