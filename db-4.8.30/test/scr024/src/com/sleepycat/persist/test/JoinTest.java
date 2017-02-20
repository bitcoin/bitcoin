/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.test;

import static com.sleepycat.persist.model.Relationship.MANY_TO_ONE;

import java.util.ArrayList;
import java.util.List;

import junit.framework.Test;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.EntityJoin;
import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.ForwardCursor;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.PrimaryKey;
import com.sleepycat.persist.model.SecondaryKey;
import com.sleepycat.util.test.TxnTestCase;

/**
 * @author Mark Hayes
 */
public class JoinTest extends TxnTestCase {

    private static final int N_RECORDS = 5;

    static protected Class<?> testClass = JoinTest.class;

    public static Test suite() {
        return txnTestSuite(testClass, null, null);
    }

    private EntityStore store;
    private PrimaryIndex<Integer,MyEntity> primary;
    private SecondaryIndex<Integer,Integer,MyEntity> sec1;
    private SecondaryIndex<Integer,Integer,MyEntity> sec2;
    private SecondaryIndex<Integer,Integer,MyEntity> sec3;

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
        sec1 = store.getSecondaryIndex(primary, Integer.class, "k1");
        sec2 = store.getSecondaryIndex(primary, Integer.class, "k2");
        sec3 = store.getSecondaryIndex(primary, Integer.class, "k3");
    }

    /**
     * Closes the store.
     */
    private void close()
        throws DatabaseException {

        store.close();
    }

    public void testJoin()
        throws DatabaseException {

        open();

        /*
         * Primary keys: {   0,   1,   2,   3,   4 }
         * Secondary k1: { 0:0, 0:1, 0:2, 0:3, 0:4 }
         * Secondary k2: { 0:0, 1:1, 0:2, 1:3, 0:4 }
         * Secondary k3: { 0:0, 1:1, 2:2, 0:3, 1:4 }
         */
        Transaction txn = txnBegin();
        for (int i = 0; i < N_RECORDS; i += 1) {
            MyEntity e = new MyEntity(i, 0, i % 2, i % 3);
            boolean ok = primary.putNoOverwrite(txn, e);
            assertTrue(ok);
        }
        txnCommit(txn);

        /*
         * k1, k2, k3, -> { primary keys }
         * -1 means don't include the key in the join.
         */
        doJoin( 0,  0,  0, new int[] { 0 });
        doJoin( 0,  0,  1, new int[] { 4 });
        doJoin( 0,  0, -1, new int[] { 0, 2, 4 });
        doJoin(-1,  1,  1, new int[] { 1 });
        doJoin(-1,  2,  2, new int[] { });
        doJoin(-1, -1,  2, new int[] { 2 });

        close();
    }

    private void doJoin(int k1, int k2, int k3, int[] expectKeys)
        throws DatabaseException {

        List<Integer> expect = new ArrayList<Integer>();
        for (int i : expectKeys) {
            expect.add(i);
        }
        EntityJoin join = new EntityJoin(primary);
        if (k1 >= 0) {
            join.addCondition(sec1, k1);
        }
        if (k2 >= 0) {
            join.addCondition(sec2, k2);
        }
        if (k3 >= 0) {
            join.addCondition(sec3, k3);
        }
        List<Integer> found;
        Transaction txn = txnBegin();

        /* Keys */
        found = new ArrayList<Integer>();
        ForwardCursor<Integer> keys = join.keys(txn, null);
        for (int i : keys) {
            found.add(i);
        }
        keys.close();
        assertEquals(expect, found);

        /* Entities */
        found = new ArrayList<Integer>();
        ForwardCursor<MyEntity> entities = join.entities(txn, null);
        for (MyEntity e : entities) {
            found.add(e.id);
        }
        entities.close();
        assertEquals(expect, found);

        txnCommit(txn);
    }

    @Entity
    private static class MyEntity {
        @PrimaryKey
        int id;
        @SecondaryKey(relate=MANY_TO_ONE)
        int k1;
        @SecondaryKey(relate=MANY_TO_ONE)
        int k2;
        @SecondaryKey(relate=MANY_TO_ONE)
        int k3;

        private MyEntity() {}

        MyEntity(int id, int k1, int k2, int k3) {
            this.id = id;
            this.k1 = k1;
            this.k2 = k2;
            this.k3 = k3;
        }

        @Override
        public String toString() {
            return "MyEntity " + id + ' ' + k1 + ' ' + k2 + ' ' + k3;
        }
    }
}
