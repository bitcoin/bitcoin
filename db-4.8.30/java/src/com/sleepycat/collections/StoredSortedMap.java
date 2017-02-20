/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.util.Comparator;
import java.util.SortedMap;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.Database;
import com.sleepycat.db.OperationStatus;

/**
 * A SortedMap view of a {@link Database}.
 *
 * <p>In addition to the standard SortedMap methods, this class provides the
 * following methods for stored sorted maps only.  Note that the use of these
 * methods is not compatible with the standard Java collections interface.</p>
 * <ul>
 * <li>{@link #headMap(Object, boolean)}</li>
 * <li>{@link #tailMap(Object, boolean)}</li>
 * <li>{@link #subMap(Object, boolean, Object, boolean)}</li>
 * </ul>
 *
 * @author Mark Hayes
 */
public class StoredSortedMap<K,V>
    extends StoredMap<K,V>
    implements SortedMap<K,V> {

    /**
     * Creates a sorted map view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param keyBinding is the binding used to translate between key buffers
     * and key objects.
     *
     * @param valueBinding is the binding used to translate between value
     * buffers and value objects.
     *
     * @param writeAllowed is true to create a read-write collection or false
     * to create a read-only collection.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public StoredSortedMap(Database database,
                           EntryBinding<K> keyBinding,
                           EntryBinding<V> valueBinding,
                           boolean writeAllowed) {

        super(new DataView(database, keyBinding, valueBinding, null,
                           writeAllowed, null));
    }

    /**
     * Creates a sorted map view of a {@link Database} with a {@link
     * PrimaryKeyAssigner}.  Writing is allowed for the created map.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param keyBinding is the binding used to translate between key buffers
     * and key objects.
     *
     * @param valueBinding is the binding used to translate between value
     * buffers and value objects.
     *
     * @param keyAssigner is used by the {@link #append} method to assign
     * primary keys.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public StoredSortedMap(Database database,
                           EntryBinding<K> keyBinding,
                           EntryBinding<V> valueBinding,
                           PrimaryKeyAssigner keyAssigner) {

        super(new DataView(database, keyBinding, valueBinding, null,
                           true, keyAssigner));
    }

    /**
     * Creates a sorted map entity view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param keyBinding is the binding used to translate between key buffers
     * and key objects.
     *
     * @param valueEntityBinding is the binding used to translate between
     * key/value buffers and entity value objects.
     *
     * @param writeAllowed is true to create a read-write collection or false
     * to create a read-only collection.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public StoredSortedMap(Database database,
                           EntryBinding<K> keyBinding,
                           EntityBinding<V> valueEntityBinding,
                           boolean writeAllowed) {

        super(new DataView(database, keyBinding, null, valueEntityBinding,
                           writeAllowed, null));
    }

    /**
     * Creates a sorted map entity view of a {@link Database} with a {@link
     * PrimaryKeyAssigner}.  Writing is allowed for the created map.
     *
     * @param database is the Database underlying the new collection.
     *
     * @param keyBinding is the binding used to translate between key buffers
     * and key objects.
     *
     * @param valueEntityBinding is the binding used to translate between
     * key/value buffers and entity value objects.
     *
     * @param keyAssigner is used by the {@link #append} method to assign
     * primary keys.
     *
     * @throws IllegalArgumentException if formats are not consistently
     * defined or a parameter is invalid.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public StoredSortedMap(Database database,
                           EntryBinding<K> keyBinding,
                           EntityBinding<V> valueEntityBinding,
                           PrimaryKeyAssigner keyAssigner) {

        super(new DataView(database, keyBinding, null, valueEntityBinding,
                           true, keyAssigner));
    }

    StoredSortedMap(DataView mapView) {

        super(mapView);
    }

    /**
     * Returns null since comparators are not supported.  The natural ordering
     * of a stored collection is data byte order, whether the data classes
     * implement the {@link java.lang.Comparable} interface or not.
     * This method does not conform to the {@link SortedMap#comparator}
     * interface.
     *
     * @return null.
     */
    public Comparator<? super K> comparator() {

        return null;
    }

    /**
     * Returns the first (lowest) key currently in this sorted map.
     * This method conforms to the {@link SortedMap#firstKey} interface.
     *
     * @return the first key.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public K firstKey() {

        return getFirstOrLastKey(true);
    }

    /**
     * Returns the last (highest) element currently in this sorted map.
     * This method conforms to the {@link SortedMap#lastKey} interface.
     *
     * @return the last key.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public K lastKey() {

        return getFirstOrLastKey(false);
    }

    private K getFirstOrLastKey(boolean doGetFirst) {

        DataCursor cursor = null;
        try {
            cursor = new DataCursor(view, false);
            OperationStatus status;
            if (doGetFirst) {
                status = cursor.getFirst(false);
            } else {
                status = cursor.getLast(false);
            }
            return (K) ((status == OperationStatus.SUCCESS) ?
                        cursor.getCurrentKey() : null);
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        } finally {
            closeCursor(cursor);
        }
    }

    /**
     * Returns a view of the portion of this sorted set whose keys are
     * strictly less than toKey.
     * This method conforms to the {@link SortedMap#headMap} interface.
     *
     * <p>Note that the return value is a StoredStoredMap and must be treated
     * as such; for example, its iterators must be explicitly closed.</p>
     *
     * @param toKey is the upper bound.
     *
     * @return the submap.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedMap<K,V> headMap(K toKey) {

        return subMap(null, false, toKey, false);
    }

    /**
     * Returns a view of the portion of this sorted map whose elements are
     * strictly less than toKey, optionally including toKey.
     * This method does not exist in the standard {@link SortedMap} interface.
     *
     * <p>Note that the return value is a StoredStoredMap and must be treated
     * as such; for example, its iterators must be explicitly closed.</p>
     *
     * @param toKey is the upper bound.
     *
     * @param toInclusive is true to include toKey.
     *
     * @return the submap.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedMap<K,V> headMap(K toKey, boolean toInclusive) {

        return subMap(null, false, toKey, toInclusive);
    }

    /**
     * Returns a view of the portion of this sorted map whose elements are
     * greater than or equal to fromKey.
     * This method conforms to the {@link SortedMap#tailMap} interface.
     *
     * <p>Note that the return value is a StoredStoredMap and must be treated
     * as such; for example, its iterators must be explicitly closed.</p>
     *
     * @param fromKey is the lower bound.
     *
     * @return the submap.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedMap<K,V> tailMap(K fromKey) {

        return subMap(fromKey, true, null, false);
    }

    /**
     * Returns a view of the portion of this sorted map whose elements are
     * strictly greater than fromKey, optionally including fromKey.
     * This method does not exist in the standard {@link SortedMap} interface.
     *
     * <p>Note that the return value is a StoredStoredMap and must be treated
     * as such; for example, its iterators must be explicitly closed.</p>
     *
     * @param fromKey is the lower bound.
     *
     * @param fromInclusive is true to include fromKey.
     *
     * @return the submap.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedMap<K,V> tailMap(K fromKey, boolean fromInclusive) {

        return subMap(fromKey, fromInclusive, null, false);
    }

    /**
     * Returns a view of the portion of this sorted map whose elements range
     * from fromKey, inclusive, to toKey, exclusive.
     * This method conforms to the {@link SortedMap#subMap} interface.
     *
     * <p>Note that the return value is a StoredStoredMap and must be treated
     * as such; for example, its iterators must be explicitly closed.</p>
     *
     * @param fromKey is the lower bound.
     *
     * @param toKey is the upper bound.
     *
     * @return the submap.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedMap<K,V> subMap(K fromKey, K toKey) {

        return subMap(fromKey, true, toKey, false);
    }

    /**
     * Returns a view of the portion of this sorted map whose elements are
     * strictly greater than fromKey and strictly less than toKey,
     * optionally including fromKey and toKey.
     * This method does not exist in the standard {@link SortedMap} interface.
     *
     * <p>Note that the return value is a StoredStoredMap and must be treated
     * as such; for example, its iterators must be explicitly closed.</p>
     *
     * @param fromKey is the lower bound.
     *
     * @param fromInclusive is true to include fromKey.
     *
     * @param toKey is the upper bound.
     *
     * @param toInclusive is true to include toKey.
     *
     * @return the submap.
     *
     * @throws RuntimeExceptionWrapper if a {@link
     * com.sleepycat.db.DatabaseException} is thrown.
     */
    public SortedMap<K,V> subMap(K fromKey,
                                 boolean fromInclusive,
                                 K toKey,
                                 boolean toInclusive) {
        try {
            return new StoredSortedMap(
               view.subView(fromKey, fromInclusive, toKey, toInclusive, null));
        } catch (Exception e) {
            throw StoredContainer.convertException(e);
        }
    }
}
