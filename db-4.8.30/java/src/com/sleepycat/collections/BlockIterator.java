/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.util.ListIterator;
import java.util.NoSuchElementException;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.util.keyrange.KeyRange;

/**
 * An iterator that does not need closing because a cursor is not kept open
 * across method calls.  A cursor is opened to read a block of records at a
 * time and then closed before the method returns.
 *
 * @author Mark Hayes
 */
class BlockIterator<E> implements BaseIterator<E> {

    private StoredCollection<E> coll;
    private boolean writeAllowed;

    /**
     * Slots for a block of record keys and values.  The priKeys array is only
     * used for secondary databases; otherwise it is set to the keys array.
     */
    private byte[][] keys;
    private byte[][] priKeys;
    private byte[][] values;

    /**
     * The slot index of the record that would be returned by next().
     * nextIndex is always greater or equal to zero.  If the next record is not
     * available, then nextIndex is equal to keys.length or keys[nextIndex] is
     * null.
     *
     * If the block is empty, then either the iterator is uninitialized or the
     * key range is empty.  Either way, nextIndex will be the array length and
     * all array values will be null.  This is the initial state set by the
     * constructor.  If remove() is used to delete all records in the key
     * range, it will restore the iterator to this initial state.  The block
     * must never be allowed to become empty when the key range is non-empty,
     * since then the iterator's position would be lost.  [#15858]
     */
    private int nextIndex;

    /**
     * The slot index of the record last returned by next() or previous(), or
     * the record inserted by add().  dataIndex is -1 if the data record is not
     * available.  If greater or equal to zero, the slot at dataIndex is always
     * non-null.
     */
    private int dataIndex;

    /**
     * The iterator data last returned by next() or previous().  This value is
     * set to null if dataIndex is -1, or if the state of the iterator is such
     * that set() or remove() cannot be called.  For example, after add() this
     * field is set to null, even though the dataIndex is still valid.
     */
    private E dataObject;

    /**
     * Creates an iterator.
     */
    BlockIterator(StoredCollection<E> coll,
                  boolean writeAllowed,
                  int blockSize) {

        this.coll = coll;
        this.writeAllowed = writeAllowed;

        keys = new byte[blockSize][];
        priKeys = coll.isSecondary() ? (new byte[blockSize][]) : keys;
        values = new byte[blockSize][];

        nextIndex = blockSize;
        dataIndex = -1;
        dataObject = null;
    }

    /**
     * Copy constructor.
     */
    private BlockIterator(BlockIterator<E> o) {

        coll = o.coll;
        writeAllowed = o.writeAllowed;

        keys = copyArray(o.keys);
        priKeys = coll.isSecondary() ? copyArray(o.priKeys) : keys;
        values = copyArray(o.values);

        nextIndex = o.nextIndex;
        dataIndex = o.dataIndex;
        dataObject = o.dataObject;
    }

    /**
     * Copies an array of byte arrays.
     */
    private byte[][] copyArray(byte[][] a) {

        byte[][] b = new byte[a.length][];
        for (int i = 0; i < b.length; i += 1) {
            if (a[i] != null) {
                b[i] = KeyRange.copyBytes(a[i]);
            }
        }
        return b;
    }

    /**
     * Returns whether the element at nextIndex is available.
     */
    private boolean isNextAvailable() {

        return (nextIndex < keys.length) &&
               (keys[nextIndex] != null);
    }

    /**
     * Returns whether the element at nextIndex-1 is available.
     */
    private boolean isPrevAvailable() {

        return (nextIndex > 0) &&
               (keys[nextIndex - 1] != null);
    }

    /**
     * Returns the record number at the given slot position.
     */
    private int getRecordNumber(int i) {

        if (coll.view.btreeRecNumDb) {
            DataCursor cursor = null;
            try {
                cursor = new DataCursor(coll.view, false);
                if (moveCursor(i, cursor)) {
                    return cursor.getCurrentRecordNumber();
                } else {
                    throw new IllegalStateException();
                }
            } catch (DatabaseException e) {
                throw StoredContainer.convertException(e);
            } finally {
                closeCursor(cursor);
            }
        } else {
            DatabaseEntry entry = new DatabaseEntry(keys[i]);
            return DbCompat.getRecordNumber(entry);
        }
    }

