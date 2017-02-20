/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import java.util.Iterator;

import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.util.keyrange.RangeCursor;

/**
 * Implements EntityCursor and uses a ValueAdapter so that it can enumerate
 * either keys or entities.
 *
 * @author Mark Hayes
 */
class BasicCursor<V> implements EntityCursor<V> {

    RangeCursor cursor;
    ValueAdapter<V> adapter;
    boolean updateAllowed;
    DatabaseEntry key;
    DatabaseEntry pkey;
    DatabaseEntry data;

    BasicCursor(RangeCursor cursor,
                ValueAdapter<V> adapter,
                boolean updateAllowed) {
        this.cursor = cursor;
        this.adapter = adapter;
        this.updateAllowed = updateAllowed;
        key = adapter.initKey();
        pkey = adapter.initPKey();
        data = adapter.initData();
    }

    public V first()
        throws DatabaseException {

        return first(null);
    }

    public V first(LockMode lockMode)
        throws DatabaseException {

        return returnValue(cursor.getFirst(key, pkey, data, lockMode));
    }

    public V last()
        throws DatabaseException {

        return last(null);
    }

    public V last(LockMode lockMode)
        throws DatabaseException {

        return returnValue(cursor.getLast(key, pkey, data, lockMode));
    }

    public V next()
        throws DatabaseException {

        return next(null);
    }

    public V next(LockMode lockMode)
        throws DatabaseException {

        return returnValue(cursor.getNext(key, pkey, data, lockMode));
    }

    public V nextDup()
        throws DatabaseException {

        return nextDup(null);
    }

    public V nextDup(LockMode lockMode)
        throws DatabaseException {

        checkInitialized();
        return returnValue(cursor.getNextDup(key, pkey, data, lockMode));
    }

    public V nextNoDup()
        throws DatabaseException {

        return nextNoDup(null);
    }

    public V nextNoDup(LockMode lockMode)
        throws DatabaseException {

        return returnValue(cursor.getNextNoDup(key, pkey, data, lockMode));
    }

    public V prev()
        throws DatabaseException {

        return prev(null);
    }

    public V prev(LockMode lockMode)
        throws DatabaseException {

        return returnValue(cursor.getPrev(key, pkey, data, lockMode));
    }

    public V prevDup()
        throws DatabaseException {

        return prevDup(null);
    }

    public V prevDup(LockMode lockMode)
        throws DatabaseException {

        checkInitialized();
        return returnValue(cursor.getPrevDup(key, pkey, data, lockMode));
    }

    public V prevNoDup()
        throws DatabaseException {

        return prevNoDup(null);
    }

    public V prevNoDup(LockMode lockMode)
        throws DatabaseException {

        return returnValue(cursor.getPrevNoDup(key, pkey, data, lockMode));
    }

    public V current()
        throws DatabaseException {

        return current(null);
    }

    public V current(LockMode lockMode)
        throws DatabaseException {

        checkInitialized();
        return returnValue(cursor.getCurrent(key, pkey, data, lockMode));
    }

    public int count()
        throws DatabaseException {

        checkInitialized();
        return cursor.count();
    }

    public Iterator<V> iterator() {
        return iterator(null);
    }

    public Iterator<V> iterator(LockMode lockMode) {
        return new BasicIterator(this, lockMode);
    }

    public boolean update(V entity)
        throws DatabaseException {

        if (!updateAllowed) {
            throw new UnsupportedOperationException
                ("Update not allowed on a secondary index");
        }
        checkInitialized();
        adapter.valueToData(entity, data);
        return cursor.putCurrent(data) == OperationStatus.SUCCESS;
    }

    public boolean delete()
        throws DatabaseException {

        checkInitialized();
        return cursor.delete() == OperationStatus.SUCCESS;
    }

    public EntityCursor<V> dup()
        throws DatabaseException {

        return new BasicCursor<V>(cursor.dup(true), adapter, updateAllowed);
    }

    public void close()
        throws DatabaseException {

        cursor.close();
    }



    void checkInitialized()
        throws IllegalStateException {

        if (!cursor.isInitialized()) {
            throw new IllegalStateException
                ("Cursor is not initialized at a valid position");
        }
    }

    V returnValue(OperationStatus status) {
        V value;
        if (status == OperationStatus.SUCCESS) {
            value = adapter.entryToValue(key, pkey, data);
        } else {
            value = null;
        }
        /* Clear entries to save memory. */
        adapter.clearEntries(key, pkey, data);
        return value;
    }
}
