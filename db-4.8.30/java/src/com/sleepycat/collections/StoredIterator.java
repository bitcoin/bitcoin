/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.util.Iterator;
import java.util.ListIterator;
import java.util.NoSuchElementException;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * The Iterator returned by all stored collections.
 *
 * <p>While in general this class conforms to the {@link Iterator} interface,
 * it is important to note that all iterators for stored collections must be
 * explicitly closed with {@link #close()}.  The static method {@link
 * #close(java.util.Iterator)} allows calling close for all iterators without
 * harm to iterators that are not from stored collections, and also avoids
 * casting.  If a stored iterator is not closed, unpredictable behavior
 * including process death may result.</p>
 *
 * <p>This class implements the {@link Iterator} interface for all stored
 * iterators.  It also implements {@link ListIterator} because some list
 * iterator methods apply to all stored iterators, for example, {@link
 * #previous} and {@link #hasPrevious}.  Other list iterator methods are always
 * supported for lists, but for other types of collections are only supported
 * under certain conditions.  See {@link #nextIndex}, {@link #previousIndex},
 * {@link #add} and {@link #set} for details.</p>
 *
 * <p>In addition, this class provides the following methods for stored
 * collection iterators only.  Note that the use of these methods is not
 * compatible with the standard Java collections interface.</p>
 * <ul>
 * <li>{@link #close()}</li>
 * <li>{@link #close(Iterator)}</li>
 * <li>{@link #count()}</li>
 * <li>{@link #getCollection}</li>
 * <li>{@link #setReadModifyWrite}</li>
 * <li>{@link #isReadModifyWrite}</li>
 * </ul>
 *
 * @author Mark Hayes
 */
public class StoredIterator<E>
    implements ListIterator<E>, BaseIterator<E>, Cloneable {

    /**
     * Closes the given iterator using {@link #close()} if it is a {@link
     * StoredIterator}.  If the given iterator is not a {@link StoredIterator},
     * this method does nothing.
     *
     * @param i is the iterator to close.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is thrown.
     */
    public static void close(Iterator<?> i) {

        if (i instanceof StoredIterator) {
            ((StoredIterator) i).close();
        }
    }

    private static final int MOVE_NEXT = 1;
    private static final int MOVE_PREV = 2;
    private static final int MOVE_FIRST = 3;

    private boolean lockForWrite;
    private StoredCollection<E> coll;
    private DataCursor cursor;
    private int toNext;
    private int toPrevious;
    private int toCurrent;
    private boolean writeAllowed;
    private boolean setAndRemoveAllowed;
    private E currentData;

    StoredIterator(StoredCollection<E> coll,
                   boolean writeAllowed,
                   DataCursor joinCursor) {
        try {
            this.coll = coll;
            this.writeAllowed = writeAllowed;
            if (joinCursor == null)
                this.cursor = new DataCursor(coll.view, writeAllowed);
            else
                this.cursor = joinCursor;
            reset();
        } catch (Exception e) {
            try {
                /* Ensure that the cursor is closed.  [#10516] */
                close();
            } catch (Exception ignored) {
		/* Klockwork - ok */
	    }
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Returns whether write-locks will be obtained when reading with this
     * cursor.
     * Obtaining write-locks can prevent deadlocks when reading and then
     * modifying data.
     *
     * @return the write-lock setting.
     */
    public final boolean isReadModifyWrite() {

        return lockForWrite;
    }

    /**
     * Changes whether write-locks will be obtained when reading with this
     * cursor.
     * Obtaining write-locks can prevent deadlocks when reading and then
     * modifying data.
     *
     * @param lockForWrite the write-lock setting.
     */
    public void setReadModifyWrite(boolean lockForWrite) {

        this.lockForWrite = lockForWrite;
    }

    // --- begin Iterator/ListIterator methods ---

    /**
     * Returns true if this iterator has more elements when traversing in the
     * forward direction.  False is returned if the iterator has been closed.
     * This method conforms to the {@link Iterator#hasNext} interface.
     *
     * @return whether {@link #next()} will succeed.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is thrown.
     */
    public boolean hasNext() {

        if (cursor == null) {
            return false;
        }
        try {
            if (toNext != 0) {
                OperationStatus status = move(toNext);
                if (status == OperationStatus.SUCCESS) {
                    toNext = 0;
                    toPrevious = MOVE_PREV;
                    toCurrent = MOVE_PREV;
                }
            }
            return (toNext == 0);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Returns true if this iterator has more elements when traversing in the
     * reverse direction.  It returns false if the iterator has been closed.
     * This method conforms to the {@link ListIterator#hasPrevious} interface.
     *
     * @return whether {@link #previous()} will succeed.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is thrown.
     */
    public boolean hasPrevious() {

        if (cursor == null) {
            return false;
        }
        try {
            if (toPrevious != 0) {
                OperationStatus status = move(toPrevious);
                if (status == OperationStatus.SUCCESS) {
                    toPrevious = 0;
                    toNext = MOVE_NEXT;
                    toCurrent = MOVE_NEXT;
                }
            }
            return (toPrevious == 0);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Returns the next element in the iteration.
     * This method conforms to the {@link Iterator#next} interface.
     *
     * @return the next element.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public E next() {

        try {
            if (toNext != 0) {
                OperationStatus status = move(toNext);
                if (status == OperationStatus.SUCCESS) {
                    toNext = 0;
                }
            }
            if (toNext == 0) {
                currentData = coll.makeIteratorData(this, cursor);
                toNext = MOVE_NEXT;
                toPrevious = 0;
                toCurrent = 0;
                setAndRemoveAllowed = true;
                return currentData;
            }
            // else throw NoSuchElementException below
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
        throw new NoSuchElementException();
    }

    /**
     * Returns the next element in the iteration.
     * This method conforms to the {@link ListIterator#previous} interface.
     *
     * @return the previous element.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public E previous() {

        try {
            if (toPrevious != 0) {
                OperationStatus status = move(toPrevious);
                if (status == OperationStatus.SUCCESS) {
                    toPrevious = 0;
                }
            }
            if (toPrevious == 0) {
                currentData = coll.makeIteratorData(this, cursor);
                toPrevious = MOVE_PREV;
                toNext = 0;
                toCurrent = 0;
                setAndRemoveAllowed = true;
                return currentData;
            }
            // else throw NoSuchElementException below
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
        throw new NoSuchElementException();
    }

    /**
     * Returns the index of the element that would be returned by a subsequent
     * call to next.
     * This method conforms to the {@link ListIterator#nextIndex} interface
     * except that it returns Integer.MAX_VALUE for stored lists when
     * positioned at the end of the list, rather than returning the list size
     * as specified by the ListIterator interface. This is because the database
     * size is not available.
     *
     * @return the next index.
     *
     * @throws UnsupportedOperationException if this iterator's collection does
     * not use record number keys.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public int nextIndex() {

        if (!coll.view.recNumAccess) {
            throw new UnsupportedOperationException(
                "Record number access not supported");
        }
        try {
            return hasNext() ? (cursor.getCurrentRecordNumber() -
                                coll.getIndexOffset())
                             : Integer.MAX_VALUE;
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Returns the index of the element that would be returned by a subsequent
     * call to previous.
     * This method conforms to the {@link ListIterator#previousIndex}
     * interface.
     *
     * @return the previous index.
     *
     * @throws UnsupportedOperationException if this iterator's collection does
     * not use record number keys.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public int previousIndex() {

        if (!coll.view.recNumAccess) {
            throw new UnsupportedOperationException(
                "Record number access not supported");
        }
        try {
            return hasPrevious() ? (cursor.getCurrentRecordNumber() -
                                    coll.getIndexOffset())
                                 : (-1);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Replaces the last element returned by next or previous with the
     * specified element (optional operation).
     * This method conforms to the {@link ListIterator#set} interface.
     *
     * <p>In order to call this method, if the underlying Database is
     * transactional then a transaction must be active when creating the
     * iterator.</p>
     *
     * @param value the new value.
     *
     * @throws UnsupportedOperationException if the collection is a {@link
     * StoredKeySet} (the set returned by {@link java.util.Map#keySet}), or if
     * duplicates are sorted since this would change the iterator position, or
     * if the collection is indexed, or if the collection is read-only.
     *
     * @throws IllegalArgumentException if an entity value binding is used and
     * the primary key of the value given is different than the existing stored
     * primary key.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public void set(E value) {

        if (!coll.hasValues()) throw new UnsupportedOperationException();
        if (!setAndRemoveAllowed) throw new IllegalStateException();
        try {
            moveToCurrent();
            cursor.putCurrent(value);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Removes the last element that was returned by next or previous (optional
     * operation).
     * This method conforms to the {@link ListIterator#remove} interface except
     * that when the collection is a list and the RECNO-RENUMBER access method
     * is not used, list indices will not be renumbered.
     *
     * <p>In order to call this method, if the underlying Database is
     * transactional then a transaction must be active when creating the
     * iterator.</p>
     *
     * <p>Note that for the JE product, RECNO-RENUMBER databases are not
     * supported, and therefore list indices are never renumbered by this
     * method.</p>
     *
     * @throws UnsupportedOperationException if the collection is a sublist, or
     * if the collection is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public void remove() {

        if (!setAndRemoveAllowed) throw new IllegalStateException();
        try {
            moveToCurrent();
            cursor.delete();
            setAndRemoveAllowed = false;
            toNext = MOVE_NEXT;
            toPrevious = MOVE_PREV;
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Inserts the specified element into the list or inserts a duplicate into
     * other types of collections (optional operation).
     * This method conforms to the {@link ListIterator#add} interface when
     * the collection is a list and the RECNO-RENUMBER access method is used.
     * Otherwise, this method may only be called when duplicates are allowed.
     * If duplicates are unsorted, the new value will be inserted in the same
     * manner as list elements.
     * If duplicates are sorted, the new value will be inserted in sort order.
     *
     * <p>Note that for the JE product, RECNO-RENUMBER databases are not
     * supported, and therefore this method may only be used to add
     * duplicates.</p>
     *
     * @param value the new value.
     *
     * @throws UnsupportedOperationException if the collection is a sublist, or
     * if the collection is indexed, or if the collection is read-only, or if
     * the collection is a list and the RECNO-RENUMBER access method was not
     * used, or if the collection is not a list and duplicates are not allowed.
     *
     * @throws IllegalStateException if the collection is empty and is not a
     * list with RECNO-RENUMBER access.
     *
     * @throws IllegalArgumentException if a duplicate value is being added
     * that already exists and duplicates are sorted.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public void add(E value) {

        coll.checkIterAddAllowed();
        try {
            OperationStatus status = OperationStatus.SUCCESS;
            if (toNext != 0 && toPrevious != 0) { // database is empty
                if (coll.view.keysRenumbered) { // recno-renumber database
                    /*
                     * Close cursor during append and then reopen to support
                     * CDB restriction that append may not be called with a
                     * cursor open; note the append will still fail if the
                     * application has another cursor open.
                     */
                    close();
                    status = coll.view.append(value, null, null);
                    cursor = new DataCursor(coll.view, writeAllowed);
                    reset();
                    next(); // move past new record
                } else { // hash/btree with duplicates
                    throw new IllegalStateException(
                        "Collection is empty, cannot add() duplicate");
                }
            } else { // database is not empty
                boolean putBefore = false;
                if (coll.view.keysRenumbered) { // recno-renumber database
                    moveToCurrent();
                    if (hasNext()) {
                        status = cursor.putBefore(value);
                        putBefore = true;
                    } else {
                        status = cursor.putAfter(value);
                    }
                } else { // hash/btree with duplicates
                    if (coll.areDuplicatesOrdered()) {
                        status = cursor.putNoDupData(null, value, null, true);
                    } else if (toNext == 0) {
                        status = cursor.putBefore(value);
                        putBefore = true;
                    } else {
                        status = cursor.putAfter(value);
                    }
                }
                if (putBefore) {
                    toPrevious = 0;
                    toNext = MOVE_NEXT;
                }
            }
            if (status == OperationStatus.KEYEXIST) {
                throw new IllegalArgumentException("Duplicate value");
            } else if (status != OperationStatus.SUCCESS) {
                throw new IllegalArgumentException("Could not insert: " +
                                                    status);
            }
            setAndRemoveAllowed = false;
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    // --- end Iterator/ListIterator methods ---

    /**
     * Resets cursor to an uninitialized state.
     */
    private void reset() {

        toNext = MOVE_FIRST;
        toPrevious = MOVE_PREV;
        toCurrent = 0;
        currentData = null;
        /*
	 * Initialize cursor at beginning to avoid "initial previous == last"
	 * behavior when cursor is uninitialized.
	 *
	 * FindBugs whines about us ignoring the return value from hasNext().
	 */
        hasNext();
    }

    /**
     * Returns the number of elements having the same key value as the key
     * value of the element last returned by next() or previous().  If no
     * duplicates are allowed, 1 is always returned.
     * This method does not exist in the standard {@link Iterator} or {@link
     * ListIterator} interfaces.
     *
     * @return the number of duplicates.
     *
     * @throws IllegalStateException if next() or previous() has not been
     * called for this iterator, or if remove() or add() were called after
     * the last call to next() or previous().
     */
    public int count() {

        if (!setAndRemoveAllowed) throw new IllegalStateException();
        try {
            moveToCurrent();
            return cursor.count();
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Closes this iterator.
     * This method does not exist in the standard {@link Iterator} or {@link
     * ListIterator} interfaces.
     *
     * <p>After being closed, only the {@link #hasNext} and {@link
     * #hasPrevious} methods may be called and these will return false.  {@link
     * #close()} may also be called again and will do nothing.  If other
     * methods are called a <code>NullPointerException</code> will generally be
     * thrown.</p>
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public void close() {

        if (cursor != null) {
            coll.closeCursor(cursor);
            cursor = null;
        }
    }

    /**
     * Returns the collection associated with this iterator.
     * This method does not exist in the standard {@link Iterator} or {@link
     * ListIterator} interfaces.
     *
     * @return the collection associated with this iterator.
     */
    public final StoredCollection<E> getCollection() {

        return coll;
    }

    // --- begin BaseIterator methods ---

    /**
     * Internal use only.
     */
    public final ListIterator<E> dup() {

        try {
            StoredIterator o = (StoredIterator) super.clone();
            o.cursor = cursor.cloneCursor();
            return o;
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * Internal use only.
     */
    public final boolean isCurrentData(Object currentData) {

        return (this.currentData == currentData);
    }

    /**
     * Internal use only.
     */
    public final boolean moveToIndex(int index) {

        try {
            OperationStatus status =
                cursor.getSearchKey(Integer.valueOf(index),
                                    null, lockForWrite);
            setAndRemoveAllowed = (status == OperationStatus.SUCCESS);
            return setAndRemoveAllowed;
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    // --- end BaseIterator methods ---

    private void moveToCurrent()
        throws DatabaseException {

        if (toCurrent != 0) {
            move(toCurrent);
            toCurrent = 0;
        }
    }

    private OperationStatus move(int direction)
        throws DatabaseException {

        switch (direction) {
            case MOVE_NEXT:
                if (coll.iterateDuplicates()) {
                    return cursor.getNext(lockForWrite);
                } else {
                    return cursor.getNextNoDup(lockForWrite);
                }
            case MOVE_PREV:
                if (coll.iterateDuplicates()) {
                    return cursor.getPrev(lockForWrite);
                } else {
                    return cursor.getPrevNoDup(lockForWrite);
                }
            case MOVE_FIRST:
                return cursor.getFirst(lockForWrite);
            default:
                throw new IllegalArgumentException(String.valueOf(direction));
        }
    }
}
