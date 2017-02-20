/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.Relationship.MANY_TO_MANY;
import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;
import static com.sleepycat.persist.model.Relationship.ONE_TO_MANY;
import static com.sleepycat.persist.model.Relationship.ONE_TO_ONE;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.SortedMap;
import java.util.SortedSet;
import java.util.TreeMap;
import java.util.TreeSet;

import junit.framework.Test;

import com.sleepycat.collections.MapEntryParameter;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.EntityCursor;
import com.sleepycat.persist.EntityIndex;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.persist.raw.RawStore;
import com.sleepycat.persist.raw.RawType;
import com.sleepycat.util.test.TxnTestCase;

/**
 * Tests EntityIndex and EntityCursor in all their permutations.
 *
 * @author Mark Hayes
 */
public class IndexTest extends TxnTestCase {

    private static final int N_RECORDS = 5;
    private static final int THREE_TO_ONE = 3;

    static protected Class<?> testClass = IndexTest.class;

    public static Test suite() {
        return txnTestSuite(testClass, null,
                            null);
                            //new String[] { TxnTestCase.TXN_NULL});
    }

    private EntityStore store;
    private PrimaryIndex<Integer,MyEntity> primary;
    private SecondaryIndex<Integer,Integer,MyEntity> oneToOne;
    private SecondaryIndex<Integer,Integer,MyEntity> manyToOne;
    private SecondaryIndex<Integer,Integer,MyEntity> oneToMany;
    private SecondaryIndex<Integer,Integer,MyEntity> manyToMany;
    private RawStore rawStore;
    private RawType entityType;
    private PrimaryIndex<Object,RawObject> primaryRaw;
    private SecondaryIndex<Object,Object,RawObject> oneToOneRaw;
    private SecondaryIndex<Object,Object,RawObject> manyToOneRaw;
    private SecondaryIndex<Object,Object,RawObject> oneToManyRaw;
    private SecondaryIndex<Object,Object,RawObject> manyToManyRaw;

    /**
     * Opens the store.
     */
    private void open()
        throws DatabaseException {

        StoreConfig config = new StoreConfig();
        config.setAllowCreate(envConfig.getAllowCreate());
        config.setTransactional(envConfig.getTransactional());

        store = new EntityStore(env, "test", config);

        primary = store.getPrimaryIndex(Integer.class, MyEntity.class);
        oneToOne =
            store.getSecondaryIndex(primary, Integer.class, "oneToOne");
        manyToOne =
            store.getSecondaryIndex(primary, Integer.class, "manyToOne");
        oneToMany =
            store.getSecondaryIndex(primary, Integer.class, "oneToMany");
        manyToMany =
            store.getSecondaryIndex(primary, Integer.class, "manyToMany");

        assertNotNull(primary);
        assertNotNull(oneToOne);
        assertNotNull(manyToOne);
        assertNotNull(oneToMany);
        assertNotNull(manyToMany);

        rawStore = new RawStore(env, "test", config);
        String clsName = MyEntity.class.getName();
        entityType = rawStore.getModel().getRawType(clsName);
        assertNotNull(entityType);

        primaryRaw = rawStore.getPrimaryIndex(clsName);
        oneToOneRaw = rawStore.getSecondaryIndex(clsName, "oneToOne");
        manyToOneRaw = rawStore.getSecondaryIndex(clsName, "manyToOne");
        oneToManyRaw = rawStore.getSecondaryIndex(clsName, "oneToMany");
        manyToManyRaw = rawStore.getSecondaryIndex(clsName, "manyToMany");

        assertNotNull(primaryRaw);
        assertNotNull(oneToOneRaw);
        assertNotNull(manyToOneRaw);
        assertNotNull(oneToManyRaw);
        assertNotNull(manyToManyRaw);
    }