    /**
     * Sets dataObject to the iterator data for the element at dataIndex.
     */
    private void makeDataObject() {

        int i = dataIndex;
        DatabaseEntry keyEntry = new DatabaseEntry(keys[i]);
        DatabaseEntry priKeyEntry = (keys != priKeys)
                                    ? (new DatabaseEntry(priKeys[i]))
                                    : keyEntry;
        DatabaseEntry valuesEntry = new DatabaseEntry(values[i]);

        dataObject = coll.makeIteratorData(this, keyEntry, priKeyEntry,
                                           valuesEntry);
    }

    /**
     * Sets all slots to null.
     */
    private void clearSlots() {

        for (int i = 0; i < keys.length; i += 1) {
            keys[i] = null;
            priKeys[i] = null;
            values[i] = null;
        }
    }

    /**
     * Sets a given slot using the data in the given cursor.
     */
    private void setSlot(int i, DataCursor cursor) {

        keys[i] = KeyRange.getByteArray(cursor.getKeyThang());

        if (keys != priKeys) {
            priKeys[i] = KeyRange.getByteArray
                (cursor.getPrimaryKeyThang());
        }

        values[i] = KeyRange.getByteArray(cursor.getValueThang());
    }

    /**
     * Inserts an added record at a given slot position and shifts other slots
     * accordingly.  Also adjusts nextIndex and sets dataIndex to -1.
     */
    private void insertSlot(int i, DataCursor cursor) {

        if (i < keys.length) {
            for (int j = keys.length - 1; j > i; j -= 1) {

                /* Shift right. */
                keys[j] = keys[j - 1];
                priKeys[j] = priKeys[j - 1];
                values[j] = values[j - 1];

                /* Bump key in recno-renumber database. */
                if (coll.view.recNumRenumber && keys[j] != null) {
                    bumpRecordNumber(j);
                }
            }
            nextIndex += 1;
        } else {
            if (i != keys.length) {
                throw new IllegalStateException();
            }
            i -= 1;
            for (int j = 0; j < i; j += 1) {
                /* Shift left. */
                keys[j] = keys[j + 1];
                priKeys[j] = priKeys[j + 1];
                values[j] = values[j + 1];
            }
        }
        setSlot(i, cursor);
        dataIndex = -1;
    }

    /**
     * Increments the record number key at the given slot.
     */
    private void bumpRecordNumber(int i) {

        DatabaseEntry entry = new DatabaseEntry(keys[i]);
        DbCompat.setRecordNumber(entry,
                                 DbCompat.getRecordNumber(entry) + 1);
        keys[i] = entry.getData();
    }

    /**
     * Deletes the given slot, adjusts nextIndex and sets dataIndex to -1.
     */
    private void deleteSlot(int i) {

        for (int j = i + 1; j < keys.length; j += 1) {
            keys[j - 1] = keys[j];
            priKeys[j - 1] = priKeys[j];
            values[j - 1] = values[j];
        }
        int last = keys.length - 1;
        keys[last] = null;
        priKeys[last] = null;
        values[last] = null;

        if (nextIndex > i) {
            nextIndex -= 1;
        }
        dataIndex = -1;
    }

    /**
     * Moves the cursor to the key/data at the given slot, and returns false
     * if the reposition (search) fails.
     */
    private boolean moveCursor(int i, DataCursor cursor)
        throws DatabaseException {

        return cursor.repositionExact(keys[i], priKeys[i], values[i], false);
    }

    /**
     * Closes the given cursor if non-null.
     */
    private void closeCursor(DataCursor cursor) {

        if (cursor != null) {
            try {
                cursor.close();
            } catch (DatabaseException e) {
                throw StoredContainer.convertException(e);
            }
        }
    }

    // --- begin Iterator/ListIterator methods ---

