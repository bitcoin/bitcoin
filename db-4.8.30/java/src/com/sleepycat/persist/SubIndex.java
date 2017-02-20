/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import java.util.Map;
import java.util.SortedMap;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.collections.StoredSortedMap;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.SecondaryCursor;
import com.sleepycat.db.SecondaryDatabase;
import com.sleepycat.db.Transaction;
import com.sleepycat.util.keyrange.KeyRange;
import com.sleepycat.util.keyrange.RangeCursor;

/**
 * The EntityIndex returned by SecondaryIndex.subIndex.  A SubIndex, in JE
 * internal terms, is a duplicates btree for a single key in the main btree.
 * From the user's viewpoint, the keys are primary keys.  This class implements
 * that viewpoint.  In general, getSearchBoth and getSearchBothRange are used
 * where in a normal index getSearchKey and getSearchRange would be used.  The
 * main tree key is always implied, not passed as a parameter.
 *
 * @author Mark Hayes
 */
class SubIndex<PK,E> implements EntityIndex<PK,E> {

    private SecondaryIndex<?,PK,E> secIndex;
    private SecondaryDatabase db;
    private boolean transactional;
    private boolean sortedDups;
    private boolean locking;
    private DatabaseEntry keyEntry;
    private Object keyObject;
    private KeyRange singleKeyRange;
    private EntryBinding pkeyBinding;
    private KeyRange emptyPKeyRange;
    private EntityBinding entityBinding;
    private ValueAdapter<PK> keyAdapter;
    private ValueAdapter<E> entityAdapter;
    private SortedMap<PK,E> map;

    <SK> SubIndex(SecondaryIndex<SK,PK,E> secIndex,
                  EntityBinding entityBinding,
                  SK key)
        throws DatabaseException {

        this.secIndex = secIndex;
        db = secIndex.getDatabase();
        transactional = secIndex.transactional;
        sortedDups = secIndex.sortedDups;
        locking =
            DbCompat.getInitializeLocking(db.getEnvironment().getConfig());

        keyObject = key;
        keyEntry = new DatabaseEntry();
        secIndex.keyBinding.objectToEntry(key, keyEntry);
        singleKeyRange = secIndex.emptyRange.subRange(keyEntry);

        PrimaryIndex<PK,E> priIndex = secIndex.getPrimaryIndex();
        pkeyBinding = priIndex.keyBinding;
        emptyPKeyRange = priIndex.emptyRange;
        this.entityBinding = entityBinding;

        keyAdapter = new PrimaryKeyValueAdapter<PK>
            (priIndex.keyClass, priIndex.keyBinding);
        entityAdapter = secIndex.entityAdapter;
    }

    public boolean contains(PK key)
        throws DatabaseException {

        return contains(null, key, null);
    }

    public boolean contains(Transaction txn, PK key, LockMode lockMode)
        throws DatabaseException {

        DatabaseEntry pkeyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = BasicIndex.NO_RETURN_ENTRY;
        pkeyBinding.objectToEntry(key, pkeyEntry);

        OperationStatus status =
            db.getSearchBoth(txn, keyEntry, pkeyEntry, dataEntry, lockMode);
        return (status == OperationStatus.SUCCESS);
    }

    public E get(PK key)
        throws DatabaseException {

        return get(null, key, null);
    }

    public E get(Transaction txn, PK key, LockMode lockMode)
        throws DatabaseException {

        DatabaseEntry pkeyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = new DatabaseEntry();
        pkeyBinding.objectToEntry(key, pkeyEntry);

        OperationStatus status =
            db.getSearchBoth(txn, keyEntry, pkeyEntry, dataEntry, lockMode);

        if (status == OperationStatus.SUCCESS) {
            return (E) entityBinding.entryToObject(pkeyEntry, dataEntry);
        } else {
            return null;
        }
    }

    public long count()
        throws DatabaseException {

        CursorConfig cursorConfig = locking ?
            CursorConfig.READ_UNCOMMITTED : null;
        EntityCursor<PK> cursor = keys(null, cursorConfig);
        try {
            if (cursor.next() != null) {
                return cursor.count();
            } else {
                return 0;
            }
        } finally {
            cursor.close();
        }
    }

    public boolean delete(PK key)
        throws DatabaseException {

        return delete(null, key);
    }

    public boolean delete(Transaction txn, PK key)
        throws DatabaseException {

        DatabaseEntry pkeyEntry = new DatabaseEntry();
        DatabaseEntry dataEntry = BasicIndex.NO_RETURN_ENTRY;
        pkeyBinding.objectToEntry(key, pkeyEntry);

        boolean autoCommit = false;
	Environment env = db.getEnvironment();
        if (transactional &&
	    txn == null &&
	    DbCompat.getThreadTransaction(env) == null) {
            txn = env.beginTransaction(null, null);
            autoCommit = true;
        }

        boolean failed = true;
        OperationStatus status;
        SecondaryCursor cursor = db.openSecondaryCursor(txn, null);
        try {
            status = cursor.getSearchBoth
                (keyEntry, pkeyEntry, dataEntry,
                 locking ? LockMode.RMW : null);
            if (status == OperationStatus.SUCCESS) {
                status = cursor.delete();
            }
            failed = false;
        } finally {
            cursor.close();
            if (autoCommit) {
                if (failed) {
                    txn.abort();
                } else {
                    txn.commit();
                }
            }
        }

        return (status == OperationStatus.SUCCESS);
    }

