/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections.test;

import java.util.ArrayList;
import java.util.List;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.RecordNumberBinding;
import com.sleepycat.collections.CurrentTransaction;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.SecondaryConfig;

/**
 * @author Mark Hayes
 */
class TestStore {

    static final TestKeyCreator BYTE_EXTRACTOR = new TestKeyCreator(false);
    static final TestKeyCreator RECNO_EXTRACTOR = new TestKeyCreator(true);
    static final EntryBinding VALUE_BINDING = new TestDataBinding();
    static final EntryBinding BYTE_KEY_BINDING = VALUE_BINDING;
    static final EntryBinding RECNO_KEY_BINDING = new RecordNumberBinding();
    static final EntityBinding BYTE_ENTITY_BINDING =
            new TestEntityBinding(false);
    static final EntityBinding RECNO_ENTITY_BINDING =
            new TestEntityBinding(true);
    static final TestKeyAssigner BYTE_KEY_ASSIGNER =
            new TestKeyAssigner(false);
    static final TestKeyAssigner RECNO_KEY_ASSIGNER =
            new TestKeyAssigner(true);

    static final TestStore BTREE_UNIQ;
    static final TestStore BTREE_DUP;
    static final TestStore BTREE_DUPSORT;
    static final TestStore BTREE_RECNUM;
    static final TestStore HASH_UNIQ;
    static final TestStore HASH_DUP;
    static final TestStore HASH_DUPSORT;
    static final TestStore QUEUE;
    static final TestStore RECNO;
    static final TestStore RECNO_RENUM;

    static final TestStore[] ALL;
    static {
        List list = new ArrayList();
        SecondaryConfig config;

        config = new SecondaryConfig();
        DbCompat.setTypeBtree(config);
        BTREE_UNIQ = new TestStore("btree-uniq", config);
        BTREE_UNIQ.indexOf = BTREE_UNIQ;
        list.add(BTREE_UNIQ);

        if (DbCompat.INSERTION_ORDERED_DUPLICATES) {
            config = new SecondaryConfig();
            DbCompat.setTypeBtree(config);
            DbCompat.setUnsortedDuplicates(config, true);
            BTREE_DUP = new TestStore("btree-dup", config);
            BTREE_DUP.indexOf = null; // indexes must use sorted dups
            list.add(BTREE_DUP);
        } else {
            BTREE_DUP = null;
        }

        config = new SecondaryConfig();
        DbCompat.setTypeBtree(config);
        DbCompat.setSortedDuplicates(config, true);
        BTREE_DUPSORT = new TestStore("btree-dupsort", config);
        BTREE_DUPSORT.indexOf = BTREE_UNIQ;
        list.add(BTREE_DUPSORT);

        if (DbCompat.BTREE_RECNUM_METHOD) {
            config = new SecondaryConfig();
            DbCompat.setTypeBtree(config);
            DbCompat.setBtreeRecordNumbers(config, true);
            BTREE_RECNUM = new TestStore("btree-recnum", config);
            BTREE_RECNUM.indexOf = BTREE_RECNUM;
            list.add(BTREE_RECNUM);
        } else {
            BTREE_RECNUM = null;
        }

        if (DbCompat.HASH_METHOD) {
            config = new SecondaryConfig();
            DbCompat.setTypeHash(config);
            HASH_UNIQ = new TestStore("hash-uniq", config);
            HASH_UNIQ.indexOf = HASH_UNIQ;
            list.add(HASH_UNIQ);

            if (DbCompat.INSERTION_ORDERED_DUPLICATES) {
                config = new SecondaryConfig();
                DbCompat.setTypeHash(config);
                DbCompat.setUnsortedDuplicates(config, true);
                HASH_DUP = new TestStore("hash-dup", config);
                HASH_DUP.indexOf = null; // indexes must use sorted dups
                list.add(HASH_DUP);
            } else {
                HASH_DUP = null;
            }

            config = new SecondaryConfig();
            DbCompat.setTypeHash(config);
            DbCompat.setSortedDuplicates(config, true);
            HASH_DUPSORT = new TestStore("hash-dupsort", config);
            HASH_DUPSORT.indexOf = HASH_UNIQ;
            list.add(HASH_DUPSORT);
        } else {
            HASH_UNIQ = null;
            HASH_DUP = null;
            HASH_DUPSORT = null;
        }

        if (DbCompat.QUEUE_METHOD) {
            config = new SecondaryConfig();
            DbCompat.setTypeQueue(config);
            QUEUE = new TestStore("queue", config);
            QUEUE.indexOf = QUEUE;
            list.add(QUEUE);
        } else {
            QUEUE = null;
        }

        if (DbCompat.RECNO_METHOD) {
            config = new SecondaryConfig();
            DbCompat.setTypeRecno(config);
            RECNO = new TestStore("recno", config);
            RECNO.indexOf = RECNO;
            list.add(RECNO);

            config = new SecondaryConfig();
            DbCompat.setTypeRecno(config);
            DbCompat.setRenumbering(config, true);
            RECNO_RENUM = new TestStore("recno-renum", config);
            RECNO_RENUM.indexOf = null; // indexes must have stable keys
            list.add(RECNO_RENUM);
        } else {
            RECNO = null;
            RECNO_RENUM = null;
        }

        ALL = new TestStore[list.size()];
        list.toArray(ALL);
    }

