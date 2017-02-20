/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.JoinConfig;
import com.sleepycat.db.OperationStatus;

/**
 * A abstract base class for all stored collections.  This class, and its
 * base class {@link StoredContainer}, provide implementations of most methods
 * in the {@link Collection} interface.  Other methods, such as {@link #add}
 * and {@link #remove}, are provided by concrete classes that extend this
 * class.
 *
 * <p>In addition, this class provides the following methods for stored
 * collections only.  Note that the use of these methods is not compatible with
 * the standard Java collections interface.</p>
 * <ul>
 * <li>{@link #getIteratorBlockSize}</li>
 * <li>{@link #setIteratorBlockSize}</li>
 * <li>{@link #storedIterator()}</li>
 * <li>{@link #storedIterator(boolean)}</li>
 * <li>{@link #join}</li>
 * <li>{@link #toList()}</li>
 * </ul>
 *
 * @author Mark Hayes
 */
public abstract class StoredCollection<E> extends StoredContainer
    implements Collection<E> {

    /**
     * The default number of records read at one time by iterators.
     * @see #setIteratorBlockSize
     */
    public static final int DEFAULT_ITERATOR_BLOCK_SIZE = 10;

    private int iteratorBlockSize = DEFAULT_ITERATOR_BLOCK_SIZE;

    StoredCollection(DataView view) {

        super(view);
    }

    /**
     * Returns the number of records read at one time by iterators returned by
     * the {@link #iterator} method.  By default this value is {@link
     * #DEFAULT_ITERATOR_BLOCK_SIZE}.
     */
    public int getIteratorBlockSize() {

        return iteratorBlockSize;
    }

    /**
     * Changes the number of records read at one time by iterators returned by
     * the {@link #iterator} method.  By default this value is {@link
     * #DEFAULT_ITERATOR_BLOCK_SIZE}.
     *
     * @throws IllegalArgumentException if the blockSize is less than two.
     */
    public void setIteratorBlockSize(int blockSize) {

        if (blockSize < 2) {
            throw new IllegalArgumentException
                ("blockSize is less than two: " + blockSize);
        }

        iteratorBlockSize = blockSize;
    }

    final boolean add(Object key, Object value) {

        DataCursor cursor = null;
        boolean doAutoCommit = beginAutoCommit();
        try {
            cursor = new DataCursor(view, true);
            OperationStatus status =
                cursor.putNoDupData(key, value, null, false);
            closeCursor(cursor);
            commitAutoCommit(doAutoCommit);
            return (status == OperationStatus.SUCCESS);
        } catch (Exception e) {
            closeCursor(cursor);
            throw handleException(e, doAutoCommit);
        }
    }

    BlockIterator blockIterator() {
        return new BlockIterator(this, isWriteAllowed(), iteratorBlockSize);
    }

    /**
     * Returns an iterator over the elements in this collection.
     * The iterator will be read-only if the collection is read-only.
     * This method conforms to the {@link Collection#iterator} interface.
     *
     * <p>The iterator returned by this method does not keep a database cursor
     * open and therefore it does not need to be closed.  It reads blocks of
     * records as needed, opening and closing a cursor to read each block of
     * records.  The number of records per block is 10 by default and can be
     * changed with {@link #setIteratorBlockSize}.</p>
     *
     * <p>Because this iterator does not keep a cursor open, if it is used
     * without transactions, the iterator does not have <em>cursor
     * stability</em> characteristics.  In other words, the record at the
     * current iterator position can be changed or deleted by another thread.
     * To prevent this from happening, call this method within a transaction or
     * use the {@link #storedIterator()} method instead.</p>
     *
     * @return a standard {@link Iterator} for this collection.
     *
     * @see #isWriteAllowed
     */
    public Iterator<E> iterator() {
        return blockIterator();
    }

    /**
     * Returns an iterator over the elements in this collection.
     * The iterator will be read-only if the collection is read-only.
     * This method does not exist in the standard {@link Collection} interface.
     *
     * <p>If {@code Iterator.set} or {@code Iterator.remove} will be called
     * and the underlying Database is transactional, then a transaction must be
     * active when calling this method and must remain active while using the
     * iterator.</p>
     *
     * <p><strong>Warning:</strong> The iterator returned must be explicitly
     * closed using {@link StoredIterator#close()} or {@link
     * StoredIterator#close(java.util.Iterator)} to release the underlying
     * database cursor resources.</p>
     *
     * @return a {@link StoredIterator} for this collection.
     *
     * @see #isWriteAllowed
     */
    public StoredIterator<E> storedIterator() {

        return storedIterator(isWriteAllowed());
    }

    /**
     * Returns a read or read-write iterator over the elements in this
     * collection.
     * This method does not exist in the standard {@link Collection} interface.
     *
     * <p>If {@code Iterator.set} or {@code Iterator.remove} will be called
     * and the underlying Database is transactional, then a transaction must be
     * active when calling this method and must remain active while using the
     * iterator.</p>
     *
     * <p><strong>Warning:</strong> The iterator returned must be explicitly
     * closed using {@link StoredIterator#close()} or {@link
     * StoredIterator#close(java.util.Iterator)} to release the underlying
     * database cursor resources.</p>
     *
     * @param writeAllowed is true to open a read-write iterator or false to
     * open a read-only iterator.  If the collection is read-only the iterator
     * will always be read-only.
     *
     * @return a {@link StoredIterator} for this collection.
     *
     * @throws IllegalStateException if writeAllowed is true but the collection
     * is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     *
     * @see #isWriteAllowed
     */
    public StoredIterator<E> storedIterator(boolean writeAllowed) {

        try {
            return new StoredIterator(this, writeAllowed && isWriteAllowed(),
                                      null);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    /**
     * @deprecated Please use {@link #storedIterator()} or {@link
     * #storedIterator(boolean)} instead.  Because the iterator returned must
     * be closed, the method name {@code iterator} is confusing since standard
     * Java iterators do not need to be closed.
     */
    public StoredIterator<E> iterator(boolean writeAllowed) {

        return storedIterator(writeAllowed);
    }

    /**
     * Returns an array of all the elements in this collection.
     * This method conforms to the {@link Collection#toArray()} interface.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public Object[] toArray() {

        ArrayList<Object> list = new ArrayList<Object>();
        StoredIterator i = storedIterator();
        try {
            while (i.hasNext()) {
                list.add(i.next());
            }
        } finally {
            i.close();
        }
        return list.toArray();
    }

    /**
     * Returns an array of all the elements in this collection whose runtime
     * type is that of the specified array.
     * This method conforms to the {@link Collection#toArray(Object[])}
     * interface.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public <T> T[] toArray(T[] a) {

        int j = 0;
        StoredIterator i = storedIterator();
        try {
            while (j < a.length && i.hasNext()) {
                a[j++] = (T) i.next();
            }
            if (j < a.length) {
                a[j] = null;
            } else if (i.hasNext()) {
                ArrayList<T> list = new ArrayList<T>(Arrays.asList(a));
                while (i.hasNext()) {
                    list.add((T) i.next());
                }
                a = list.toArray(a);
            }
        } finally {
            i.close();
        }
        return a;
    }

    /**
     * Returns true if this collection contains all of the elements in the
     * specified collection.
     * This method conforms to the {@link Collection#containsAll} interface.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean containsAll(Collection<?> coll) {
	Iterator<?> i = storedOrExternalIterator(coll);
        try {
            while (i.hasNext()) {
                if (!contains(i.next())) {
                    return false;
                }
            }
        } finally {
            StoredIterator.close(i);
        }
	return true;
    }

    /**
     * Adds all of the elements in the specified collection to this collection
     * (optional operation).
     * This method calls the {@link #add(Object)} method of the concrete
     * collection class, which may or may not be supported.
     * This method conforms to the {@link Collection#addAll} interface.
     *
     * @throws UnsupportedOperationException if the collection is read-only, or
     * if the collection is indexed, or if the add method is not supported by
     * the concrete collection.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean addAll(Collection<? extends E> coll) {
	Iterator<? extends E> i = null;
        boolean doAutoCommit = beginAutoCommit();
        try {
            i = storedOrExternalIterator(coll);
            boolean changed = false;
            while (i.hasNext()) {
                if (add(i.next())) {
                    changed = true;
                }
            }
            StoredIterator.close(i);
            commitAutoCommit(doAutoCommit);
            return changed;
        } catch (Exception e) {
            StoredIterator.close(i);
            throw handleException(e, doAutoCommit);
        }
    }

    /**
     * Removes all this collection's elements that are also contained in the
     * specified collection (optional operation).
     * This method conforms to the {@link Collection#removeAll} interface.
     *
     * @throws UnsupportedOperationException if the collection is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean removeAll(Collection<?> coll) {

        return removeAll(coll, true);
    }

    /**
     * Retains only the elements in this collection that are contained in the
     * specified collection (optional operation).
     * This method conforms to the {@link Collection#removeAll} interface.
     *
     * @throws UnsupportedOperationException if the collection is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean retainAll(Collection<?> coll) {

        return removeAll(coll, false);
    }

    private boolean removeAll(Collection<?> coll, boolean ifExistsInColl) {
	StoredIterator i = null;
        boolean doAutoCommit = beginAutoCommit();
        try {
            boolean changed = false;
            i = storedIterator();
            while (i.hasNext()) {
                if (ifExistsInColl == coll.contains(i.next())) {
                    i.remove();
                    changed = true;
                }
            }
            i.close();
            commitAutoCommit(doAutoCommit);
            return changed;
        } catch (Exception e) {
            if (i != null) {
                i.close();
            }
            throw handleException(e, doAutoCommit);
        }
    }

    /**
     * Compares the specified object with this collection for equality.
     * A value comparison is performed by this method and the stored values
     * are compared rather than calling the equals() method of each element.
     * This method conforms to the {@link Collection#equals} interface.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean equals(Object other) {

        if (other instanceof Collection) {
            Collection otherColl = StoredCollection.copyCollection(other);
            StoredIterator i = storedIterator();
            try {
                while (i.hasNext()) {
                    if (!otherColl.remove(i.next())) {
                        return false;
                    }
                }
                return otherColl.isEmpty();
            } finally {
                i.close();
            }
        } else {
            return false;
        }
    }

    /*
     * Add this in to keep FindBugs from whining at us about implementing
     * equals(), but not hashCode().
     */
    public int hashCode() {
	return super.hashCode();
    }

    /**
     * Returns a copy of this collection as an ArrayList.  This is the same as
     * {@link #toArray()} but returns a collection instead of an array.
     *
     * @return an {@link ArrayList} containing a copy of all elements in this
     * collection.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public List<E> toList() {

        ArrayList<E> list = new ArrayList<E>();
        StoredIterator<E> i = storedIterator();
        try {
            while (i.hasNext()) list.add(i.next());
            return list;
        } finally {
            i.close();
        }
    }

    /**
     * Converts the collection to a string representation for debugging.
     * WARNING: The returned string may be very large.
     *
     * @return the string representation.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public String toString() {
	StringBuffer buf = new StringBuffer();
	buf.append("[");
	StoredIterator i = storedIterator();
        try {
            while (i.hasNext()) {
                if (buf.length() > 1) buf.append(',');
                buf.append(i.next().toString());
            }
            buf.append(']');
            return buf.toString();
        } finally {
            i.close();
        }
    }

    // Inherit javadoc
    public int size() {

        boolean countDups = iterateDuplicates();
        if (DbCompat.DATABASE_COUNT && countDups && !view.range.hasBound()) {
            try {
                return (int) DbCompat.getDatabaseCount(view.db);
            } catch (Exception e) {
                throw StoredContainer.convertException(e);
            }
        } else {
            int count = 0;
            CursorConfig cursorConfig = view.currentTxn.isLockingMode() ?
                CursorConfig.READ_UNCOMMITTED : null;
            DataCursor cursor = null;
            try {
                cursor = new DataCursor(view, false, cursorConfig);
                OperationStatus status = cursor.getFirst(false);
                while (status == OperationStatus.SUCCESS) {
                    if (countDups) {
                        count += cursor.count();
                    } else {
                        count += 1;
                    }
                    status = cursor.getNextNoDup(false);
                }
            } catch (Exception e) {
                throw StoredContainer.convertException(e);
            } finally {
                closeCursor(cursor);
            }
            return count;
        }
    }

    /**
     * Returns an iterator representing an equality join of the indices and
     * index key values specified.
     * This method does not exist in the standard {@link Collection} interface.
     *
     * <p><strong>Warning:</strong> The iterator returned must be explicitly
     * closed using {@link StoredIterator#close()} or {@link
     * StoredIterator#close(java.util.Iterator)} to release the underlying
     * database cursor resources.</p>
     *
     * <p>The returned iterator supports only the two methods: hasNext() and
     * next().  All other methods will throw UnsupportedOperationException.</p>
     *
     * @param indices is an array of indices with elements corresponding to
     * those in the indexKeys array.
     *
     * @param indexKeys is an array of index key values identifying the
     * elements to be selected.
     *
     * @param joinConfig is the join configuration, or null to use the
     * default configuration.
     *
     * @return an iterator over the elements in this collection that match
     * all specified index key values.
     *
     * @throws IllegalArgumentException if this collection is indexed or if a
     * given index does not have the same store as this collection.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public StoredIterator<E> join(StoredContainer[] indices,
                                  Object[] indexKeys,
                                  JoinConfig joinConfig) {

        try {
            DataView[] indexViews = new DataView[indices.length];
            for (int i = 0; i < indices.length; i += 1) {
                indexViews[i] = indices[i].view;
            }
            DataCursor cursor = view.join(indexViews, indexKeys, joinConfig);
            return new StoredIterator<E>(this, false, cursor);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }

    final E getFirstOrLast(boolean doGetFirst) {

        DataCursor cursor = null;
        try {
            cursor = new DataCursor(view, false);
            OperationStatus status;
            if (doGetFirst) {
                status = cursor.getFirst(false);
            } else {
                status = cursor.getLast(false);
            }
            return (status == OperationStatus.SUCCESS) ?
                    makeIteratorData(null, cursor) : null;
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    E makeIteratorData(BaseIterator iterator, DataCursor cursor) {

        return makeIteratorData(iterator,
                                cursor.getKeyThang(),
                                cursor.getPrimaryKeyThang(),
                                cursor.getValueThang());
    }

    abstract E makeIteratorData(BaseIterator iterator,
                                DatabaseEntry keyEntry,
                                DatabaseEntry priKeyEntry,
                                DatabaseEntry valueEntry);

    abstract boolean hasValues();

    boolean iterateDuplicates() {

        return true;
    }

    void checkIterAddAllowed()
        throws UnsupportedOperationException {

        if (!areDuplicatesAllowed()) {
            throw new UnsupportedOperationException("duplicates required");
        }
    }

    int getIndexOffset() {

        return 0;
    }

    private static Collection copyCollection(Object other) {

        if (other instanceof StoredCollection) {
            return ((StoredCollection) other).toList();
        } else {
            return new ArrayList((Collection) other);
        }
    }
}