    /**
     * Closes the store.
     */
    private void close()
        throws DatabaseException {

        store.close();
        store = null;
        rawStore.close();
        rawStore = null;
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
            if (rawStore != null) {
                rawStore.close();
            }
        } catch (Throwable e) {
            System.out.println("During tearDown: " + e);
        }
        try {
            if (store != null) {
                store.close();
            }
        } catch (Throwable e) {
            System.out.println("During tearDown: " + e);
        }
        store = null;
        rawStore = null;
        super.tearDown();
    }

    /**
     * Primary keys: {0, 1, 2, 3, 4}
     */
    public void testPrimary()
        throws DatabaseException {

        SortedMap<Integer,SortedSet<Integer>> expected =
            new TreeMap<Integer,SortedSet<Integer>>();

        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            SortedSet<Integer> values = new TreeSet<Integer>();
            values.add(priKey);
            expected.put(priKey, values);
        }

        open();
        addEntities(primary);
        checkIndex(primary, expected, keyGetter, entityGetter);
        checkIndex(primaryRaw, expected, rawKeyGetter, rawEntityGetter);

        /* Close and reopen, then recheck indices. */
        close();
        open();
        checkIndex(primary, expected, keyGetter, entityGetter);
        checkIndex(primaryRaw, expected, rawKeyGetter, rawEntityGetter);

        /* Check primary delete, last key first for variety. */
        for (int priKey = N_RECORDS - 1; priKey >= 0; priKey -= 1) {
            boolean useRaw = ((priKey & 1) != 0);
            Transaction txn = txnBegin();
            if (useRaw) {
                primaryRaw.delete(txn, priKey);
            } else {
                primary.delete(txn, priKey);
            }
            txnCommit(txn);
            expected.remove(priKey);
            checkIndex(primary, expected, keyGetter, entityGetter);
        }
        checkAllEmpty();

        /* Check PrimaryIndex put operations. */
        MyEntity e;
        Transaction txn = txnBegin();
        /* put() */
        e = primary.put(txn, new MyEntity(1));
        assertNull(e);
        e = primary.get(txn, 1, null);
        assertEquals(1, e.key);
        /* putNoReturn() */
        primary.putNoReturn(txn, new MyEntity(2));
        e = primary.get(txn, 2, null);
        assertEquals(2, e.key);
        /* putNoOverwrite */
        assertTrue(!primary.putNoOverwrite(txn, new MyEntity(1)));
        assertTrue(!primary.putNoOverwrite(txn, new MyEntity(2)));
        assertTrue(primary.putNoOverwrite(txn, new MyEntity(3)));
        e = primary.get(txn, 3, null);
        assertEquals(3, e.key);
        txnCommit(txn);
        close();
    }

    /**
     * { 0:0, 1:-1, 2:-2, 3:-3, 4:-4 }
     */
    public void testOneToOne()
        throws DatabaseException {

        SortedMap<Integer,SortedSet<Integer>> expected =
            new TreeMap<Integer,SortedSet<Integer>>();

        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            SortedSet<Integer> values = new TreeSet<Integer>();
            values.add(priKey);
            Integer secKey = (-priKey);
            expected.put(secKey, values);
        }

        open();
        addEntities(primary);
        checkSecondary(oneToOne, oneToOneRaw, expected);
        checkDelete(oneToOne, oneToOneRaw, expected);
        close();
    }

    /**
     * { 0:0, 1:1, 2:2, 3:0, 4:1 }
     */
    public void testManyToOne()
        throws DatabaseException {

        SortedMap<Integer,SortedSet<Integer>> expected =
            new TreeMap<Integer,SortedSet<Integer>>();

        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            Integer secKey = priKey % THREE_TO_ONE;
            SortedSet<Integer> values = expected.get(secKey);
            if (values == null) {
                values = new TreeSet<Integer>();
                expected.put(secKey, values);
            }
            values.add(priKey);
        }

        open();
        addEntities(primary);
        checkSecondary(manyToOne, manyToOneRaw, expected);
        checkDelete(manyToOne, manyToOneRaw, expected);
        close();
    }

    /**
     * { 0:{}, 1:{10}, 2:{20,21}, 3:{30,31,32}, 4:{40,41,42,43}
     */
    public void testOneToMany()
        throws DatabaseException {

        SortedMap<Integer,SortedSet<Integer>> expected =
            new TreeMap<Integer,SortedSet<Integer>>();

        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            for (int i = 0; i < priKey; i += 1) {
                Integer secKey = (N_RECORDS * priKey) + i;
                SortedSet<Integer> values = expected.get(secKey);
                if (values == null) {
                    values = new TreeSet<Integer>();
                    expected.put(secKey, values);
                }
                values.add(priKey);
            }
        }

        open();
        addEntities(primary);
        checkSecondary(oneToMany, oneToManyRaw, expected);
        checkDelete(oneToMany, oneToManyRaw, expected);
        close();
    }

    /**
     * { 0:{}, 1:{0}, 2:{0,1}, 3:{0,1,2}, 4:{0,1,2,3}
     */
    public void testManyToMany()
        throws DatabaseException {

        SortedMap<Integer,SortedSet<Integer>> expected =
            new TreeMap<Integer,SortedSet<Integer>>();

        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            for (int i = 0; i < priKey; i += 1) {
                Integer secKey = i;
                SortedSet<Integer> values = expected.get(secKey);
                if (values == null) {
                    values = new TreeSet<Integer>();
                    expected.put(secKey, values);
                }
                values.add(priKey);
            }
        }

        open();
        addEntities(primary);
        checkSecondary(manyToMany, manyToManyRaw, expected);
        checkDelete(manyToMany, manyToManyRaw, expected);
        close();
    }

    private void addEntities(PrimaryIndex<Integer,MyEntity> primary)
        throws DatabaseException {

        Transaction txn = txnBegin();
        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            MyEntity prev = primary.put(txn, new MyEntity(priKey));
            assertNull(prev);
        }
        txnCommit(txn);
    }

    private void checkDelete(SecondaryIndex<Integer,Integer,MyEntity> index,
                             SecondaryIndex<Object,Object,RawObject> indexRaw,
                             SortedMap<Integer,SortedSet<Integer>> expected)
        throws DatabaseException {

        SortedMap<Integer,SortedSet<Integer>> expectedSubIndex =
            new TreeMap<Integer,SortedSet<Integer>>();

        while (expected.size() > 0) {
            Integer delSecKey = expected.firstKey();
            SortedSet<Integer> deletedPriKeys = expected.remove(delSecKey);
            for (SortedSet<Integer> priKeys : expected.values()) {
                priKeys.removeAll(deletedPriKeys);
            }
            Transaction txn = txnBegin();
            boolean deleted = index.delete(txn, delSecKey);
            assertEquals(deleted, !deletedPriKeys.isEmpty());
            deleted = index.delete(txn, delSecKey);
            assertTrue(!deleted);
            assertNull(index.get(txn, delSecKey, null));
            txnCommit(txn);
            checkSecondary(index, indexRaw, expected);
        }

        /*
         * Delete remaining records so that the primary index is empty.  Use
         * the RawStore for variety.
         */
        Transaction txn = txnBegin();
        for (int priKey = 0; priKey < N_RECORDS; priKey += 1) {
            primaryRaw.delete(txn, priKey);
        }
        txnCommit(txn);
        checkAllEmpty();
    }

    private void checkSecondary(SecondaryIndex<Integer,Integer,MyEntity> index,
                                SecondaryIndex<Object,Object,RawObject>
                                indexRaw,
                                SortedMap<Integer,SortedSet<Integer>> expected)
        throws DatabaseException {

        checkIndex(index, expected, keyGetter, entityGetter);
        checkIndex(index.keysIndex(), expected, keyGetter, keyGetter);

        checkIndex(indexRaw, expected, rawKeyGetter, rawEntityGetter);
        checkIndex(indexRaw.keysIndex(), expected, rawKeyGetter, rawKeyGetter);

        SortedMap<Integer,SortedSet<Integer>> expectedSubIndex =
            new TreeMap<Integer,SortedSet<Integer>>();

        for (Integer secKey : expected.keySet()) {
            expectedSubIndex.clear();
            for (Integer priKey : expected.get(secKey)) {
                SortedSet<Integer> values = new TreeSet<Integer>();
                values.add(priKey);
                expectedSubIndex.put(priKey, values);
            }
            checkIndex(index.subIndex(secKey),
                       expectedSubIndex,
                       keyGetter,
                       entityGetter);
            checkIndex(indexRaw.subIndex(secKey),
                       expectedSubIndex,
                       rawKeyGetter,
                       rawEntityGetter);
        }
    }

    private <K,V> void checkIndex(EntityIndex<K,V> index,
                                  SortedMap<Integer,SortedSet<Integer>>
                                  expected,
                                  Getter<K> kGetter,
                                  Getter<V> vGetter)
        throws DatabaseException {

        SortedMap<K,V> map = index.sortedMap();

        Transaction txn = txnBegin();
        for (int i : expected.keySet()) {
            K k = kGetter.fromInt(i);
            SortedSet<Integer> dups = expected.get(i);
            if (dups.isEmpty()) {

                /* EntityIndex */
                V v = index.get(txn, k, null);
                assertNull(v);
                assertTrue(!index.contains(txn, k, null));

                /* Map/Collection */
                v = map.get(i);
                assertNull(v);
                assertTrue(!map.containsKey(i));
            } else {
                int j = dups.first();

                /* EntityIndex */
                V v = index.get(txn, k, null);
                assertNotNull(v);
                assertEquals(j, vGetter.getKey(v));
                assertTrue(index.contains(txn, k, null));

                /* Map/Collection */
                v = map.get(i);
                assertNotNull(v);
                assertEquals(j, vGetter.getKey(v));
                assertTrue(map.containsKey(i));
                assertTrue("" + i + ' ' + j + ' ' + v + ' ' + map,
                           map.containsValue(v));
                assertTrue(map.keySet().contains(i));
                assertTrue(map.values().contains(v));
                assertTrue
                    (map.entrySet().contains(new MapEntryParameter(i, v)));
            }
        }
        txnCommit(txn);

        int keysSize = expandKeySize(expected);
        int valuesSize = expandValueSize(expected);

        /* EntityIndex.count */
        assertEquals("keysSize=" + keysSize, valuesSize, index.count());

        /* Map/Collection size */
        assertEquals(valuesSize, map.size());
        assertEquals(valuesSize, map.values().size());
        assertEquals(valuesSize, map.entrySet().size());
        assertEquals(keysSize, map.keySet().size());

        /* Map/Collection isEmpty */
        assertEquals(valuesSize == 0, map.isEmpty());
        assertEquals(valuesSize == 0, map.values().isEmpty());
        assertEquals(valuesSize == 0, map.entrySet().isEmpty());
        assertEquals(keysSize == 0, map.keySet().isEmpty());

        txn = txnBeginCursor();

        /* Unconstrained cursors. */
        checkCursor
            (index.keys(txn, null),
             map.keySet(), true,
             expandKeys(expected), kGetter);
        checkCursor
            (index.entities(txn, null),
             map.values(), false,
             expandValues(expected), vGetter);

        /* Range cursors. */
        if (expected.isEmpty()) {
            checkOpenRanges(txn, 0, index, expected, kGetter, vGetter);
            checkClosedRanges(txn, 0, 1, index, expected, kGetter, vGetter);
        } else {
            int firstKey = expected.firstKey();
            int lastKey = expected.lastKey();
            for (int i = firstKey - 1; i <= lastKey + 1; i += 1) {
                checkOpenRanges(txn, i, index, expected, kGetter, vGetter);
                int j = i + 1;
                if (j < lastKey + 1) {
                    checkClosedRanges
                        (txn, i, j, index, expected, kGetter, vGetter);
                }
            }
        }

        txnCommit(txn);
    }

    private <K,V> void checkOpenRanges(Transaction txn, int i,
                                       EntityIndex<K,V> index,
                                       SortedMap<Integer,SortedSet<Integer>>
                                       expected,
                                       Getter<K> kGetter,
                                       Getter<V> vGetter)
        throws DatabaseException {

        SortedMap<K,V> map = index.sortedMap();
        SortedMap<Integer,SortedSet<Integer>> rangeExpected;
        K k = kGetter.fromInt(i);
        K kPlusOne = kGetter.fromInt(i + 1);

        /* Head range exclusive. */
        rangeExpected = expected.headMap(i);
        checkCursor
            (index.keys(txn, null, false, k, false, null),
             map.headMap(k).keySet(), true,
             expandKeys(rangeExpected), kGetter);
        checkCursor
            (index.entities(txn, null, false, k, false, null),
             map.headMap(k).values(), false,
             expandValues(rangeExpected), vGetter);

        /* Head range inclusive. */
        rangeExpected = expected.headMap(i + 1);
        checkCursor
            (index.keys(txn, null, false, k, true, null),
             map.headMap(kPlusOne).keySet(), true,
             expandKeys(rangeExpected), kGetter);
        checkCursor
            (index.entities(txn, null, false, k, true, null),
             map.headMap(kPlusOne).values(), false,
             expandValues(rangeExpected), vGetter);

        /* Tail range exclusive. */
        rangeExpected = expected.tailMap(i + 1);
        checkCursor
            (index.keys(txn, k, false, null, false, null),
             map.tailMap(kPlusOne).keySet(), true,
             expandKeys(rangeExpected), kGetter);
        checkCursor
            (index.entities(txn, k, false, null, false, null),
             map.tailMap(kPlusOne).values(), false,
             expandValues(rangeExpected), vGetter);

        /* Tail range inclusive. */
        rangeExpected = expected.tailMap(i);
        checkCursor
            (index.keys(txn, k, true, null, false, null),
             map.tailMap(k).keySet(), true,
             expandKeys(rangeExpected), kGetter);
        checkCursor
            (index.entities(txn, k, true, null, false, null),
             map.tailMap(k).values(), false,
             expandValues(rangeExpected), vGetter);
    }

    private <K,V> void checkClosedRanges(Transaction txn, int i, int j,
                                         EntityIndex<K,V> index,
                                         SortedMap<Integer,SortedSet<Integer>>
                                         expected,
                                         Getter<K> kGetter,
                                         Getter<V> vGetter)
        throws DatabaseException {

        SortedMap<K,V> map = index.sortedMap();
        SortedMap<Integer,SortedSet<Integer>> rangeExpected;
        K k = kGetter.fromInt(i);
        K kPlusOne = kGetter.fromInt(i + 1);
        K l = kGetter.fromInt(j);
        K lPlusOne = kGetter.fromInt(j + 1);

        /* Sub range exclusive. */
        rangeExpected = expected.subMap(i + 1, j);
        checkCursor
            (index.keys(txn, k, false, l, false, null),
             map.subMap(kPlusOne, l).keySet(), true,
             expandKeys(rangeExpected), kGetter);
        checkCursor
            (index.entities(txn, k, false, l, false, null),
             map.subMap(kPlusOne, l).values(), false,
             expandValues(rangeExpected), vGetter);

        /* Sub range inclusive. */
        rangeExpected = expected.subMap(i, j + 1);
        checkCursor
            (index.keys(txn, k, true, l, true, null),
             map.subMap(k, lPlusOne).keySet(), true,
             expandKeys(rangeExpected), kGetter);
        checkCursor
            (index.entities(txn, k, true, l, true, null),
             map.subMap(k, lPlusOne).values(), false,
             expandValues(rangeExpected), vGetter);
    }

    private List<List<Integer>>
        expandKeys(SortedMap<Integer,SortedSet<Integer>> map) {

        List<List<Integer>> list = new ArrayList<List<Integer>>();
        for (Integer key : map.keySet()) {
            SortedSet<Integer> values = map.get(key);
            List<Integer> dups = new ArrayList<Integer>();
            for (int i = 0; i < values.size(); i += 1) {
                dups.add(key);
            }
            list.add(dups);
        }
        return list;
    }

    private List<List<Integer>>
        expandValues(SortedMap<Integer,SortedSet<Integer>> map) {

        List<List<Integer>> list = new ArrayList<List<Integer>>();
        for (SortedSet<Integer> values : map.values()) {
            list.add(new ArrayList<Integer>(values));
        }
        return list;
    }

    private int expandKeySize(SortedMap<Integer,SortedSet<Integer>> map) {

        int size = 0;
        for (SortedSet<Integer> values : map.values()) {
            if (values.size() > 0) {
                size += 1;
            }
        }
        return size;
    }

    private int expandValueSize(SortedMap<Integer,SortedSet<Integer>> map) {

        int size = 0;
        for (SortedSet<Integer> values : map.values()) {
            size += values.size();
        }
        return size;
    }

    private <T> void checkCursor(EntityCursor<T> cursor,
                                 Collection<T> collection,
                                 boolean collectionIsKeySet,
                                 List<List<Integer>> expected,
                                 Getter<T> getter)
        throws DatabaseException {

        boolean first;
        boolean firstDup;
        Iterator<T> iterator = collection.iterator();

        for (List<Integer> dups : expected) {
            for (int i : dups) {
                T o = cursor.next();
                assertNotNull(o);
                assertEquals(i, getter.getKey(o));
                /* Value iterator over duplicates. */
                if (!collectionIsKeySet) {
                    assertTrue(iterator.hasNext());
                    o = iterator.next();
                    assertNotNull(o);
                    assertEquals(i, getter.getKey(o));
                }
            }
        }

        first = true;
        for (List<Integer> dups : expected) {
            firstDup = true;
            for (int i : dups) {
                T o = first ? cursor.first()
                            : (firstDup ? cursor.next() : cursor.nextDup());
                assertNotNull(o);
                assertEquals(i, getter.getKey(o));
                first = false;
                firstDup = false;
            }
        }

        first = true;
        for (List<Integer> dups : expected) {
            if (!dups.isEmpty()) {
                int i = dups.get(0);
                T o = first ? cursor.first() : cursor.nextNoDup();
                assertNotNull(o);
                assertEquals(i, getter.getKey(o));
                /* Key iterator over non-duplicates. */
                if (collectionIsKeySet) {
                    assertTrue(iterator.hasNext());
                    o = iterator.next();
                    assertNotNull(o);
                    assertEquals(i, getter.getKey(o));
                }
                first = false;
            }
        }

        List<List<Integer>> reversed = new ArrayList<List<Integer>>();
        for (List<Integer> dups : expected) {
            ArrayList<Integer> reversedDups = new ArrayList<Integer>(dups);
            Collections.reverse(reversedDups);
            reversed.add(reversedDups);
        }
        Collections.reverse(reversed);

        first = true;
        for (List<Integer> dups : reversed) {
            for (int i : dups) {
                T o = first ? cursor.last() : cursor.prev();
                assertNotNull(o);
                assertEquals(i, getter.getKey(o));
                first = false;
            }
        }

        first = true;
        for (List<Integer> dups : reversed) {
            firstDup = true;
            for (int i : dups) {
                T o = first ? cursor.last()
                            : (firstDup ? cursor.prev() : cursor.prevDup());
                assertNotNull(o);
                assertEquals(i, getter.getKey(o));
                first = false;
                firstDup = false;
            }
        }

        first = true;
        for (List<Integer> dups : reversed) {
            if (!dups.isEmpty()) {
                int i = dups.get(0);
                T o = first ? cursor.last() : cursor.prevNoDup();
                assertNotNull(o);
                assertEquals(i, getter.getKey(o));
                first = false;
            }
        }

        cursor.close();
    }

    private void checkAllEmpty()
        throws DatabaseException {

        checkEmpty(primary);
        checkEmpty(oneToOne);
        checkEmpty(oneToMany);
        checkEmpty(manyToOne);
        checkEmpty(manyToMany);
    }

    private <K,V> void checkEmpty(EntityIndex<K,V> index)
        throws DatabaseException {

        EntityCursor<K> keys = index.keys();
        assertNull(keys.next());
        assertTrue(!keys.iterator().hasNext());
        keys.close();
        EntityCursor<V> entities = index.entities();
        assertNull(entities.next());
        assertTrue(!entities.iterator().hasNext());
        entities.close();
    }

    private interface Getter<T> {
        int getKey(T o);
        T fromInt(int i);
    }

    private static Getter<MyEntity> entityGetter =
               new Getter<MyEntity>() {
        public int getKey(MyEntity o) {
            return o.key;
        }
        public MyEntity fromInt(int i) {
            throw new UnsupportedOperationException();
        }
    };

    private static Getter<Integer> keyGetter =
               new Getter<Integer>() {
        public int getKey(Integer o) {
            return o;
        }
        public Integer fromInt(int i) {
            return Integer.valueOf(i);
        }
    };

    private static Getter<RawObject> rawEntityGetter =
               new Getter<RawObject>() {
        public int getKey(RawObject o) {
            Object val = o.getValues().get("key");
            return ((Integer) val).intValue();
        }
        public RawObject fromInt(int i) {
            throw new UnsupportedOperationException();
        }
    };

    private static Getter<Object> rawKeyGetter =
               new Getter<Object>() {
        public int getKey(Object o) {
            return ((Integer) o).intValue();
        }
        public Object fromInt(int i) {
            return Integer.valueOf(i);
        }
    };

    @Entity
    private static class MyEntity {

        @PrimaryKey
        private int key;

        @SecondaryKey(relate=ONE_TO_ONE)
        private int oneToOne;

        @SecondaryKey(relate=MANY_TO_ONE)
        private int manyToOne;

        @SecondaryKey(relate=ONE_TO_MANY)
        private Set<Integer> oneToMany = new TreeSet<Integer>();

        @SecondaryKey(relate=MANY_TO_MANY)
        private Set<Integer> manyToMany = new TreeSet<Integer>();

        private MyEntity() {}

        private MyEntity(int key) {

            /* example keys: {0, 1, 2, 3, 4} */
            this.key = key;

            /* { 0:0, 1:-1, 2:-2, 3:-3, 4:-4 } */
            oneToOne = -key;

            /* { 0:0, 1:1, 2:2, 3:0, 4:1 } */
            manyToOne = key % THREE_TO_ONE;

            /* { 0:{}, 1:{10}, 2:{20,21}, 3:{30,31,32}, 4:{40,41,42,43} */
            for (int i = 0; i < key; i += 1) {
                oneToMany.add((N_RECORDS * key) + i);
            }

            /* { 0:{}, 1:{0}, 2:{0,1}, 3:{0,1,2}, 4:{0,1,2,3} */
            for (int i = 0; i < key; i += 1) {
                manyToMany.add(i);
            }
        }

        @Override
        public String toString() {
            return "MyEntity " + key;
        }
    }
}