    public boolean hasNext() {

        if (isNextAvailable()) {
            return true;
        }
        DataCursor cursor = null;
        try {
            cursor = new DataCursor(coll.view, writeAllowed);
            int prev = nextIndex - 1;
            boolean found = false;

            if (keys[prev] == null) {
                /* Get the first record for an uninitialized iterator. */
                OperationStatus status = cursor.getFirst(false);
                if (status == OperationStatus.SUCCESS) {
                    found = true;
                    nextIndex = 0;
                }
            } else {
                /* Reposition to the last known key/data pair. */
                int repos = cursor.repositionRange
                    (keys[prev], priKeys[prev], values[prev], false);

                if (repos == DataCursor.REPOS_EXACT) {

                    /*
                     * The last known key/data pair was found and will now be
                     * in slot zero.
                     */
                    found = true;
                    nextIndex = 1;

                    /* The data object is now in slot zero or not available. */
                    if (dataIndex == prev) {
                        dataIndex = 0;
                    } else {
                        dataIndex = -1;
                        dataObject = null;
                    }
                } else if (repos == DataCursor.REPOS_NEXT) {

                    /*
                     * The last known key/data pair was not found, but the
                     * following record was found and it will be in slot zero.
                     */
                    found = true;
                    nextIndex = 0;

                    /* The data object is no longer available. */
                    dataIndex = -1;
                    dataObject = null;
                } else {
                    if (repos != DataCursor.REPOS_EOF) {
                        throw new IllegalStateException();
                    }
                }
            }

            if (found) {
                /* Clear all slots first in case an exception occurs below. */
                clearSlots();

                /* Attempt to fill all slots with records. */
                int i = 0;
                boolean done = false;
                while (!done) {
                    setSlot(i, cursor);
                    i += 1;
                    if (i < keys.length) {
                        OperationStatus status = coll.iterateDuplicates() ?
                                                 cursor.getNext(false) :
                                                 cursor.getNextNoDup(false);
                        if (status != OperationStatus.SUCCESS) {
                            done = true;
                        }
                    } else {
                        done = true;
                    }
                }

            }

            /*
             * If REPOS_EXACT was returned above, make sure we retrieved
             * the following record.
             */
            return isNextAvailable();
        } catch (DatabaseException e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    public boolean hasPrevious() {

        if (isPrevAvailable()) {
            return true;
        }
        if (!isNextAvailable()) {
            return false;
        }
        DataCursor cursor = null;
        try {
            cursor = new DataCursor(coll.view, writeAllowed);
            int last = keys.length - 1;
            int next = nextIndex;
            boolean found = false;

            /* Reposition to the last known key/data pair. */
            int repos = cursor.repositionRange
                (keys[next], priKeys[next], values[next], false);

            if (repos == DataCursor.REPOS_EXACT ||
                repos == DataCursor.REPOS_NEXT) {

                /*
                 * The last known key/data pair, or the record following it,
                 * was found and will now be in the last slot.
                 */
                found = true;
                nextIndex = last;

                /* The data object is now in the last slot or not available. */
                if (dataIndex == next && repos == DataCursor.REPOS_EXACT) {
                    dataIndex = last;
                } else {
                    dataIndex = -1;
                    dataObject = null;
                }
            } else {
                if (repos != DataCursor.REPOS_EOF) {
                    throw new IllegalStateException();
                }
            }

            if (found) {
                /* Clear all slots first in case an exception occurs below. */
                clearSlots();

                /* Attempt to fill all slots with records. */
                int i = last;
                boolean done = false;
                while (!done) {
                    setSlot(i, cursor);
                    i -= 1;
                    if (i >= 0) {
                        OperationStatus status = coll.iterateDuplicates() ?
                                                 cursor.getPrev(false) :
                                                 cursor.getPrevNoDup(false);
                        if (status != OperationStatus.SUCCESS) {
                            done = true;
                        }
                    } else {
                        done = true;
                    }
                }
            }

            /*
             * Make sure we retrieved the preceding record after the reposition
             * above.
             */
            return isPrevAvailable();
        } catch (DatabaseException e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    public E next() {

        if (hasNext()) {
            dataIndex = nextIndex;
            nextIndex += 1;
            makeDataObject();
            return dataObject;
        } else {
            throw new NoSuchElementException();
        }
    }

    public E previous() {

        if (hasPrevious()) {
            nextIndex -= 1;
            dataIndex = nextIndex;
            makeDataObject();
            return dataObject;
        } else {
            throw new NoSuchElementException();
        }
    }

    public int nextIndex() {

        if (!coll.view.recNumAccess) {
            throw new UnsupportedOperationException(
                "Record number access not supported");
        }

        return hasNext() ? (getRecordNumber(nextIndex) -
                            coll.getIndexOffset())
                         : Integer.MAX_VALUE;
    }

    public int previousIndex() {

        if (!coll.view.recNumAccess) {
            throw new UnsupportedOperationException(
                "Record number access not supported");
        }

        return hasPrevious() ? (getRecordNumber(nextIndex - 1) -
                                coll.getIndexOffset())
                             : (-1);
    }

    public void set(E value) {

        if (dataObject == null) {
            throw new IllegalStateException();
        }
        if (!coll.hasValues()) {
            throw new UnsupportedOperationException();
        }
        DataCursor cursor = null;
        boolean doAutoCommit = coll.beginAutoCommit();
        try {
            cursor = new DataCursor(coll.view, writeAllowed);
            if (moveCursor(dataIndex, cursor)) {
                cursor.putCurrent(value);
                setSlot(dataIndex, cursor);
                coll.closeCursor(cursor);
                coll.commitAutoCommit(doAutoCommit);
            } else {
                throw new IllegalStateException();
            }
        } catch (Exception e) {
            coll.closeCursor(cursor);
            throw coll.handleException(e, doAutoCommit);
        }
    }

    public void remove() {

        if (dataObject == null) {
            throw new IllegalStateException();
        }
        DataCursor cursor = null;
        boolean doAutoCommit = coll.beginAutoCommit();
        try {
            cursor = new DataCursor(coll.view, writeAllowed);
            if (moveCursor(dataIndex, cursor)) {
                cursor.delete();
                deleteSlot(dataIndex);
                dataObject = null;

                /*
                 * Repopulate the block after removing all records, using the
                 * cursor position of the deleted record as a starting point.
                 * First try moving forward, since the user was moving forward.
                 * (It is possible to delete all records in the block only by
                 * moving forward, i.e, when nextIndex is greater than
                 * dataIndex.)
                 */
                if (nextIndex == 0 && keys[0] == null) {
                    OperationStatus status;
                    for (int i = 0; i < keys.length; i += 1) {
                        status = coll.iterateDuplicates() ?
                                 cursor.getNext(false) :
                                 cursor.getNextNoDup(false);
                        if (status == OperationStatus.SUCCESS) {
                            setSlot(i, cursor);
                        } else {
                            break;
                        }
                    }

                    /*
                     * If no records are found past the cursor position, try
                     * moving backward.  If no records are found before the
                     * cursor position, leave nextIndex set to keys.length,
                     * which is the same as the initial iterator state and is
                     * appropriate for an empty key range.
                     */
                    if (keys[0] == null) {
                        nextIndex = keys.length;
                        for (int i = nextIndex - 1; i >= 0; i -= 1) {
                            status = coll.iterateDuplicates() ?
                                     cursor.getPrev(false) :
                                     cursor.getPrevNoDup(false);
                            if (status == OperationStatus.SUCCESS) {
                                setSlot(i, cursor);
                            } else {
                                break;
                            }
                        }
                    }
                }
                coll.closeCursor(cursor);
                coll.commitAutoCommit(doAutoCommit);
            } else {
                throw new IllegalStateException();
            }
        } catch (Exception e) {
            coll.closeCursor(cursor);
            throw coll.handleException(e, doAutoCommit);
        }
    }

    public void add(E value) {

        /*
         * The checkIterAddAllowed method ensures that one of the following two
         * conditions holds and throws UnsupportedOperationException otherwise:
         * 1- This is a list iterator for a recno-renumber database.
         * 2- This is a collection iterator for a duplicates database.
         */
        coll.checkIterAddAllowed();
        OperationStatus status = OperationStatus.SUCCESS;
        DataCursor cursor = null;
        boolean doAutoCommit = coll.beginAutoCommit();
        try {
            if (coll.view.keysRenumbered || !coll.areDuplicatesOrdered()) {

                /*
                 * This is a recno-renumber database or a btree/hash database
                 * with unordered duplicates.
                 */
                boolean hasPrev = hasPrevious();
                if (!hasPrev && !hasNext()) {

                    /* The collection is empty. */
                    if (coll.view.keysRenumbered) {

                        /* Append to an empty recno-renumber database. */
                        status = coll.view.append(value, null, null);

                    } else if (coll.view.dupsAllowed &&
                               coll.view.range.isSingleKey()) {

                        /*
                         * When inserting a duplicate into a single-key range,
                         * the main key is fixed, so we can always insert into
                         * an empty collection.
                         */
                        cursor = new DataCursor(coll.view, writeAllowed);
                        cursor.useRangeKey();
                        status = cursor.putNoDupData(null, value, null, true);
                        coll.closeCursor(cursor);
                        cursor = null;
                    } else {
                        throw new IllegalStateException
                            ("Collection is empty, cannot add() duplicate");
                    }

                    /*
                     * Move past the record just inserted (the cursor should be
                     * closed above to prevent multiple open cursors in certain
                     * DB core modes).
                     */
                    if (status == OperationStatus.SUCCESS) {
                        next();
                        dataIndex = nextIndex - 1;
                    }
                } else {

                    /*
                     * The collection is non-empty.  If hasPrev is true then
                     * the element at (nextIndex - 1) is available; otherwise
                     * the element at nextIndex is available.
                     */
                    cursor = new DataCursor(coll.view, writeAllowed);
                    int insertIndex = hasPrev ? (nextIndex - 1) : nextIndex;

                    if (!moveCursor(insertIndex, cursor)) {
                        throw new IllegalStateException();
                    }

                    /*
                     * For a recno-renumber database or a database with
                     * unsorted duplicates, insert before the iterator 'next'
                     * position, or after the 'prev' position.  Then adjust
                     * the slots to account for the inserted record.
                     */
                    status = hasPrev ? cursor.putAfter(value)
                                     : cursor.putBefore(value);
                    if (status == OperationStatus.SUCCESS) {
                        insertSlot(nextIndex, cursor);
                    }
                }
            } else {
                /* This is a btree/hash database with ordered duplicates. */
                cursor = new DataCursor(coll.view, writeAllowed);

                if (coll.view.range.isSingleKey()) {

                    /*
                     * When inserting a duplicate into a single-key range,
                     * the main key is fixed.
                     */
                    cursor.useRangeKey();
                } else {

                    /*
                     * When inserting into a multi-key range, the main key
                     * is the last dataIndex accessed by next(), previous()
                     * or add().
                     */
                    if (dataIndex < 0 || !moveCursor(dataIndex, cursor)) {
                        throw new IllegalStateException();
                    }
                }

                /*
                 * For a hash/btree with duplicates, insert the duplicate,
                 * put the new record in slot zero, and set the next index
                 * to slot one (past the new record).
                 */
                status = cursor.putNoDupData(null, value, null, true);
                if (status == OperationStatus.SUCCESS) {
                    clearSlots();
                    setSlot(0, cursor);
                    dataIndex = 0;
                    nextIndex = 1;
                }
            }

            if (status == OperationStatus.KEYEXIST) {
                throw new IllegalArgumentException("Duplicate value");
            } else if (status != OperationStatus.SUCCESS) {
                throw new IllegalArgumentException("Could not insert: " +
                                                    status);
            }

            /* Prevent subsequent set() or remove() call. */
            dataObject = null;

            coll.closeCursor(cursor);
            coll.commitAutoCommit(doAutoCommit);
        } catch (Exception e) {
            /* Catch RuntimeExceptions too. */
            coll.closeCursor(cursor);
            throw coll.handleException(e, doAutoCommit);
        }
    }

    // --- end Iterator/ListIterator methods ---

    // --- begin BaseIterator methods ---

    public final ListIterator<E> dup() {

        return new BlockIterator<E>(this);
    }

    public final boolean isCurrentData(Object currentData) {

        return (dataObject == currentData);
    }

    public final boolean moveToIndex(int index) {

        DataCursor cursor = null;
        try {
            cursor = new DataCursor(coll.view, writeAllowed);
            OperationStatus status =
                cursor.getSearchKey(Integer.valueOf(index), null, false);
            if (status == OperationStatus.SUCCESS) {
                clearSlots();
                setSlot(0, cursor);
                nextIndex = 0;
                return true;
            } else {
                return false;
            }
        } catch (DatabaseException e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    // --- end BaseIterator methods ---
}