    private String name;
    private SecondaryConfig config;
    private TestStore indexOf;
    private boolean isRecNumFormat;

    private TestStore(String name, SecondaryConfig config) {

        this.name = name;
        this.config = config;

        isRecNumFormat = isQueueOrRecno() ||
                            (DbCompat.isTypeBtree(config) &&
                             DbCompat.getBtreeRecordNumbers(config));
    }

    EntryBinding getValueBinding() {

        return VALUE_BINDING;
    }

    EntryBinding getKeyBinding() {

        return isRecNumFormat ? RECNO_KEY_BINDING : BYTE_KEY_BINDING;
    }

    EntityBinding getEntityBinding() {

        return isRecNumFormat ? RECNO_ENTITY_BINDING : BYTE_ENTITY_BINDING;
    }

    TestKeyAssigner getKeyAssigner() {

        if (isQueueOrRecno()) {
            return null;
        } else {
            if (isRecNumFormat) {
                return RECNO_KEY_ASSIGNER;
            } else {
                return BYTE_KEY_ASSIGNER;
            }
        }
    }

    String getName() {

        return name;
    }

    boolean isOrdered() {

        return !DbCompat.isTypeHash(config);
    }

    boolean isQueueOrRecno() {

        return DbCompat.isTypeQueue(config) || DbCompat.isTypeRecno(config);
    }

    boolean areKeyRangesAllowed() {
        return isOrdered() && !isQueueOrRecno();
    }

    boolean areDuplicatesAllowed() {

        return DbCompat.getSortedDuplicates(config) ||
               DbCompat.getUnsortedDuplicates(config);
    }

    boolean hasRecNumAccess() {

        return isRecNumFormat;
    }

    boolean areKeysRenumbered() {

        return hasRecNumAccess() &&
                (DbCompat.isTypeBtree(config) ||
                 DbCompat.getRenumbering(config));
    }

    TestStore getIndexOf() {

        return DbCompat.SECONDARIES ? indexOf : null;
    }

    Database open(Environment env, String fileName)
        throws DatabaseException {

        int fixedLen = (isQueueOrRecno() ? 1 : 0);
        return openDb(env, fileName, fixedLen, null);
    }

    Database openIndex(Database primary, String fileName)
        throws DatabaseException {

        int fixedLen = (isQueueOrRecno() ? 4 : 0);
        config.setKeyCreator(isRecNumFormat ? RECNO_EXTRACTOR
                                            : BYTE_EXTRACTOR);
        Environment env = primary.getEnvironment();
        return openDb(env, fileName, fixedLen, primary);
    }

    private Database openDb(Environment env, String fileName, int fixedLen,
                            Database primary)
        throws DatabaseException {

        if (fixedLen > 0) {
            DbCompat.setRecordLength(config, fixedLen);
            DbCompat.setRecordPad(config, 0);
        } else {
            DbCompat.setRecordLength(config, 0);
        }
        config.setAllowCreate(true);
        DbCompat.setReadUncommitted(config, true);
        config.setTransactional(CurrentTransaction.getInstance(env) != null);
        if (primary != null) {
            return DbCompat.testOpenSecondaryDatabase
                (env, null, fileName, null, primary, config);
        } else {
            return DbCompat.testOpenDatabase
                (env, null, fileName, null, config);
        }
    }
}
