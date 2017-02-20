/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections.test;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Set;
import java.util.SortedMap;
import java.util.SortedSet;
import java.util.concurrent.ConcurrentMap;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.collections.MapEntryParameter;
import com.sleepycat.collections.StoredCollection;
import com.sleepycat.collections.StoredCollections;
import com.sleepycat.collections.StoredContainer;
import com.sleepycat.collections.StoredEntrySet;
import com.sleepycat.collections.StoredIterator;
import com.sleepycat.collections.StoredKeySet;
import com.sleepycat.collections.StoredList;
import com.sleepycat.collections.StoredMap;
import com.sleepycat.collections.StoredSortedEntrySet;
import com.sleepycat.collections.StoredSortedKeySet;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.collections.StoredSortedValueSet;
import com.sleepycat.collections.StoredValueSet;
import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestEnv;

/**
 * @author Mark Hayes
 */
public class CollectionTest extends TestCase {

    private static final int NONE = 0;
    private static final int SUB = 1;
    private static final int HEAD = 2;
    private static final int TAIL = 3;

    /*
     * For long tests we permute testStoredIterator to test both StoredIterator
     * and BlockIterator.  When testing BlockIterator, we permute the maxKey
     * over the array values below.  BlockIterator's block size is 10.  So we
     * test below the block size (6), at the block size (10), and above it (14
     * and 22).
     */
    private static final int DEFAULT_MAX_KEY = 6;
    private static final int[] MAX_KEYS = {6, 10, 14, 22};

    private boolean testStoredIterator;
    private int maxKey; /* Must be a multiple of 2. */
    private int beginKey = 1;
    private int endKey;

    private Environment env;
    private Database store;
    private Database index;
    private final boolean isEntityBinding;
    private final boolean isAutoCommit;
    private TestStore testStore;
    private String testName;
    private final EntryBinding keyBinding;
    private final EntryBinding valueBinding;
    private final EntityBinding entityBinding;
    private TransactionRunner readRunner;
    private TransactionRunner writeRunner;
    private TransactionRunner writeIterRunner;
    private TestEnv testEnv;