    public EntityCursor<PK> keys()
        throws DatabaseException {

        return keys(null, null);
    }

    public EntityCursor<PK> keys(Transaction txn, CursorConfig config)
        throws DatabaseException {

        return cursor(txn, null, keyAdapter, config);
    }

    public EntityCursor<E> entities()
        throws DatabaseException {

        return cursor(null, null, entityAdapter, null);
    }

    public EntityCursor<E> entities(Transaction txn,
                                    CursorConfig config)
        throws DatabaseException {

        return cursor(txn, null, entityAdapter, config);
    }

    public EntityCursor<PK> keys(PK fromKey,
                                 boolean fromInclusive,
                                 PK toKey,
                                 boolean toInclusive)
        throws DatabaseException {

        return cursor(null, fromKey, fromInclusive, toKey, toInclusive,
                      keyAdapter, null);
    }

    public EntityCursor<PK> keys(Transaction txn,
                                 PK fromKey,
                                 boolean fromInclusive,
                                 PK toKey,
                                 boolean toInclusive,
                                 CursorConfig config)
        throws DatabaseException {

        return cursor(txn, fromKey, fromInclusive, toKey, toInclusive,
                      keyAdapter, config);
    }

    public EntityCursor<E> entities(PK fromKey,
                                    boolean fromInclusive,
                                    PK toKey,
                                    boolean toInclusive)
        throws DatabaseException {

        return cursor(null, fromKey, fromInclusive, toKey, toInclusive,
                      entityAdapter, null);
    }

    public EntityCursor<E> entities(Transaction txn,
                                    PK fromKey,
                                    boolean fromInclusive,
                                    PK toKey,
                                    boolean toInclusive,
                                    CursorConfig config)
        throws DatabaseException {

        return cursor(txn, fromKey, fromInclusive, toKey, toInclusive,
                      entityAdapter, config);
    }

    /*
    public ForwardCursor<PK> unsortedKeys(KeySelector<PK> selector)
        throws DatabaseException {

        return unsortedKeys(null, selector, null);
    }

    public ForwardCursor<PK> unsortedKeys(Transaction txn,
                                          KeySelector<PK> selector,
                                          CursorConfig config)
        throws DatabaseException {

        throw new UnsupportedOperationException();
    }

    public ForwardCursor<E> unsortedEntities(KeySelector<PK> selector)
        throws DatabaseException {

        return unsortedEntities(null, selector, null);
    }

    public ForwardCursor<E> unsortedEntities(Transaction txn,
                                             KeySelector<PK> selector,
                                             CursorConfig config)
        throws DatabaseException {

        throw new UnsupportedOperationException();
    }
    */

    private <V> EntityCursor<V> cursor(Transaction txn,
                                       PK fromKey,
                                       boolean fromInclusive,
                                       PK toKey,
                                       boolean toInclusive,
                                       ValueAdapter<V> adapter,
                                       CursorConfig config)
        throws DatabaseException {

        DatabaseEntry fromEntry = null;
        if (fromKey != null) {
            fromEntry = new DatabaseEntry();
            pkeyBinding.objectToEntry(fromKey, fromEntry);
        }
        DatabaseEntry toEntry = null;
        if (toKey != null) {
            toEntry = new DatabaseEntry();
            pkeyBinding.objectToEntry(toKey, toEntry);
        }
        KeyRange pkeyRange = emptyPKeyRange.subRange
            (fromEntry, fromInclusive, toEntry, toInclusive);
        return cursor(txn, pkeyRange, adapter, config);
    }

    private <V> EntityCursor<V> cursor(Transaction txn,
                                       KeyRange pkeyRange,
                                       ValueAdapter<V> adapter,
                                       CursorConfig config)
        throws DatabaseException {

        Cursor cursor = db.openCursor(txn, config);
        RangeCursor rangeCursor =
            new RangeCursor(singleKeyRange, pkeyRange, sortedDups, cursor);
        return new SubIndexCursor<V>(rangeCursor, adapter);
    }

    public Map<PK,E> map() {
        return sortedMap();
    }

    public synchronized SortedMap<PK,E> sortedMap() {
        if (map == null) {
            map = (SortedMap) ((StoredSortedMap) secIndex.sortedMap()).
                duplicatesMap(keyObject, pkeyBinding);
        }
        return map;
    }
}
