/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.util.Set;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.OperationStatus;

/**
 * The Set returned by Map.values() and Map.duplicates(), and which can also be
 * constructed directly if a Map is not needed.
 * Although this collection is a set it may contain duplicate values.  Only if
 * an entity value binding is used are all elements guaranteed to be unique.
 *
 * @author Mark Hayes
 */
public class StoredValueSet<E> extends StoredCollection<E> implements Set<E> {

    /*
     * This class is also used internally for the set returned by duplicates().
     */

    /**
     * Creates a value set view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
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
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public StoredValueSet(Database database,
                          EntryBinding<E> valueBinding,
                          boolean writeAllowed) {

        super(new DataView(database, null, valueBinding, null,
                           writeAllowed, null));
    }

    /**
     * Creates a value set entity view of a {@link Database}.
     *
     * @param database is the Database underlying the new collection.
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
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public StoredValueSet(Database database,
                          EntityBinding<E> valueEntityBinding,
                          boolean writeAllowed) {

        super(new DataView(database, null, null, valueEntityBinding,
                           writeAllowed, null));
    }

    StoredValueSet(DataView valueSetView) {

        super(valueSetView);
    }

    /**
     * Adds the specified entity to this set if it is not already present
     * (optional operation).
     * This method conforms to the {@link Set#add} interface.
     *
     * @param entity is the entity to be added.
     *
     * @return true if the entity was added, that is the key-value pair
     * represented by the entity was not previously present in the collection.
     *
     * @throws UnsupportedOperationException if the collection is read-only,
     * if the collection is indexed, or if an entity binding is not used.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean add(E entity) {

        if (view.isSecondary()) {
            throw new UnsupportedOperationException(
                "add() not allowed with index");
        } else if (view.range.isSingleKey()) {
            /* entity is actually just a value in this case */
            if (!view.dupsAllowed) {
                throw new UnsupportedOperationException("duplicates required");
            }
            DataCursor cursor = null;
            boolean doAutoCommit = beginAutoCommit();
            try {
                cursor = new DataCursor(view, true);
                cursor.useRangeKey();
                OperationStatus status =
                    cursor.putNoDupData(null, entity, null, true);
                closeCursor(cursor);
                commitAutoCommit(doAutoCommit);
                return (status == OperationStatus.SUCCESS);
            } catch (Exception e) {
                closeCursor(cursor);
                throw handleException(e, doAutoCommit);
            }
        } else if (view.entityBinding == null) {
            throw new UnsupportedOperationException(
                "add() requires entity binding");
        } else {
            return add(null, entity);
        }
    }

    /**
     * Returns true if this set contains the specified element.
     * This method conforms to the {@link java.util.Set#contains}
     * interface.
     *
     * @param value the value to check.
     *
     * @return whether the set contains the given value.
     */
    public boolean contains(Object value) {

        return containsValue(value);
    }

    /**
     * Removes the specified value from this set if it is present (optional
     * operation).
     * If an entity binding is used, the key-value pair represented by the
     * given entity is removed.  If an entity binding is used, the first
     * occurrence of a key-value pair with the given value is removed.
     * This method conforms to the {@link Set#remove} interface.
     *
     * @throws UnsupportedOperationException if the collection is read-only.
     *
     * @throws RuntimeExceptionWrapper if a {@link DatabaseException} is
     * thrown.
     */
    public boolean remove(Object value) {

        return removeValue(value);
    }

    E makeIteratorData(BaseIterator iterator,
                       DatabaseEntry keyEntry,
                       DatabaseEntry priKeyEntry,
                       DatabaseEntry valueEntry) {

        return (E) view.makeValue(priKeyEntry, valueEntry);
    }

    boolean hasValues() {

        return true;
    }
}