    private StoredMap map;
    private StoredMap imap; // insertable map (primary store for indexed map)
    private StoredSortedMap smap; // sorted map (null or equal to map)
    private StoredMap saveMap;
    private StoredSortedMap saveSMap;
    private int rangeType;
    private StoredList list;
    private StoredList ilist; // insertable list (primary store for index list)
    private StoredList saveList;
    private StoredKeySet keySet;
    private StoredValueSet valueSet;

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
		junit.textui.TestRunner.run(suite(args));
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
            "Usage: java com.sleepycat.collections.test.CollectionTest\n" +
            "              -h | -help\n" +
            "              [testName]...\n" +
            "  where testName has the format:\n" +
            "    <env>-<store>-{entity|value}\n" +
            "  <env> is:\n" +
            "    bdb | cdb | txn\n" +
            "  <store> is:\n" +
            "    btree-uniq | btree-dup | btree-dupsort | btree-recnum |\n" +
            "    hash-uniq | hash-dup | hash-dupsort |\n" +
            "    queue | recno | recno-renum\n" +
            "  For example:  bdb-btree-uniq-entity\n" +
            "  If no arguments are given then all tests are run.");
        System.exit(2);
    }

    public static Test suite() {
        return suite(null);
    }

    static Test suite(String[] args) {
        if (SharedTestUtils.runLongTests()) {
            TestSuite suite = new TestSuite();

            /* StoredIterator tests. */
            permuteTests(args, suite, true, DEFAULT_MAX_KEY);

            /* BlockIterator tests with different maxKey values. */
            for (int i = 0; i < MAX_KEYS.length; i += 1) {
                permuteTests(args, suite, false, MAX_KEYS[i]);
            }

            return suite;
        } else {
            return baseSuite(args);
        }
    }

    private static void permuteTests(String[] args,
                                     TestSuite suite,
                                     boolean storedIter,
                                     int maxKey) {
       TestSuite baseTests = baseSuite(args);
        Enumeration e = baseTests.tests();
        while (e.hasMoreElements()) {
            CollectionTest t = (CollectionTest) e.nextElement();
            t.setParams(storedIter, maxKey);
            suite.addTest(t);
        }
    }

    private static TestSuite baseSuite(String[] args) {
        TestSuite suite = new TestSuite();
        for (int i = 0; i < TestEnv.ALL.length; i += 1) {
            for (int j = 0; j < TestStore.ALL.length; j += 1) {
                for (int k = 0; k < 2; k += 1) {
                    boolean entityBinding = (k != 0);

                    addTest(args, suite, new CollectionTest(
                            TestEnv.ALL[i], TestStore.ALL[j],
                            entityBinding, false));

                    if (TestEnv.ALL[i].isTxnMode()) {
                        addTest(args, suite, new CollectionTest(
                                TestEnv.ALL[i], TestStore.ALL[j],
                                entityBinding, true));
                    }
                }
            }
        }
        return suite;
    }

    private static void addTest(String[] args, TestSuite suite,
                                CollectionTest test) {

        if (args == null || args.length == 0) {
            suite.addTest(test);
        } else {
            for (int t = 0; t < args.length; t += 1) {
                if (args[t].equals(test.testName)) {
                    suite.addTest(test);
                    break;
                }
            }
        }
    }

    public CollectionTest(TestEnv testEnv, TestStore testStore,
                          boolean isEntityBinding, boolean isAutoCommit) {

        super(null);

        this.testEnv = testEnv;
        this.testStore = testStore;
        this.isEntityBinding = isEntityBinding;
        this.isAutoCommit = isAutoCommit;

        keyBinding = testStore.getKeyBinding();
        valueBinding = testStore.getValueBinding();
        entityBinding = testStore.getEntityBinding();

        setParams(false, DEFAULT_MAX_KEY);
    }

    private void setParams(boolean storedIter, int maxKey) {

        this.testStoredIterator = storedIter;
        this.maxKey = maxKey;
        this.endKey = maxKey;

        testName = testEnv.getName() + '-' + testStore.getName() +
                    (isEntityBinding ? "-entity" : "-value") +
                    (isAutoCommit ? "-autoCommit" : "") +
                    (testStoredIterator ? "-storedIter" : "") +
                    ((maxKey != DEFAULT_MAX_KEY) ? ("-maxKey-" + maxKey) : "");
    }

    @Override
    public void tearDown() {
        setName(testName);
    }

    @Override
    public void runTest()
        throws Exception {

        SharedTestUtils.printTestName(SharedTestUtils.qualifiedTestName(this));
        try {
            env = testEnv.open(testName);

            // For testing auto-commit, use a normal (transactional) runner for
            // all reading and for writing via an iterator, and a do-nothing
            // runner for writing via collections; if auto-commit is tested,
            // the per-collection auto-commit property will be set elsewhere.
            //
            TransactionRunner normalRunner = newTransactionRunner(env);
            normalRunner.setAllowNestedTransactions(
                    DbCompat.NESTED_TRANSACTIONS);
            TransactionRunner nullRunner = new NullTransactionRunner(env);
            readRunner = nullRunner;
            if (isAutoCommit) {
                writeRunner = nullRunner;
                writeIterRunner = testStoredIterator ? normalRunner
                                                     : nullRunner;
            } else {
                writeRunner = normalRunner;
                writeIterRunner = normalRunner;
            }

            store = testStore.open(env, "unindexed.db");
            testUnindexed();
            store.close();
            store = null;

            TestStore indexOf = testStore.getIndexOf();
            if (indexOf != null) {
                store = indexOf.open(env, "indexed.db");
                index = testStore.openIndex(store, "index.db");
                testIndexed();
                index.close();
                index = null;
                store.close();
                store = null;
            }
            env.close();
            env = null;
        } catch (Exception e) {
            throw ExceptionUnwrapper.unwrap(e);
        } finally {
            if (index != null) {
                try {
		    index.close();
		} catch (Exception e) {
		}
            }
            if (store != null) {
                try {
		    store.close();
		} catch (Exception e) {
		}
            }
            if (env != null) {
                try {
		    env.close();
		} catch (Exception e) {
		}
            }
            /* Ensure that GC can cleanup. */
            index = null;
            store = null;
            env = null;
            readRunner = null;
            writeRunner = null;
            writeIterRunner = null;
            map = null;
            imap = null;
            smap = null;
            saveMap = null;
            saveSMap = null;
            list = null;
            ilist = null;
            saveList = null;
            keySet = null;
            valueSet = null;
	    testEnv = null;
	    testStore = null;
        }
    }

    /**
     * Is overridden in XACollectionTest.
     * @throws DatabaseException from subclasses.
     */
    protected TransactionRunner newTransactionRunner(Environment env)
        throws DatabaseException {

        return new TransactionRunner(env);
    }

    void testCreation(StoredContainer cont, int expectSize) {
        assertEquals(index != null, cont.isSecondary());
        assertEquals(testStore.isOrdered(), cont.isOrdered());
        assertEquals(testStore.areKeyRangesAllowed(),
                     cont.areKeyRangesAllowed());
        assertEquals(testStore.areKeysRenumbered(), cont.areKeysRenumbered());
        assertEquals(testStore.areDuplicatesAllowed(),
                     cont.areDuplicatesAllowed());
        assertEquals(testEnv.isTxnMode(), cont.isTransactional());
        assertEquals(expectSize, cont.size());
    }

    void testMapCreation(ConcurrentMap map) {
        assertTrue(map.values() instanceof Set);
        assertEquals(testStore.areKeyRangesAllowed(),
                     map.keySet() instanceof SortedSet);
        assertEquals(testStore.areKeyRangesAllowed(),
                     map.entrySet() instanceof SortedSet);
        assertEquals(testStore.areKeyRangesAllowed() && isEntityBinding,
                     map.values() instanceof SortedSet);
    }

    void testUnindexed()
        throws Exception {

        // create primary map
        if (testStore.areKeyRangesAllowed()) {
            if (isEntityBinding) {
                smap = new StoredSortedMap(store, keyBinding,
                                           entityBinding,
                                           testStore.getKeyAssigner());
                valueSet = new StoredSortedValueSet(store, entityBinding,
                                                    true);
            } else {
                smap = new StoredSortedMap(store, keyBinding,
                                           valueBinding,
                                           testStore.getKeyAssigner());
                // sorted value set is not possible since key cannot be derived
                // for performing subSet, etc.
            }
            keySet = new StoredSortedKeySet(store, keyBinding, true);
            map = smap;
        } else {
            if (isEntityBinding) {
                map = new StoredMap(store, keyBinding, entityBinding,
                                    testStore.getKeyAssigner());
                valueSet = new StoredValueSet(store, entityBinding, true);
            } else {
                map = new StoredMap(store, keyBinding, valueBinding,
                                    testStore.getKeyAssigner());
                valueSet = new StoredValueSet(store, valueBinding, true);
            }
            smap = null;
            keySet = new StoredKeySet(store, keyBinding, true);
        }
        imap = map;

        // create primary list
        if (testStore.hasRecNumAccess()) {
            if (isEntityBinding) {
                ilist = new StoredList(store, entityBinding,
                                       testStore.getKeyAssigner());
            } else {
                ilist = new StoredList(store, valueBinding,
                                       testStore.getKeyAssigner());
            }
            list = ilist;
        } else {
            try {
                if (isEntityBinding) {
                    ilist = new StoredList(store, entityBinding,
                                           testStore.getKeyAssigner());
                } else {
                    ilist = new StoredList(store, valueBinding,
                                           testStore.getKeyAssigner());
                }
                fail();
            } catch (IllegalArgumentException expected) {}
        }

        testCreation(map, 0);
        if (list != null) {
            testCreation(list, 0);
        }
        testMapCreation(map);
        addAll();
        testAll();
    }

    void testIndexed()
        throws Exception {

        // create primary map
        if (isEntityBinding) {
            map = new StoredMap(store, keyBinding, entityBinding,
                                testStore.getKeyAssigner());
        } else {
            map = new StoredMap(store, keyBinding, valueBinding,
                                testStore.getKeyAssigner());
        }
        imap = map;
        smap = null;
        // create primary list
        if (testStore.hasRecNumAccess()) {
            if (isEntityBinding) {
                list = new StoredList(store, entityBinding,
                                      testStore.getKeyAssigner());
            } else {
                list = new StoredList(store, valueBinding,
                                      testStore.getKeyAssigner());
            }
            ilist = list;
        }

        addAll();
        readAll();

        // create indexed map (keySet/valueSet)
        if (testStore.areKeyRangesAllowed()) {
            if (isEntityBinding) {
                map = smap = new StoredSortedMap(index, keyBinding,
                                                 entityBinding, true);
                valueSet = new StoredSortedValueSet(index, entityBinding,
                                                    true);
            } else {
                map = smap = new StoredSortedMap(index, keyBinding,
                                                 valueBinding, true);
                // sorted value set is not possible since key cannot be derived
                // for performing subSet, etc.
            }
            keySet = new StoredSortedKeySet(index, keyBinding, true);
        } else {
            if (isEntityBinding) {
                map = new StoredMap(index, keyBinding, entityBinding, true);
                valueSet = new StoredValueSet(index, entityBinding, true);
            } else {
                map = new StoredMap(index, keyBinding, valueBinding, true);
                valueSet = new StoredValueSet(index, valueBinding, true);
            }
            smap = null;
            keySet = new StoredKeySet(index, keyBinding, true);
        }

        // create indexed list
        if (testStore.hasRecNumAccess()) {
            if (isEntityBinding) {
                list = new StoredList(index, entityBinding, true);
            } else {
                list = new StoredList(index, valueBinding, true);
            }
        } else {
            try {
                if (isEntityBinding) {
                    list = new StoredList(index, entityBinding, true);
                } else {
                    list = new StoredList(index, valueBinding, true);
                }
                fail();
            } catch (IllegalArgumentException expected) {}
        }

        testCreation(map, maxKey);
        testCreation((StoredContainer) map.values(), maxKey);
        testCreation((StoredContainer) map.keySet(), maxKey);
        testCreation((StoredContainer) map.entrySet(), maxKey);
        if (list != null) {
            testCreation(list, maxKey);
        }
        testMapCreation(map);
        testAll();
    }

    void testAll()
        throws Exception {

        checkKeySetAndValueSet();
        readAll();
        updateAll();
        readAll();
        if (!map.areKeysRenumbered()) {
            removeOdd();
            readEven();
            addOdd();
            readAll();
            removeOddIter();
            readEven();
            if (imap.areDuplicatesAllowed()) {
                addOddDup();
            } else {
                addOdd();
            }
            readAll();
            removeOddEntry();
            readEven();
            addOdd();
            readAll();
            if (isEntityBinding) {
                removeOddEntity();
                readEven();
                addOddEntity();
                readAll();
            }
            bulkOperations();
        }
        if (isListAddAllowed()) {
            removeOddList();
            readEvenList();
            addOddList();
            readAll();
            if (!isEntityBinding) {
                removeOddListValue();
                readEvenList();
                addOddList();
                readAll();
            }
        }
        if (list != null) {
            bulkListOperations();
        } else {
            listOperationsNotAllowed();
        }
        if (smap != null) {
            readWriteRange(SUB,  1, 1);
            readWriteRange(HEAD, 1, 1);
            readWriteRange(SUB,  1, maxKey);
            readWriteRange(HEAD, 1, maxKey);
            readWriteRange(TAIL, 1, maxKey);
            readWriteRange(SUB,  1, 3);
            readWriteRange(HEAD, 1, 3);
            readWriteRange(SUB,  2, 2);
            readWriteRange(SUB,  2, maxKey);
            readWriteRange(TAIL, 2, maxKey);
            readWriteRange(SUB,  maxKey, maxKey);
            readWriteRange(TAIL, maxKey, maxKey);
            readWriteRange(SUB,  maxKey + 1, maxKey + 1);
            readWriteRange(TAIL, maxKey + 1, maxKey + 1);
            readWriteRange(SUB,  0, 0);
            readWriteRange(HEAD, 0, 0);
        }
        updateAll();
        readAll();
        if (map.areDuplicatesAllowed()) {
            readWriteDuplicates();
            readAll();
        } else {
            duplicatesNotAllowed();
            readAll();
        }
        if (testEnv.isCdbMode()) {
            testCdbLocking();
        }
        removeAll();
        if (!map.areKeysRenumbered()) {
            testConcurrentMap();
        }
        if (isListAddAllowed()) {
            testIterAddList();
            clearAll();
        }
        if (imap.areDuplicatesAllowed()) {
            testIterAddDuplicates();
            clearAll();
        }
        if (isListAddAllowed()) {
            addAllList();
            readAll();
            removeAllList();
        }
        appendAll();
    }

    void checkKeySetAndValueSet() {

        // use bulk operations to check that explicitly constructed
        // keySet/valueSet are equivalent
        assertEquals(keySet, imap.keySet());
        if (valueSet != null) {
            assertEquals(valueSet, imap.values());
        }
    }

    Iterator iterator(Collection storedCollection) {

        if (testStoredIterator) {
            return ((StoredCollection) storedCollection).storedIterator();
        } else {
            return storedCollection.iterator();
        }
    }

    void addAll()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                assertTrue(imap.isEmpty());
                Iterator iter = iterator(imap.entrySet());
                try {
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }
                assertEquals(0, imap.keySet().toArray().length);
                assertEquals(0, imap.keySet().toArray(new Object[0]).length);
                assertEquals(0, imap.entrySet().toArray().length);
                assertEquals(0, imap.entrySet().toArray(new Object[0]).length);
                assertEquals(0, imap.values().toArray().length);
                assertEquals(0, imap.values().toArray(new Object[0]).length);

                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertNull(imap.get(key));
                    assertTrue(!imap.keySet().contains(key));
                    assertTrue(!imap.values().contains(val));
                    assertNull(imap.put(key, val));
                    assertEquals(val, imap.get(key));
                    assertTrue(imap.keySet().contains(key));
                    assertTrue(imap.values().contains(val));
                    assertTrue(imap.duplicates(key).contains(val));
                    if (!imap.areDuplicatesAllowed()) {
                        assertEquals(val, imap.put(key, val));
                    }
                    checkDupsSize(1, imap.duplicates(key));
                }
                assertTrue(!imap.isEmpty());
            }
        });
    }

    void appendAll()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                assertTrue(imap.isEmpty());

                TestKeyAssigner keyAssigner = testStore.getKeyAssigner();
                if (keyAssigner != null) {
                    keyAssigner.reset();
                }

                for (int i = beginKey; i <= endKey; i += 1) {
                    boolean useList = (i & 1) == 0;
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertNull(imap.get(key));
                    if (keyAssigner != null) {
                        if (useList && ilist != null) {
                            assertEquals(i - 1, ilist.append(val));
                        } else {
                            assertEquals(key, imap.append(val));
                        }
                        assertEquals(val, imap.get(key));
                    } else {
                        Long recnoKey;
                        if (useList && ilist != null) {
                            recnoKey = new Long(ilist.append(val) + 1);
                        } else {
                            recnoKey = (Long) imap.append(val);
                        }
                        assertNotNull(recnoKey);
                        Object recnoVal;
                        if (isEntityBinding) {
                            recnoVal = makeEntity(recnoKey.intValue(), i);
                        } else {
                            recnoVal = val;
                        }
                        assertEquals(recnoVal, imap.get(recnoKey));
                    }
                }
            }
        });
    }

    void updateAll()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() throws Exception {
                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    if (!imap.areDuplicatesAllowed()) {
                        assertEquals(val, imap.put(key, val));
                    }
                    if (isEntityBinding) {
                        assertTrue(!imap.values().add(val));
                    }
                    checkDupsSize(1, imap.duplicates(key));
                    if (ilist != null) {
                        int idx = i - 1;
                        assertEquals(val, ilist.set(idx, val));
                    }
                }
                updateIter(map.entrySet());
                updateIter(map.values());
                if (beginKey <= endKey) {
                    ListIterator iter = (ListIterator) iterator(map.keySet());
                    try {
                        assertNotNull(iter.next());
                        iter.set(makeKey(beginKey));
                        fail();
                    } catch (UnsupportedOperationException e) {
                    } finally {
                        StoredIterator.close(iter);
                    }
                }
                if (list != null) {
                    updateIter(list);
                }
            }
        });
    }

    void updateIter(final Collection coll)
        throws Exception {

        writeIterRunner.run(new TransactionWorker() {
            public void doWork() {
                ListIterator iter = (ListIterator) iterator(coll);
                try {
                    for (int i = beginKey; i <= endKey; i += 1) {
                        assertTrue(iter.hasNext());
                        Object obj = iter.next();
                        if (map.isOrdered()) {
                            assertEquals(i, intIter(coll, obj));
                        }
                        if (index != null) {
                            try {
                                setValuePlusOne(iter, obj);
                                fail();
                            } catch (UnsupportedOperationException e) {}
                        } else if
                           (((StoredCollection) coll).areDuplicatesOrdered()) {
                            try {
                                setValuePlusOne(iter, obj);
                                fail();
                            } catch (RuntimeException e) {
                                Exception e2 = ExceptionUnwrapper.unwrap(e);
                                assertTrue(e2.getClass().getName(),
                                      e2 instanceof IllegalArgumentException ||
                                      e2 instanceof DatabaseException);
                            }
                        } else {
                            setValuePlusOne(iter, obj);
                            /* Ensure iterator position is correct. */
                            if (map.isOrdered()) {
                                assertTrue(iter.hasPrevious());
                                obj = iter.previous();
                                assertEquals(i, intIter(coll, obj));
                                assertTrue(iter.hasNext());
                                obj = iter.next();
                                assertEquals(i, intIter(coll, obj));
                            }
                        }
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }
            }
        });
    }

    void setValuePlusOne(ListIterator iter, Object obj) {

        if (obj instanceof Map.Entry) {
            Map.Entry entry = (Map.Entry) obj;
            Long key = (Long) entry.getKey();
            Object oldVal = entry.getValue();
            Object val = makeVal(key.intValue() + 1);
            if (isEntityBinding) {
                try {
                    // must fail on attempt to change the key via an entity
                    entry.setValue(val);
                    fail();
                } catch (IllegalArgumentException e) {}
                val = makeEntity(key.intValue(), key.intValue() + 1);
            }
            entry.setValue(val);
            assertEquals(val, entry.getValue());
            assertEquals(val, map.get(key));
            assertTrue(map.duplicates(key).contains(val));
            checkDupsSize(1, map.duplicates(key));
            entry.setValue(oldVal);
            assertEquals(oldVal, entry.getValue());
            assertEquals(oldVal, map.get(key));
            assertTrue(map.duplicates(key).contains(oldVal));
            checkDupsSize(1, map.duplicates(key));
        } else {
            Object oldVal = obj;
            Long key = makeKey(intVal(obj));
            Object val = makeVal(key.intValue() + 1);
            if (isEntityBinding) {
                try {
                    // must fail on attempt to change the key via an entity
                    iter.set(val);
                    fail();
                } catch (IllegalArgumentException e) {}
                val = makeEntity(key.intValue(), key.intValue() + 1);
            }
            iter.set(val);
            assertEquals(val, map.get(key));
            assertTrue(map.duplicates(key).contains(val));
            checkDupsSize(1, map.duplicates(key));
            iter.set(oldVal);
            assertEquals(oldVal, map.get(key));
            assertTrue(map.duplicates(key).contains(oldVal));
            checkDupsSize(1, map.duplicates(key));
        }
    }

    void removeAll()
        throws Exception {

        writeIterRunner.run(new TransactionWorker() {
            public void doWork() {
                assertTrue(!map.isEmpty());
                ListIterator iter = null;
                try {
                    if (list != null) {
                        iter = (ListIterator) iterator(list);
                    } else {
                        iter = (ListIterator) iterator(map.values());
                    }
                    iteratorSetAndRemoveNotAllowed(iter);

                    Object val = iter.next();
                    assertNotNull(val);
                    iter.remove();
                    iteratorSetAndRemoveNotAllowed(iter);

                    if (index == null) {
                        val = iter.next();
                        assertNotNull(val);
                        iter.set(val);

                        if (map.areDuplicatesAllowed()) {
                            iter.add(makeVal(intVal(val), intVal(val) + 1));
                            iteratorSetAndRemoveNotAllowed(iter);
                        }
                    }
                } finally {
                    StoredIterator.close(iter);
                }
                map.clear();
                assertTrue(map.isEmpty());
                assertTrue(map.entrySet().isEmpty());
                assertTrue(map.keySet().isEmpty());
                assertTrue(map.values().isEmpty());
                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertNull(map.get(key));
                    assertTrue(!map.duplicates(key).contains(val));
                    checkDupsSize(0, map.duplicates(key));
                }
            }
        });
    }

    void clearAll()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                map.clear();
                assertTrue(map.isEmpty());
            }
        });
    }

    /**
     * Tests that removing while iterating works properly, especially when
     * removing everything in the key range or everything from some point to
     * the end of the range. [#15858]
     */
    void removeIter()
        throws Exception {

        writeIterRunner.run(new TransactionWorker() {
            public void doWork() {
                ListIterator iter;

                /* Save contents. */
                HashMap<Object,Object> savedMap =
                    new HashMap<Object,Object>(map);
                assertEquals(savedMap, map);

                /* Remove all moving forward. */
                iter = (ListIterator) iterator(map.keySet());
                try {
                    while (iter.hasNext()) {
                        assertNotNull(iter.next());
                        iter.remove();
                    }
                    assertTrue(!iter.hasNext());
                    assertTrue(!iter.hasPrevious());
                    assertTrue(map.isEmpty());
                } finally {
                    StoredIterator.close(iter);
                }

                /* Restore contents. */
                imap.putAll(savedMap);
                assertEquals(savedMap, map);

                /* Remove all moving backward. */
                iter = (ListIterator) iterator(map.keySet());
                try {
                    while (iter.hasNext()) {
                        assertNotNull(iter.next());
                    }
                    while (iter.hasPrevious()) {
                        assertNotNull(iter.previous());
                        iter.remove();
                    }
                    assertTrue(!iter.hasNext());
                    assertTrue(!iter.hasPrevious());
                    assertTrue(map.isEmpty());
                } finally {
                    StoredIterator.close(iter);
                }

                /* Restore contents. */
                imap.putAll(savedMap);
                assertEquals(savedMap, map);

                int first = Math.max(1, beginKey);
                int last = Math.min(maxKey, endKey);

                /* Skip N forward, remove all from that point forward. */
                for (int readTo = first + 1; readTo <= last; readTo += 1) {
                    iter = (ListIterator) iterator(map.keySet());
                    try {
                        for (int i = first; i < readTo; i += 1) {
                            assertTrue(iter.hasNext());
                            assertNotNull(iter.next());
                        }
                        for (int i = readTo; i <= last; i += 1) {
                            assertTrue(iter.hasNext());
                            assertNotNull(iter.next());
                            iter.remove();
                        }
                        assertTrue(!iter.hasNext());
                        assertTrue(iter.hasPrevious());
                        assertEquals(readTo - first, map.size());
                    } finally {
                        StoredIterator.close(iter);
                    }

                    /* Restore contents. */
                    for (Map.Entry entry : savedMap.entrySet()) {
                        if (!imap.entrySet().contains(entry)) {
                            imap.put(entry.getKey(), entry.getValue());
                        }
                    }
                    assertEquals(savedMap, map);
                }

                /* Skip N backward, remove all from that point backward. */
                for (int readTo = last - 1; readTo >= first; readTo -= 1) {
                    iter = (ListIterator) iterator(map.keySet());
                    try {
                        while (iter.hasNext()) {
                            assertNotNull(iter.next());
                        }
                        for (int i = last; i > readTo; i -= 1) {
                            assertTrue(iter.hasPrevious());
                            assertNotNull(iter.previous());
                        }
                        for (int i = readTo; i >= first; i -= 1) {
                            assertTrue(iter.hasPrevious());
                            assertNotNull(iter.previous());
                            iter.remove();
                        }
                        assertTrue(!iter.hasPrevious());
                        assertTrue(iter.hasNext());
                        assertEquals(last - readTo, map.size());
                    } finally {
                        StoredIterator.close(iter);
                    }

                    /* Restore contents. */
                    for (Map.Entry entry : savedMap.entrySet()) {
                        if (!imap.entrySet().contains(entry)) {
                            imap.put(entry.getKey(), entry.getValue());
                        }
                    }
                    assertEquals(savedMap, map);
                }
            }
        });
    }

    void iteratorSetAndRemoveNotAllowed(ListIterator i) {

        try {
            i.remove();
            fail();
        } catch (IllegalStateException e) {}

        if (index == null) {
            try {
                Object val = makeVal(1);
                i.set(val);
                fail();
            } catch (IllegalStateException e) {}
        }
    }

    void removeOdd()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                boolean toggle = false;
                for (int i = beginKey; i <= endKey; i += 2) {
                    toggle = !toggle;
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    if (toggle) {
                        assertTrue(map.keySet().contains(key));
                        assertTrue(map.keySet().remove(key));
                        assertTrue(!map.keySet().contains(key));
                    } else {
                        assertTrue(map.containsValue(val));
                        Object oldVal = map.remove(key);
                        assertEquals(oldVal, val);
                        assertTrue(!map.containsKey(key));
                        assertTrue(!map.containsValue(val));
                    }
                    assertNull(map.get(key));
                    assertTrue(!map.duplicates(key).contains(val));
                    checkDupsSize(0, map.duplicates(key));
                }
            }
        });
    }

    void removeOddEntity()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 2) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertTrue(map.values().contains(val));
                    assertTrue(map.values().remove(val));
                    assertTrue(!map.values().contains(val));
                    assertNull(map.get(key));
                    assertTrue(!map.duplicates(key).contains(val));
                    checkDupsSize(0, map.duplicates(key));
                }
            }
        });
    }

    void removeOddEntry()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 2) {
                    Long key = makeKey(i);
                    Object val = mapEntry(i);
                    assertTrue(map.entrySet().contains(val));
                    assertTrue(map.entrySet().remove(val));
                    assertTrue(!map.entrySet().contains(val));
                    assertNull(map.get(key));
                }
            }
        });
    }

    void removeOddIter()
        throws Exception {

        writeIterRunner.run(new TransactionWorker() {
            public void doWork() {
                Iterator iter = iterator(map.keySet());
                try {
                    for (int i = beginKey; i <= endKey; i += 1) {
                        assertTrue(iter.hasNext());
                        Long key = (Long) iter.next();
                        assertNotNull(key);
                        if (map instanceof SortedMap) {
                            assertEquals(makeKey(i), key);
                        }
                        if ((key.intValue() & 1) != 0) {
                            iter.remove();
                        }
                    }
                } finally {
                    StoredIterator.close(iter);
                }
            }
        });
    }

    void removeOddList()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 2) {
                    // remove by index
                    // (with entity binding, embbeded keys in values are
                    // being changed so we can't use values for comparison)
                    int idx = (i - beginKey) / 2;
                    Object val = makeVal(i);
                    if (!isEntityBinding) {
                        assertTrue(list.contains(val));
                        assertEquals(val, list.get(idx));
                        assertEquals(idx, list.indexOf(val));
                    }
                    assertNotNull(list.get(idx));
                    if (isEntityBinding) {
                        assertNotNull(list.remove(idx));
                    } else {
                        assertTrue(list.contains(val));
                        assertEquals(val, list.remove(idx));
                    }
                    assertTrue(!list.remove(val));
                    assertTrue(!list.contains(val));
                    assertTrue(!val.equals(list.get(idx)));
                }
            }
        });
    }

    void removeOddListValue()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 2) {
                    // for non-entity case remove by value
                    // (with entity binding, embbeded keys in values are
                    // being changed so we can't use values for comparison)
                    int idx = (i - beginKey) / 2;
                    Object val = makeVal(i);
                    assertTrue(list.contains(val));
                    assertEquals(val, list.get(idx));
                    assertEquals(idx, list.indexOf(val));
                    assertTrue(list.remove(val));
                    assertTrue(!list.remove(val));
                    assertTrue(!list.contains(val));
                    assertTrue(!val.equals(list.get(idx)));
                }
            }
        });
    }

    void addOdd()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                // add using Map.put()
                for (int i = beginKey; i <= endKey; i += 2) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertNull(imap.get(key));
                    assertNull(imap.put(key, val));
                    assertEquals(val, imap.get(key));
                    assertTrue(imap.duplicates(key).contains(val));
                    checkDupsSize(1, imap.duplicates(key));
                    if (isEntityBinding) {
                        assertTrue(!imap.values().add(val));
                    }
                    if (!imap.areDuplicatesAllowed()) {
                        assertEquals(val, imap.put(key, val));
                    }
                }
            }
        });
    }

    void addOddEntity()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                // add using Map.values().add()
                for (int i = beginKey; i <= endKey; i += 2) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertNull(imap.get(key));
                    assertTrue(!imap.values().contains(val));
                    assertTrue(imap.values().add(val));
                    assertEquals(val, imap.get(key));
                    assertTrue(imap.values().contains(val));
                    assertTrue(imap.duplicates(key).contains(val));
                    checkDupsSize(1, imap.duplicates(key));
                    if (isEntityBinding) {
                        assertTrue(!imap.values().add(val));
                    }
                }
            }
        });
    }

    void addOddDup()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                // add using Map.duplicates().add()
                for (int i = beginKey; i <= endKey; i += 2) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    assertNull(imap.get(key));
                    assertTrue(!imap.values().contains(val));
                    assertTrue(imap.duplicates(key).add(val));
                    assertEquals(val, imap.get(key));
                    assertTrue(imap.values().contains(val));
                    assertTrue(imap.duplicates(key).contains(val));
                    checkDupsSize(1, imap.duplicates(key));
                    assertTrue(!imap.duplicates(key).add(val));
                    if (isEntityBinding) {
                        assertTrue(!imap.values().add(val));
                    }
                }
            }
        });
    }

    void addOddList()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 2) {
                    int idx = i - beginKey;
                    Object val = makeVal(i);
                    assertTrue(!list.contains(val));
                    assertTrue(!val.equals(list.get(idx)));
                    list.add(idx, val);
                    assertTrue(list.contains(val));
                    assertEquals(val, list.get(idx));
                }
            }
        });
    }

    void addAllList()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 1) {
                    int idx = i - beginKey;
                    Object val = makeVal(i);
                    assertTrue(!list.contains(val));
                    assertTrue(list.add(val));
                    assertTrue(list.contains(val));
                    assertEquals(val, list.get(idx));
                }
            }
        });
    }

    void removeAllList()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                assertTrue(!list.isEmpty());
                list.clear();
                assertTrue(list.isEmpty());
                for (int i = beginKey; i <= endKey; i += 1) {
                    int idx = i - beginKey;
                    assertNull(list.get(idx));
                }
            }
        });
    }

    /**
     * Tests ConcurentMap methods implemented by StordMap.  Starts with an
     * empty DB and ends with an empty DB.  [#16218]
     */
    void testConcurrentMap()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = makeKey(i);
                    Object val = makeVal(i);
                    Object valPlusOne = makeVal(i, i + 1);
                    assertFalse(imap.containsKey(key));

                    assertNull(imap.putIfAbsent(key, val));
                    assertEquals(val, imap.get(key));

                    assertEquals(val, imap.putIfAbsent(key, val));
                    assertEquals(val, imap.get(key));

                    if (!imap.areDuplicatesAllowed()) {
                        assertEquals(val, imap.replace(key, valPlusOne));
                        assertEquals(valPlusOne, imap.get(key));

                        assertEquals(valPlusOne, imap.replace(key, val));
                        assertEquals(val, imap.get(key));

                        assertFalse(imap.replace(key, valPlusOne, val));
                        assertEquals(val, imap.get(key));

                        assertTrue(imap.replace(key, val, valPlusOne));
                        assertEquals(valPlusOne, imap.get(key));

                        assertTrue(imap.replace(key, valPlusOne, val));
                        assertEquals(val, imap.get(key));
                    }

                    assertFalse(imap.remove(key, valPlusOne));
                    assertTrue(imap.containsKey(key));

                    assertTrue(imap.remove(key, val));
                    assertFalse(imap.containsKey(key));

                    assertNull(imap.replace(key, val));
                    assertFalse(imap.containsKey(key));
                }
            }
        });
    }

    void testIterAddList()
        throws Exception {

        writeIterRunner.run(new TransactionWorker() {
            public void doWork() {
                ListIterator i = (ListIterator) iterator(list);
                try {
                    assertTrue(!i.hasNext());
                    i.add(makeVal(3));
                    assertTrue(!i.hasNext());
                    assertTrue(i.hasPrevious());
                    assertEquals(3, intVal(i.previous()));

                    i.add(makeVal(1));
                    assertTrue(i.hasPrevious());
                    assertTrue(i.hasNext());
                    assertEquals(1, intVal(i.previous()));
                    assertTrue(i.hasNext());
                    assertEquals(1, intVal(i.next()));
                    assertTrue(i.hasNext());
                    assertEquals(3, intVal(i.next()));
                    assertEquals(3, intVal(i.previous()));

                    assertTrue(i.hasNext());
                    i.add(makeVal(2));
                    assertTrue(i.hasNext());
                    assertTrue(i.hasPrevious());
                    assertEquals(2, intVal(i.previous()));
                    assertTrue(i.hasNext());
                    assertEquals(2, intVal(i.next()));
                    assertTrue(i.hasNext());
                    assertEquals(3, intVal(i.next()));

                    assertTrue(!i.hasNext());
                    i.add(makeVal(4));
                    i.add(makeVal(5));
                    assertTrue(!i.hasNext());
                    assertEquals(5, intVal(i.previous()));
                    assertEquals(4, intVal(i.previous()));
                    assertEquals(3, intVal(i.previous()));
                    assertEquals(2, intVal(i.previous()));
                    assertEquals(1, intVal(i.previous()));
                    assertTrue(!i.hasPrevious());
                } finally {
                    StoredIterator.close(i);
                }
            }
        });
    }

    void testIterAddDuplicates()
        throws Exception {

        writeIterRunner.run(new TransactionWorker() {
            public void doWork() {
                assertNull(imap.put(makeKey(1), makeVal(1)));
                ListIterator i =
                    (ListIterator) iterator(imap.duplicates(makeKey(1)));
                try {
                    if (imap.areDuplicatesOrdered()) {
                        i.add(makeVal(1, 4));
                        i.add(makeVal(1, 2));
                        i.add(makeVal(1, 3));
                        while (i.hasPrevious()) i.previous();
                        assertEquals(1, intVal(i.next()));
                        assertEquals(2, intVal(i.next()));
                        assertEquals(3, intVal(i.next()));
                        assertEquals(4, intVal(i.next()));
                        assertTrue(!i.hasNext());
                    } else {
                        assertEquals(1, intVal(i.next()));
                        i.add(makeVal(1, 2));
                        i.add(makeVal(1, 3));
                        assertTrue(!i.hasNext());
                        assertTrue(i.hasPrevious());
                        assertEquals(3, intVal(i.previous()));
                        assertEquals(2, intVal(i.previous()));
                        assertEquals(1, intVal(i.previous()));
                        assertTrue(!i.hasPrevious());
                        i.add(makeVal(1, 4));
                        i.add(makeVal(1, 5));
                        assertTrue(i.hasNext());
                        assertEquals(5, intVal(i.previous()));
                        assertEquals(4, intVal(i.previous()));
                        assertTrue(!i.hasPrevious());
                        assertEquals(4, intVal(i.next()));
                        assertEquals(5, intVal(i.next()));
                        assertEquals(1, intVal(i.next()));
                        assertEquals(2, intVal(i.next()));
                        assertEquals(3, intVal(i.next()));
                        assertTrue(!i.hasNext());
                    }
                } finally {
                    StoredIterator.close(i);
                }
            }
        });
    }

    void readAll()
        throws Exception {

        readRunner.run(new TransactionWorker() {
            public void doWork() {
                // map

                assertNotNull(map.toString());
                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = makeKey(i);
                    Object val = map.get(key);
                    assertEquals(makeVal(i), val);
                    assertTrue(map.containsKey(key));
                    assertTrue(map.containsValue(val));
                    assertTrue(map.keySet().contains(key));
                    assertTrue(map.values().contains(val));
                    assertTrue(map.duplicates(key).contains(val));
                    checkDupsSize(1, map.duplicates(key));
                }
                assertNull(map.get(makeKey(-1)));
                assertNull(map.get(makeKey(0)));
                assertNull(map.get(makeKey(beginKey - 1)));
                assertNull(map.get(makeKey(endKey + 1)));
                checkDupsSize(0, map.duplicates(makeKey(-1)));
                checkDupsSize(0, map.duplicates(makeKey(0)));
                checkDupsSize(0, map.duplicates(makeKey(beginKey - 1)));
                checkDupsSize(0, map.duplicates(makeKey(endKey + 1)));

                // entrySet

                Set set = map.entrySet();
                assertNotNull(set.toString());
                assertEquals(beginKey > endKey, set.isEmpty());
                Iterator iter = iterator(set);
                try {
                    for (int i = beginKey; i <= endKey; i += 1) {
                        assertTrue(iter.hasNext());
                        Map.Entry entry = (Map.Entry) iter.next();
                        Long key = (Long) entry.getKey();
                        Object val = entry.getValue();
                        if (map instanceof SortedMap) {
                            assertEquals(intKey(key), i);
                        }
                        assertEquals(intKey(key), intVal(val));
                        assertTrue(set.contains(entry));
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }
                Map.Entry[] entries =
                    (Map.Entry[]) set.toArray(new Map.Entry[0]);
                assertNotNull(entries);
                assertEquals(endKey - beginKey + 1, entries.length);
                for (int i = beginKey; i <= endKey; i += 1) {
                    Map.Entry entry = entries[i - beginKey];
                    assertNotNull(entry);
                    if (map instanceof SortedMap) {
                        assertEquals(makeKey(i), entry.getKey());
                        assertEquals(makeVal(i), entry.getValue());
                    }
                }
                readIterator(set, iterator(set), beginKey, endKey);
                if (smap != null) {
                    SortedSet sset = (SortedSet) set;
                    if (beginKey == 1 && endKey >= 1) {
                        readIterator(sset,
                                     iterator(sset.subSet(mapEntry(1),
                                                          mapEntry(2))),
                                     1, 1);
                    }
                    if (beginKey <= 2 && endKey >= 2) {
                        readIterator(sset,
                                     iterator(sset.subSet(mapEntry(2),
                                                          mapEntry(3))),
                                     2, 2);
                    }
                    if (beginKey <= endKey) {
                        readIterator(sset,
                                     iterator(sset.subSet
                                                (mapEntry(endKey),
                                                 mapEntry(endKey + 1))),
                                     endKey, endKey);
                    }
                    if (isSubMap()) {
                        if (beginKey <= endKey) {
                            if (rangeType != TAIL) {
                                try {
                                    sset.subSet(mapEntry(endKey + 1),
                                                mapEntry(endKey + 2));
                                    fail();
                                } catch (IllegalArgumentException e) {}
                            }
                            if (rangeType != HEAD) {
                                try {
                                    sset.subSet(mapEntry(0),
                                                mapEntry(1));
                                    fail();
                                } catch (IllegalArgumentException e) {}
                            }
                        }
                    } else {
                        readIterator(sset,
                                     iterator(sset.subSet
                                                (mapEntry(endKey + 1),
                                                 mapEntry(endKey + 2))),
                                     endKey, endKey - 1);
                        readIterator(sset,
                                     iterator(sset.subSet(mapEntry(0),
                                                          mapEntry(1))),
                                     0, -1);
                    }
                }

                // keySet

                set = map.keySet();
                assertNotNull(set.toString());
                assertEquals(beginKey > endKey, set.isEmpty());
                iter = iterator(set);
                try {
                    for (int i = beginKey; i <= endKey; i += 1) {
                        assertTrue(iter.hasNext());
                        Long key = (Long) iter.next();
                        assertTrue(set.contains(key));
                        Object val = map.get(key);
                        if (map instanceof SortedMap) {
                            assertEquals(key, makeKey(i));
                        }
                        assertEquals(intKey(key), intVal(val));
                    }
                    assertTrue("" + beginKey + ' ' + endKey, !iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }
                Long[] keys = (Long[]) set.toArray(new Long[0]);
                assertNotNull(keys);
                assertEquals(endKey - beginKey + 1, keys.length);
                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = keys[i - beginKey];
                    assertNotNull(key);
                    if (map instanceof SortedMap) {
                        assertEquals(makeKey(i), key);
                    }
                }
                readIterator(set, iterator(set), beginKey, endKey);

                // values

                Collection coll = map.values();
                assertNotNull(coll.toString());
                assertEquals(beginKey > endKey, coll.isEmpty());
                iter = iterator(coll);
                try {
                    for (int i = beginKey; i <= endKey; i += 1) {
                        assertTrue(iter.hasNext());
                        Object val = iter.next();
                        if (map instanceof SortedMap) {
                            assertEquals(makeVal(i), val);
                        }
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }
                Object[] values = coll.toArray();
                assertNotNull(values);
                assertEquals(endKey - beginKey + 1, values.length);
                for (int i = beginKey; i <= endKey; i += 1) {
                    Object val = values[i - beginKey];
                    assertNotNull(val);
                    if (map instanceof SortedMap) {
                        assertEquals(makeVal(i), val);
                    }
                }
                readIterator(coll, iterator(coll), beginKey, endKey);

                // list

                if (list != null) {
                    assertNotNull(list.toString());
                    assertEquals(beginKey > endKey, list.isEmpty());
                    for (int i = beginKey; i <= endKey; i += 1) {
                        int idx = i - beginKey;
                        Object val = list.get(idx);
                        assertEquals(makeVal(i), val);
                        assertTrue(list.contains(val));
                        assertEquals(idx, list.indexOf(val));
                        assertEquals(idx, list.lastIndexOf(val));
                    }
                    ListIterator li = (ListIterator) iterator(list);
                    try {
                        for (int i = beginKey; i <= endKey; i += 1) {
                            int idx = i - beginKey;
                            assertTrue(li.hasNext());
                            assertEquals(idx, li.nextIndex());
                            Object val = li.next();
                            assertEquals(makeVal(i), val);
                            assertEquals(idx, li.previousIndex());
                        }
                        assertTrue(!li.hasNext());
                    } finally {
                        StoredIterator.close(li);
                    }
                    if (beginKey < endKey) {
                        li = list.listIterator(1);
                        try {
                            for (int i = beginKey + 1; i <= endKey; i += 1) {
                                int idx = i - beginKey;
                                assertTrue(li.hasNext());
                                assertEquals(idx, li.nextIndex());
                                Object val = li.next();
                                assertEquals(makeVal(i), val);
                                assertEquals(idx, li.previousIndex());
                            }
                            assertTrue(!li.hasNext());
                        } finally {
                            StoredIterator.close(li);
                        }
                    }
                    values = list.toArray();
                    assertNotNull(values);
                    assertEquals(endKey - beginKey + 1, values.length);
                    for (int i = beginKey; i <= endKey; i += 1) {
                        Object val = values[i - beginKey];
                        assertNotNull(val);
                        assertEquals(makeVal(i), val);
                    }
                    readIterator(list, iterator(list), beginKey, endKey);
                }

                // first/last

                if (smap != null) {
                    if (beginKey <= endKey &&
                        beginKey >= 1 && beginKey <= maxKey) {
                        assertEquals(makeKey(beginKey),
                                     smap.firstKey());
                        assertEquals(makeKey(beginKey),
                                     ((SortedSet) smap.keySet()).first());
                        Object entry = ((SortedSet) smap.entrySet()).first();
                        assertEquals(makeKey(beginKey),
                                     ((Map.Entry) entry).getKey());
                        if (smap.values() instanceof SortedSet) {
                            assertEquals(makeVal(beginKey),
                                         ((SortedSet) smap.values()).first());
                        }
                    } else {
                        assertNull(smap.firstKey());
                        assertNull(((SortedSet) smap.keySet()).first());
                        assertNull(((SortedSet) smap.entrySet()).first());
                        if (smap.values() instanceof SortedSet) {
                            assertNull(((SortedSet) smap.values()).first());
                        }
                    }
                    if (beginKey <= endKey &&
                        endKey >= 1 && endKey <= maxKey) {
                        assertEquals(makeKey(endKey),
                                     smap.lastKey());
                        assertEquals(makeKey(endKey),
                                     ((SortedSet) smap.keySet()).last());
                        Object entry = ((SortedSet) smap.entrySet()).last();
                        assertEquals(makeKey(endKey),
                                     ((Map.Entry) entry).getKey());
                        if (smap.values() instanceof SortedSet) {
                            assertEquals(makeVal(endKey),
                                         ((SortedSet) smap.values()).last());
                        }
                    } else {
                        assertNull(smap.lastKey());
                        assertNull(((SortedSet) smap.keySet()).last());
                        assertNull(((SortedSet) smap.entrySet()).last());
                        if (smap.values() instanceof SortedSet) {
                            assertNull(((SortedSet) smap.values()).last());
                        }
                    }
                }
            }
        });
    }

    void readEven()
        throws Exception {

        readRunner.run(new TransactionWorker() {
            public void doWork() {
                int readBegin = ((beginKey & 1) != 0) ?
                                    (beginKey + 1) : beginKey;
                int readEnd = ((endKey & 1) != 0) ?  (endKey - 1) : endKey;
                int readIncr = 2;

                // map

                for (int i = beginKey; i <= endKey; i += 1) {
                    Long key = makeKey(i);
                    if ((i & 1) == 0) {
                        Object val = map.get(key);
                        assertEquals(makeVal(i), val);
                        assertTrue(map.containsKey(key));
                        assertTrue(map.containsValue(val));
                        assertTrue(map.keySet().contains(key));
                        assertTrue(map.values().contains(val));
                        assertTrue(map.duplicates(key).contains(val));
                        checkDupsSize(1, map.duplicates(key));
                    } else {
                        Object val = makeVal(i);
                        assertTrue(!map.containsKey(key));
                        assertTrue(!map.containsValue(val));
                        assertTrue(!map.keySet().contains(key));
                        assertTrue(!map.values().contains(val));
                        assertTrue(!map.duplicates(key).contains(val));
                        checkDupsSize(0, map.duplicates(key));
                    }
                }

                // entrySet

                Set set = map.entrySet();
                assertEquals(beginKey > endKey, set.isEmpty());
                Iterator iter = iterator(set);
                try {
                    for (int i = readBegin; i <= readEnd; i += readIncr) {
                        assertTrue(iter.hasNext());
                        Map.Entry entry = (Map.Entry) iter.next();
                        Long key = (Long) entry.getKey();
                        Object val = entry.getValue();
                        if (map instanceof SortedMap) {
                            assertEquals(intKey(key), i);
                        }
                        assertEquals(intKey(key), intVal(val));
                        assertTrue(set.contains(entry));
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }

                // keySet

                set = map.keySet();
                assertEquals(beginKey > endKey, set.isEmpty());
                iter = iterator(set);
                try {
                    for (int i = readBegin; i <= readEnd; i += readIncr) {
                        assertTrue(iter.hasNext());
                        Long key = (Long) iter.next();
                        assertTrue(set.contains(key));
                        Object val = map.get(key);
                        if (map instanceof SortedMap) {
                            assertEquals(key, makeKey(i));
                        }
                        assertEquals(intKey(key), intVal(val));
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }

                // values

                Collection coll = map.values();
                assertEquals(beginKey > endKey, coll.isEmpty());
                iter = iterator(coll);
                try {
                    for (int i = readBegin; i <= readEnd; i += readIncr) {
                        assertTrue(iter.hasNext());
                        Object val = iter.next();
                        if (map instanceof SortedMap) {
                            assertEquals(makeVal(i), val);
                        }
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }

                // list not used since keys may not be renumbered for this
                // method to work in general

                // first/last

                if (smap != null) {
                    if (readBegin <= readEnd &&
                        readBegin >= 1 && readBegin <= maxKey) {
                        assertEquals(makeKey(readBegin),
                                     smap.firstKey());
                        assertEquals(makeKey(readBegin),
                                     ((SortedSet) smap.keySet()).first());
                        Object entry = ((SortedSet) smap.entrySet()).first();
                        assertEquals(makeKey(readBegin),
                                     ((Map.Entry) entry).getKey());
                        if (smap.values() instanceof SortedSet) {
                            assertEquals(makeVal(readBegin),
                                         ((SortedSet) smap.values()).first());
                        }
                    } else {
                        assertNull(smap.firstKey());
                        assertNull(((SortedSet) smap.keySet()).first());
                        assertNull(((SortedSet) smap.entrySet()).first());
                        if (smap.values() instanceof SortedSet) {
                            assertNull(((SortedSet) smap.values()).first());
                        }
                    }
                    if (readBegin <= readEnd &&
                        readEnd >= 1 && readEnd <= maxKey) {
                        assertEquals(makeKey(readEnd),
                                     smap.lastKey());
                        assertEquals(makeKey(readEnd),
                                     ((SortedSet) smap.keySet()).last());
                        Object entry = ((SortedSet) smap.entrySet()).last();
                        assertEquals(makeKey(readEnd),
                                     ((Map.Entry) entry).getKey());
                        if (smap.values() instanceof SortedSet) {
                            assertEquals(makeVal(readEnd),
                                         ((SortedSet) smap.values()).last());
                        }
                    } else {
                        assertNull(smap.lastKey());
                        assertNull(((SortedSet) smap.keySet()).last());
                        assertNull(((SortedSet) smap.entrySet()).last());
                        if (smap.values() instanceof SortedSet) {
                            assertNull(((SortedSet) smap.values()).last());
                        }
                    }
                }
            }
        });
    }

    void readEvenList()
        throws Exception {

        readRunner.run(new TransactionWorker() {
            public void doWork() {
                int readBegin = ((beginKey & 1) != 0) ?
                                    (beginKey + 1) : beginKey;
                int readEnd = ((endKey & 1) != 0) ?  (endKey - 1) : endKey;
                int readIncr = 2;

                assertEquals(beginKey > endKey, list.isEmpty());
                ListIterator iter = (ListIterator) iterator(list);
                try {
                    int idx = 0;
                    for (int i = readBegin; i <= readEnd; i += readIncr) {
                        assertTrue(iter.hasNext());
                        assertEquals(idx, iter.nextIndex());
                        Object val = iter.next();
                        assertEquals(idx, iter.previousIndex());
                        if (isEntityBinding) {
                            assertEquals(i, intVal(val));
                        } else {
                            assertEquals(makeVal(i), val);
                        }
                        idx += 1;
                    }
                    assertTrue(!iter.hasNext());
                } finally {
                    StoredIterator.close(iter);
                }
            }
        });
    }

    void readIterator(Collection coll, Iterator iter,
                      int beginValue, int endValue) {

        ListIterator li = (ListIterator) iter;
        boolean isList = (coll instanceof List);
        Iterator clone = null;
        try {
            // at beginning
            assertTrue(!li.hasPrevious());
            assertTrue(!li.hasPrevious());
            try { li.previous(); } catch (NoSuchElementException e) {}
            if (isList) {
                assertEquals(-1, li.previousIndex());
            }
            if (endValue < beginValue) {
                // is empty
                assertTrue(!iter.hasNext());
                try { iter.next(); } catch (NoSuchElementException e) {}
                if (isList) {
                    assertEquals(Integer.MAX_VALUE, li.nextIndex());
                }
            }
            // loop thru all and collect in array
            int[] values = new int[endValue - beginValue + 1];
            for (int i = beginValue; i <= endValue; i += 1) {
                assertTrue(iter.hasNext());
                int idx = i - beginKey;
                if (isList) {
                    assertEquals(idx, li.nextIndex());
                }
                int value = intIter(coll, iter.next());
                if (isList) {
                    assertEquals(idx, li.previousIndex());
                }
                values[i - beginValue] = value;
                if (((StoredCollection) coll).isOrdered()) {
                    assertEquals(i, value);
                } else {
                    assertTrue(value >= beginValue);
                    assertTrue(value <= endValue);
                }
            }
            // at end
            assertTrue(!iter.hasNext());
            try { iter.next(); } catch (NoSuchElementException e) {}
            if (isList) {
                assertEquals(Integer.MAX_VALUE, li.nextIndex());
            }
            // clone at same position
            clone = StoredCollections.iterator(iter);
            assertTrue(!clone.hasNext());
            // loop thru in reverse
            for (int i = endValue; i >= beginValue; i -= 1) {
                assertTrue(li.hasPrevious());
                int idx = i - beginKey;
                if (isList) {
                    assertEquals(idx, li.previousIndex());
                }
                int value = intIter(coll, li.previous());
                if (isList) {
                    assertEquals(idx, li.nextIndex());
                }
                assertEquals(values[i - beginValue], value);
            }
            // clone should not have changed
            assertTrue(!clone.hasNext());
            // at beginning
            assertTrue(!li.hasPrevious());
            try { li.previous(); } catch (NoSuchElementException e) {}
            if (isList) {
                assertEquals(-1, li.previousIndex());
            }
            // loop thru with some back-and-forth
            for (int i = beginValue; i <= endValue; i += 1) {
                assertTrue(iter.hasNext());
                int idx = i - beginKey;
                if (isList) {
                    assertEquals(idx, li.nextIndex());
                }
                Object obj = iter.next();
                if (isList) {
                    assertEquals(idx, li.previousIndex());
                }
                assertEquals(obj, li.previous());
                if (isList) {
                    assertEquals(idx, li.nextIndex());
                }
                assertEquals(obj, iter.next());
                if (isList) {
                    assertEquals(idx, li.previousIndex());
                }
                int value = intIter(coll, obj);
                assertEquals(values[i - beginValue], value);
            }
            // at end
            assertTrue(!iter.hasNext());
            try { iter.next(); } catch (NoSuchElementException e) {}
            if (isList) {
                assertEquals(Integer.MAX_VALUE, li.nextIndex());
            }
        } finally {
            StoredIterator.close(iter);
            StoredIterator.close(clone);
        }
    }

    void bulkOperations()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                HashMap hmap = new HashMap();
                for (int i = Math.max(1, beginKey);
                         i <= Math.min(maxKey, endKey);
                         i += 1) {
                    hmap.put(makeKey(i), makeVal(i));
                }
                assertEquals(hmap, map);
                assertEquals(hmap.entrySet(), map.entrySet());
                assertEquals(hmap.keySet(), map.keySet());
                assertEquals(map.values(), hmap.values());

                assertTrue(map.entrySet().containsAll(hmap.entrySet()));
                assertTrue(map.keySet().containsAll(hmap.keySet()));
                assertTrue(map.values().containsAll(hmap.values()));

                map.clear();
                assertTrue(map.isEmpty());
                imap.putAll(hmap);
                assertEquals(hmap, map);

                assertTrue(map.entrySet().removeAll(hmap.entrySet()));
                assertTrue(map.entrySet().isEmpty());
                assertTrue(!map.entrySet().removeAll(hmap.entrySet()));
                assertTrue(imap.entrySet().addAll(hmap.entrySet()));
                assertTrue(map.entrySet().containsAll(hmap.entrySet()));
                assertTrue(!imap.entrySet().addAll(hmap.entrySet()));
                assertEquals(hmap, map);

                assertTrue(!map.entrySet().retainAll(hmap.entrySet()));
                assertEquals(hmap, map);
                assertTrue(map.entrySet().retainAll(Collections.EMPTY_SET));
                assertTrue(map.isEmpty());
                imap.putAll(hmap);
                assertEquals(hmap, map);

                assertTrue(map.values().removeAll(hmap.values()));
                assertTrue(map.values().isEmpty());
                assertTrue(!map.values().removeAll(hmap.values()));
                if (isEntityBinding) {
                    assertTrue(imap.values().addAll(hmap.values()));
                    assertTrue(map.values().containsAll(hmap.values()));
                    assertTrue(!imap.values().addAll(hmap.values()));
                } else {
                    imap.putAll(hmap);
                }
                assertEquals(hmap, map);

                assertTrue(!map.values().retainAll(hmap.values()));
                assertEquals(hmap, map);
                assertTrue(map.values().retainAll(Collections.EMPTY_SET));
                assertTrue(map.isEmpty());
                imap.putAll(hmap);
                assertEquals(hmap, map);

                assertTrue(map.keySet().removeAll(hmap.keySet()));
                assertTrue(map.keySet().isEmpty());
                assertTrue(!map.keySet().removeAll(hmap.keySet()));
                assertTrue(imap.keySet().addAll(hmap.keySet()));
                assertTrue(imap.keySet().containsAll(hmap.keySet()));
                if (index != null) {
                    assertTrue(map.keySet().isEmpty());
                }
                assertTrue(!imap.keySet().addAll(hmap.keySet()));
                // restore values to non-null
                imap.keySet().removeAll(hmap.keySet());
                imap.putAll(hmap);
                assertEquals(hmap, map);

                assertTrue(!map.keySet().retainAll(hmap.keySet()));
                assertEquals(hmap, map);
                assertTrue(map.keySet().retainAll(Collections.EMPTY_SET));
                assertTrue(map.isEmpty());
                imap.putAll(hmap);
                assertEquals(hmap, map);
            }
        });
    }

    void bulkListOperations()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() {
                ArrayList alist = new ArrayList();
                for (int i = beginKey; i <= endKey; i += 1) {
                    alist.add(makeVal(i));
                }

                assertEquals(alist, list);
                assertTrue(list.containsAll(alist));

                if (isListAddAllowed()) {
                    list.clear();
                    assertTrue(list.isEmpty());
                    assertTrue(ilist.addAll(alist));
                    assertEquals(alist, list);
                }

                assertTrue(!list.retainAll(alist));
                assertEquals(alist, list);

                if (isListAddAllowed()) {
                    assertTrue(list.retainAll(Collections.EMPTY_SET));
                    assertTrue(list.isEmpty());
                    assertTrue(ilist.addAll(alist));
                    assertEquals(alist, list);
                }

                if (isListAddAllowed() && !isEntityBinding) {
                    // deleting in a renumbered list with entity binding will
                    // change the values dynamically, making it very difficult
                    // to test
                    assertTrue(list.removeAll(alist));
                    assertTrue(list.isEmpty());
                    assertTrue(!list.removeAll(alist));
                    assertTrue(ilist.addAll(alist));
                    assertTrue(list.containsAll(alist));
                    assertEquals(alist, list);
                }

                if (isListAddAllowed() && !isEntityBinding) {
                    // addAll at an index is also very difficult to test with
                    // an entity binding

                    // addAll at first index
                    ilist.addAll(beginKey, alist);
                    assertTrue(list.containsAll(alist));
                    assertEquals(2 * alist.size(), countElements(list));
                    for (int i = beginKey; i <= endKey; i += 1)
                        ilist.remove(beginKey);
                    assertEquals(alist, list);

                    // addAll at last index
                    ilist.addAll(endKey, alist);
                    assertTrue(list.containsAll(alist));
                    assertEquals(2 * alist.size(), countElements(list));
                    for (int i = beginKey; i <= endKey; i += 1)
                        ilist.remove(endKey);
                    assertEquals(alist, list);

                    // addAll in the middle
                    ilist.addAll(endKey - 1, alist);
                    assertTrue(list.containsAll(alist));
                    assertEquals(2 * alist.size(), countElements(list));
                    for (int i = beginKey; i <= endKey; i += 1)
                        ilist.remove(endKey - 1);
                    assertEquals(alist, list);
                }
            }
        });
    }

    void readWriteRange(final int type, final int rangeBegin,
                        final int rangeEnd)
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() throws Exception {
                setRange(type, rangeBegin, rangeEnd);
                createOutOfRange(rangeBegin, rangeEnd);
                if (rangeType != TAIL) {
                    writeOutOfRange(new Long(rangeEnd + 1));
                }
                if (rangeType != HEAD) {
                    writeOutOfRange(new Long(rangeBegin - 1));
                }
                if (rangeBegin <= rangeEnd) {
                    updateAll();
                }
                if (rangeBegin < rangeEnd && !map.areKeysRenumbered()) {
                    bulkOperations();
                    removeIter();
                }
                readAll();
                clearRange();
            }
        });
    }

    void setRange(int type, int rangeBegin, int rangeEnd) {

        rangeType = type;
        saveMap = map;
        saveSMap = smap;
        saveList = list;
        int listBegin = rangeBegin - beginKey;
        boolean canMakeSubList = (list != null && listBegin>= 0);
        if (!canMakeSubList) {
            list = null;
        }
        if (list != null) {
            try {
                list.subList(-1, 0);
                fail();
            } catch (IndexOutOfBoundsException e) { }
        }
        switch (type) {

        case SUB:
            smap = (StoredSortedMap) smap.subMap(makeKey(rangeBegin),
                                                 makeKey(rangeEnd + 1));
            if (canMakeSubList) {
                list = (StoredList) list.subList(listBegin,
                                                 rangeEnd + 1 - beginKey);
            }
            // check for equivalent ranges
            assertEquals(smap,
                        (saveSMap).subMap(
                            makeKey(rangeBegin), true,
                            makeKey(rangeEnd + 1), false));
            assertEquals(smap.entrySet(),
                        ((StoredSortedEntrySet) saveSMap.entrySet()).subSet(
                            mapEntry(rangeBegin), true,
                            mapEntry(rangeEnd + 1), false));
            assertEquals(smap.keySet(),
                        ((StoredSortedKeySet) saveSMap.keySet()).subSet(
                            makeKey(rangeBegin), true,
                            makeKey(rangeEnd + 1), false));
            if (smap.values() instanceof SortedSet) {
                assertEquals(smap.values(),
                            ((StoredSortedValueSet) saveSMap.values()).subSet(
                                makeVal(rangeBegin), true,
                                makeVal(rangeEnd + 1), false));
            }
            break;
        case HEAD:
            smap = (StoredSortedMap) smap.headMap(makeKey(rangeEnd + 1));
            if (canMakeSubList) {
                list = (StoredList) list.subList(0,
                                                 rangeEnd + 1 - beginKey);
            }
            // check for equivalent ranges
            assertEquals(smap,
                        (saveSMap).headMap(
                            makeKey(rangeEnd + 1), false));
            assertEquals(smap.entrySet(),
                        ((StoredSortedEntrySet) saveSMap.entrySet()).headSet(
                            mapEntry(rangeEnd + 1), false));
            assertEquals(smap.keySet(),
                        ((StoredSortedKeySet) saveSMap.keySet()).headSet(
                            makeKey(rangeEnd + 1), false));
            if (smap.values() instanceof SortedSet) {
                assertEquals(smap.values(),
                            ((StoredSortedValueSet) saveSMap.values()).headSet(
                                makeVal(rangeEnd + 1), false));
            }
            break;
        case TAIL:
            smap = (StoredSortedMap) smap.tailMap(makeKey(rangeBegin));
            if (canMakeSubList) {
                list = (StoredList) list.subList(listBegin,
                                                 maxKey + 1 - beginKey);
            }
            // check for equivalent ranges
            assertEquals(smap,
                        (saveSMap).tailMap(
                            makeKey(rangeBegin), true));
            assertEquals(smap.entrySet(),
                        ((StoredSortedEntrySet) saveSMap.entrySet()).tailSet(
                            mapEntry(rangeBegin), true));
            assertEquals(smap.keySet(),
                        ((StoredSortedKeySet) saveSMap.keySet()).tailSet(
                            makeKey(rangeBegin), true));
            if (smap.values() instanceof SortedSet) {
                assertEquals(smap.values(),
                            ((StoredSortedValueSet) saveSMap.values()).tailSet(
                                makeVal(rangeBegin), true));
            }
            break;
        default: throw new RuntimeException();
        }
        map = smap;
        beginKey = rangeBegin;
        if (rangeBegin < 1 || rangeEnd > maxKey) {
            endKey = rangeBegin - 1; // force empty range for readAll()
        } else {
            endKey = rangeEnd;
        }
    }

    void clearRange() {

        rangeType = NONE;
        beginKey = 1;
        endKey = maxKey;
        map = saveMap;
        smap = saveSMap;
        list = saveList;
    }

    void createOutOfRange(int rangeBegin, int rangeEnd) {
        // map

        if (rangeType != TAIL) {
            try {
                smap.subMap(makeKey(rangeBegin), makeKey(rangeEnd + 2));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                smap.headMap(makeKey(rangeEnd + 2));
                fail();
            } catch (IllegalArgumentException e) { }
            checkDupsSize(0, smap.duplicates(makeKey(rangeEnd + 2)));
        }
        if (rangeType != HEAD) {
            try {
                smap.subMap(makeKey(rangeBegin - 1), makeKey(rangeEnd + 1));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                smap.tailMap(makeKey(rangeBegin - 1));
                fail();
            } catch (IllegalArgumentException e) { }
            checkDupsSize(0, smap.duplicates(makeKey(rangeBegin - 1)));
        }

        // keySet

        if (rangeType != TAIL) {
            SortedSet sset = (SortedSet) map.keySet();
            try {
                sset.subSet(makeKey(rangeBegin), makeKey(rangeEnd + 2));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                sset.headSet(makeKey(rangeEnd + 2));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                iterator(sset.subSet(makeKey(rangeEnd + 1),
                                     makeKey(rangeEnd + 2)));
                fail();
            } catch (IllegalArgumentException e) { }
        }
        if (rangeType != HEAD) {
            SortedSet sset = (SortedSet) map.keySet();
            try {
                sset.subSet(makeKey(rangeBegin - 1), makeKey(rangeEnd + 1));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                sset.tailSet(makeKey(rangeBegin - 1));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                iterator(sset.subSet(makeKey(rangeBegin - 1),
                                     makeKey(rangeBegin)));
                fail();
            } catch (IllegalArgumentException e) { }
        }

        // entrySet

        if (rangeType != TAIL) {
            SortedSet sset = (SortedSet) map.entrySet();
            try {
                sset.subSet(mapEntry(rangeBegin), mapEntry(rangeEnd + 2));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                sset.headSet(mapEntry(rangeEnd + 2));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                iterator(sset.subSet(mapEntry(rangeEnd + 1),
                                     mapEntry(rangeEnd + 2)));
                fail();
            } catch (IllegalArgumentException e) { }
        }
        if (rangeType != HEAD) {
            SortedSet sset = (SortedSet) map.entrySet();
            try {
                sset.subSet(mapEntry(rangeBegin - 1), mapEntry(rangeEnd + 1));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                sset.tailSet(mapEntry(rangeBegin - 1));
                fail();
            } catch (IllegalArgumentException e) { }
            try {
                iterator(sset.subSet(mapEntry(rangeBegin - 1),
                                     mapEntry(rangeBegin)));
                fail();
            } catch (IllegalArgumentException e) { }
        }

        // values

        if (map.values() instanceof SortedSet) {
            SortedSet sset = (SortedSet) map.values();
            if (rangeType != TAIL) {
                try {
                    sset.subSet(makeVal(rangeBegin),
                                makeVal(rangeEnd + 2));
                    fail();
                } catch (IllegalArgumentException e) { }
                try {
                    sset.headSet(makeVal(rangeEnd + 2));
                    fail();
                } catch (IllegalArgumentException e) { }
            }
            if (rangeType != HEAD) {
                try {
                    sset.subSet(makeVal(rangeBegin - 1),
                                makeVal(rangeEnd + 1));
                    fail();
                } catch (IllegalArgumentException e) { }
                try {
                    sset.tailSet(makeVal(rangeBegin - 1));
                    fail();
                } catch (IllegalArgumentException e) { }
            }
        }

        // list

        if (list != null) {
            int size = rangeEnd - rangeBegin + 1;
            try {
                list.subList(0, size + 1);
                fail();
            } catch (IndexOutOfBoundsException e) { }
            try {
                list.subList(-1, size);
                fail();
            } catch (IndexOutOfBoundsException e) { }
            try {
                list.subList(2, 1);
                fail();
            } catch (IndexOutOfBoundsException e) { }
            try {
                list.subList(size, size);
                fail();
            } catch (IndexOutOfBoundsException e) { }
        }
    }

    void writeOutOfRange(Long badNewKey) {
        try {
            map.put(badNewKey, makeVal(badNewKey));
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(e.toString(), index == null);
        } catch (UnsupportedOperationException e) {
            assertTrue(index != null);
        }
        try {
            map.keySet().add(badNewKey);
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(index == null);
        } catch (UnsupportedOperationException e) {
            assertTrue(index != null);
        }
        try {
            map.values().add(makeEntity(badNewKey));
            fail();
        } catch (IllegalArgumentException e) {
            assertTrue(isEntityBinding && index == null);
        } catch (UnsupportedOperationException e) {
            assertTrue(!(isEntityBinding && index == null));
        }
        if (list != null) {
            int i = badNewKey.intValue() - beginKey;
            try {
                list.set(i, makeVal(i));
                fail();
            } catch (IndexOutOfBoundsException e) {
                assertTrue(index == null);
            } catch (UnsupportedOperationException e) {
                assertTrue(index != null);
            }
            try {
                list.add(i, makeVal(badNewKey));
                fail();
            } catch (UnsupportedOperationException e) {
            }
        }
    }

    void readWriteDuplicates()
        throws Exception {

        writeRunner.run(new TransactionWorker() {
            public void doWork() throws Exception {
                if (index == null) {
                    readWritePrimaryDuplicates(beginKey);
                    readWritePrimaryDuplicates(beginKey + 1);
                    readWritePrimaryDuplicates(endKey);
                    readWritePrimaryDuplicates(endKey - 1);
                } else {
                    readWriteIndexedDuplicates(beginKey);
                    readWriteIndexedDuplicates(beginKey + 1);
                    readWriteIndexedDuplicates(endKey);
                    readWriteIndexedDuplicates(endKey - 1);
                }
            }
        });
    }

    void readWritePrimaryDuplicates(int i)
        throws Exception {

        Collection dups;
        // make duplicate values
        final Long key = makeKey(i);
        final Object[] values = new Object[5];
        for (int j = 0; j < values.length; j += 1) {
            values[j] = isEntityBinding
                        ? makeEntity(i, i + j)
                        : makeVal(i + j);
        }
        // add duplicates
        outerLoop: for (int writeMode = 0;; writeMode += 1) {
            //System.out.println("write mode " + writeMode);
            switch (writeMode) {
                case 0:
                case 1: {
                    // write with Map.put()
                    for (int j = 1; j < values.length; j += 1) {
                        map.put(key, values[j]);
                    }
                    break;
                }
                case 2: {
                    // write with Map.duplicates().add()
                    dups = map.duplicates(key);
                    for (int j = 1; j < values.length; j += 1) {
                        dups.add(values[j]);
                    }
                    break;
                }
                case 3: {
                    // write with Map.duplicates().iterator().add()
                    writeIterRunner.run(new TransactionWorker() {
                        public void doWork() {
                            Collection dups = map.duplicates(key);
                            Iterator iter = iterator(dups);
                            assertEquals(values[0], iter.next());
                            assertTrue(!iter.hasNext());
                            try {
                                for (int j = 1; j < values.length; j += 1) {
                                    ((ListIterator) iter).add(values[j]);
                                }
                            } finally {
                                StoredIterator.close(iter);
                            }
                        }
                    });
                    break;
                }
                case 4: {
                    // write with Map.values().add()
                    if (!isEntityBinding) {
                        continue;
                    }
                    Collection set = map.values();
                    for (int j = 1; j < values.length; j += 1) {
                        set.add(values[j]);
                    }
                    break;
                }
                default: {
                    break outerLoop;
                }
            }
            checkDupsSize(values.length, map.duplicates(key));
            // read duplicates
            readDuplicates(i, key, values);
            // remove duplicates
            switch (writeMode) {
                case 0: {
                    // remove with Map.remove()
                    checkDupsSize(values.length, map.duplicates(key));
                    map.remove(key); // remove all values
                    checkDupsSize(0, map.duplicates(key));
                    map.put(key, values[0]); // put back original value
                    checkDupsSize(1, map.duplicates(key));
                    break;
                }
                case 1: {
                    // remove with Map.keySet().remove()
                    map.keySet().remove(key); // remove all values
                    map.put(key, values[0]); // put back original value
                    break;
                }
                case 2: {
                    // remove with Map.duplicates().clear()
                    dups = map.duplicates(key);
                    dups.clear(); // remove all values
                    dups.add(values[0]); // put back original value
                    break;
                }
                case 3: {
                    // remove with Map.duplicates().iterator().remove()
                    writeIterRunner.run(new TransactionWorker() {
                        public void doWork() {
                            Collection dups = map.duplicates(key);
                            Iterator iter = iterator(dups);
                            try {
                                for (int j = 0; j < values.length; j += 1) {
                                    assertEquals(values[j], iter.next());
                                    if (j != 0) {
                                        iter.remove();
                                    }
                                }
                            } finally {
                                StoredIterator.close(iter);
                            }
                        }
                    });
                    break;
                }
                case 4: {
                    // remove with Map.values().remove()
                    if (!isEntityBinding) {
                        throw new IllegalStateException();
                    }
                    Collection set = map.values();
                    for (int j = 1; j < values.length; j += 1) {
                        set.remove(values[j]);
                    }
                    break;
                }
                default: throw new IllegalStateException();
            }
            // verify that only original value is present
            dups = map.duplicates(key);
            assertTrue(dups.contains(values[0]));
            for (int j = 1; j < values.length; j += 1) {
                assertTrue(!dups.contains(values[j]));
            }
            checkDupsSize(1, dups);
        }
    }

    void readWriteIndexedDuplicates(int i) {
        Object key = makeKey(i);
        Object[] values = new Object[3];
        values[0] = makeVal(i);
        for (int j = 1; j < values.length; j += 1) {
            values[j] = isEntityBinding
                        ? makeEntity(endKey + j, i)
                        : makeVal(i);
        }
        // add duplicates
        for (int j = 1; j < values.length; j += 1) {
            imap.put(makeKey(endKey + j), values[j]);
        }
        // read duplicates
        readDuplicates(i, key, values);
        // remove duplicates
        for (int j = 1; j < values.length; j += 1) {
            imap.remove(makeKey(endKey + j));
        }
        checkDupsSize(1, map.duplicates(key));
    }

    void readDuplicates(int i, Object key, Object[] values) {

        boolean isOrdered = map.isOrdered();
        Collection dups;
        Iterator iter;
        // read with Map.duplicates().iterator()
        dups = map.duplicates(key);
        checkDupsSize(values.length, dups);
        iter = iterator(dups);
        try {
            for (int j = 0; j < values.length; j += 1) {
                assertTrue(iter.hasNext());
                Object val = iter.next();
                assertEquals(values[j], val);
            }
            assertTrue(!iter.hasNext());
        } finally {
            StoredIterator.close(iter);
        }
        // read with Map.values().iterator()
        Collection clone = ((StoredCollection) map.values()).toList();
        iter = iterator(map.values());
        try {
            for (int j = beginKey; j < i; j += 1) {
                Object val = iter.next();
                assertTrue(clone.remove(makeVal(j)));
                if (isOrdered) {
                    assertEquals(makeVal(j), val);
                }
            }
            for (int j = 0; j < values.length; j += 1) {
                Object val = iter.next();
                assertTrue(clone.remove(values[j]));
                if (isOrdered) {
                    assertEquals(values[j], val);
                }
            }
            for (int j = i + 1; j <= endKey; j += 1) {
                Object val = iter.next();
                assertTrue(clone.remove(makeVal(j)));
                if (isOrdered) {
                    assertEquals(makeVal(j), val);
                }
            }
            assertTrue(!iter.hasNext());
            assertTrue(clone.isEmpty());
        } finally {
            StoredIterator.close(iter);
        }
        // read with Map.entrySet().iterator()
        clone = ((StoredCollection) map.entrySet()).toList();
        iter = iterator(map.entrySet());
        try {
            for (int j = beginKey; j < i; j += 1) {
                Map.Entry entry = (Map.Entry) iter.next();
                assertTrue(clone.remove(mapEntry(j)));
                if (isOrdered) {
                    assertEquals(makeVal(j), entry.getValue());
                    assertEquals(makeKey(j), entry.getKey());
                }
            }
            for (int j = 0; j < values.length; j += 1) {
                Map.Entry entry = (Map.Entry) iter.next();
                assertTrue(clone.remove(mapEntry(makeKey(i), values[j])));
                if (isOrdered) {
                    assertEquals(values[j], entry.getValue());
                    assertEquals(makeKey(i), entry.getKey());
                }
            }
            for (int j = i + 1; j <= endKey; j += 1) {
                Map.Entry entry = (Map.Entry) iter.next();
                assertTrue(clone.remove(mapEntry(j)));
                if (isOrdered) {
                    assertEquals(makeVal(j), entry.getValue());
                    assertEquals(makeKey(j), entry.getKey());
                }
            }
            assertTrue(!iter.hasNext());
            assertTrue(clone.isEmpty());
        } finally {
            StoredIterator.close(iter);
        }
        // read with Map.keySet().iterator()
        clone = ((StoredCollection) map.keySet()).toList();
        iter = iterator(map.keySet());
        try {
            for (int j = beginKey; j < i; j += 1) {
                Object val = iter.next();
                assertTrue(clone.remove(makeKey(j)));
                if (isOrdered) {
                    assertEquals(makeKey(j), val);
                }
            }
            if (true) {
                // only one key is iterated for all duplicates
                Object val = iter.next();
                assertTrue(clone.remove(makeKey(i)));
                if (isOrdered) {
                    assertEquals(makeKey(i), val);
                }
            }
            for (int j = i + 1; j <= endKey; j += 1) {
                Object val = iter.next();
                assertTrue(clone.remove(makeKey(j)));
                if (isOrdered) {
                    assertEquals(makeKey(j), val);
                }
            }
            assertTrue(!iter.hasNext());
            assertTrue(clone.isEmpty());
        } finally {
            StoredIterator.close(iter);
        }
    }

    void duplicatesNotAllowed() {

        Collection dups = map.duplicates(makeKey(beginKey));
        try {
            dups.add(makeVal(beginKey));
            fail();
        } catch (UnsupportedOperationException expected) { }
        ListIterator iter = (ListIterator) iterator(dups);
        try {
            iter.add(makeVal(beginKey));
            fail();
        } catch (UnsupportedOperationException expected) {
        } finally {
            StoredIterator.close(iter);
        }
    }

    void listOperationsNotAllowed() {

        ListIterator iter = (ListIterator) iterator(map.values());
        try {
            try {
                iter.nextIndex();
                fail();
            } catch (UnsupportedOperationException expected) { }
            try {
                iter.previousIndex();
                fail();
            } catch (UnsupportedOperationException expected) { }
        } finally {
            StoredIterator.close(iter);
        }
    }

    void testCdbLocking() {

        Iterator readIterator;
        Iterator writeIterator;
        StoredKeySet set = (StoredKeySet) map.keySet();

        // can open two CDB read cursors
        readIterator = set.storedIterator(false);
        try {
            Iterator readIterator2 = set.storedIterator(false);
            StoredIterator.close(readIterator2);
        } finally {
            StoredIterator.close(readIterator);
        }

        // can open two CDB write cursors
        writeIterator = set.storedIterator(true);
        try {
            Iterator writeIterator2 = set.storedIterator(true);
            StoredIterator.close(writeIterator2);
        } finally {
            StoredIterator.close(writeIterator);
        }

        // cannot open CDB write cursor when read cursor is open,
        readIterator = set.storedIterator(false);
        try {
            writeIterator = set.storedIterator(true);
            fail();
            StoredIterator.close(writeIterator);
        } catch (IllegalStateException e) {
        } finally {
            StoredIterator.close(readIterator);
        }

        if (index == null) {
            // cannot put() with read cursor open
            readIterator = set.storedIterator(false);
            try {
                map.put(makeKey(1), makeVal(1));
                fail();
            } catch (IllegalStateException e) {
            } finally {
                StoredIterator.close(readIterator);
            }

            // cannot append() with write cursor open with RECNO/QUEUE only
            writeIterator = set.storedIterator(true);
            try {
                if (testStore.isQueueOrRecno()) {
                    try {
                        map.append(makeVal(1));
                        fail();
                    } catch (IllegalStateException e) {}
                } else {
                    map.append(makeVal(1));
                }
            } finally {
                StoredIterator.close(writeIterator);
            }
        }
    }

    Object makeVal(int key) {

        if (isEntityBinding) {
            return makeEntity(key);
        } else {
            return new Long(key + 100);
        }
    }

    Object makeVal(int key, int val) {

        if (isEntityBinding) {
            return makeEntity(key, val);
        } else {
            return makeVal(val);
        }
    }

    Object makeEntity(int key, int val) {

        return new TestEntity(key, val + 100);
    }

    int intVal(Object val) {

        if (isEntityBinding) {
            return ((TestEntity) val).value - 100;
        } else {
            return ((Long) val).intValue() - 100;
        }
    }

    int intKey(Object key) {

        return ((Long) key).intValue();
    }

    Object makeVal(Long key) {

        return makeVal(key.intValue());
    }

    Object makeEntity(int key) {

        return makeEntity(key, key);
    }

    Object makeEntity(Long key) {

        return makeEntity(key.intValue());
    }

    int intIter(Collection coll, Object value) {

        if (coll instanceof StoredKeySet) {
            return intKey(value);
        } else {
            if (coll instanceof StoredEntrySet) {
                value = ((Map.Entry) value).getValue();
            }
            return intVal(value);
        }
    }

    Map.Entry mapEntry(Object key, Object val) {

        return new MapEntryParameter(key, val);
    }

    Map.Entry mapEntry(int key) {

        return new MapEntryParameter(makeKey(key), makeVal(key));
    }

    Long makeKey(int key) {

        return new Long(key);
    }

    boolean isSubMap() {

        return rangeType != NONE;
    }

    void checkDupsSize(int expected, Collection coll) {

        assertEquals(expected, coll.size());
        if (coll instanceof StoredCollection) {
            StoredIterator i = ((StoredCollection) coll).storedIterator(false);
            try {
                int actual = 0;
                if (i.hasNext()) {
                    i.next();
                    actual = i.count();
                }
                assertEquals(expected, actual);
            } finally {
                StoredIterator.close(i);
            }
        }
    }

    private boolean isListAddAllowed() {

        return list != null && testStore.isQueueOrRecno() &&
               list.areKeysRenumbered();
    }

    private int countElements(Collection coll) {

        int count = 0;
        Iterator iter = iterator(coll);
        try {
            while (iter.hasNext()) {
                iter.next();
                count += 1;
            }
        } finally {
            StoredIterator.close(iter);
        }
        return count;
    }
}
