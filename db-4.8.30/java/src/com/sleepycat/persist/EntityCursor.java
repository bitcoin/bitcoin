/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import java.util.Iterator;

import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.Transaction;
import com.sleepycat.persist.model.Relationship;
import com.sleepycat.persist.model.SecondaryKey;

/**
 * Traverses entity values or key values and allows deleting or updating the
 * entity at the current cursor position.  The value type (V) is either an
 * entity class or a key class, depending on how the cursor was opened.
 *
 * <p>{@code EntityCursor} objects are <em>not</em> thread-safe.  Cursors
 * should be opened, used and closed by a single thread.</p>
 *
 * <p>Cursors are opened using the {@link EntityIndex#keys} and {@link
 * EntityIndex#entities} family of methods.  These methods are available for
 * objects of any class that implements {@link EntityIndex}: {@link
 * PrimaryIndex}, {@link SecondaryIndex}, and the indices returned by {@link
 * SecondaryIndex#keysIndex} and {@link SecondaryIndex#subIndex}.  A {@link
 * ForwardCursor}, which implements a subset of cursor operations, is also
 * available via the {@link EntityJoin#keys} and {@link EntityJoin#entities}
 * methods.</p>
 *
 * <p>Values are always returned by a cursor in key order, where the key is
 * defined by the underlying {@link EntityIndex}.  For example, a cursor on a
 * {@link SecondaryIndex} returns values ordered by secondary key, while an
 * index on a {@link PrimaryIndex} or a {@link SecondaryIndex#subIndex} returns
 * values ordered by primary key.</p>
 *
 * <p><em>WARNING:</em> Cursors must always be closed to prevent resource leaks
 * which could lead to the index becoming unusable or cause an
 * <code>OutOfMemoryError</code>.  To ensure that a cursor is closed in the
 * face of exceptions, call {@link #close} in a finally block.  For example,
 * the following code traverses all Employee entities and closes the cursor
 * whether or not an exception occurs:</p>
 *
 * <pre class="code">
 * {@literal @Entity}
 * class Employee {
 *
 *     {@literal @PrimaryKey}
 *     long id;
 *
 *     {@literal @SecondaryKey(relate=MANY_TO_ONE)}
 *     String department;
 *
 *     String name;
 *
 *     private Employee() {}
 * }
 *
 * EntityStore store = ...
 *
 * {@code PrimaryIndex<Long,Employee>} primaryIndex =
 *     store.getPrimaryIndex(Long.class, Employee.class);
 *
 * {@code EntityCursor<Employee>} cursor = primaryIndex.entities();
 * try {
 *     for (Employee entity = cursor.first();
 *                   entity != null;
 *                   entity = cursor.next()) {
 *         // Do something with the entity...
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <h3>Initializing the Cursor Position</h3>
 *
 * <p>When it is opened, a cursor is not initially positioned on any value; in
 * other words, it is uninitialized.  Most methods in this interface initialize
 * the cursor position but certain methods, for example, {@link #current} and
 * {@link #delete}, throw {@link IllegalStateException} when called for an
 * uninitialized cursor.</p>
 *
 * <p>Note that the {@link #next} and {@link #prev} methods return the first or
 * last value respectively for an uninitialized cursor.  This allows the loop
 * in the example above to be rewritten as follows:</p>
 *
 * <pre class="code">
 * {@code EntityCursor<Employee>} cursor = primaryIndex.entities();
 * try {
 *     Employee entity;
 *     while ((entity = cursor.next()) != null) {
 *         // Do something with the entity...
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <h3>Cursors and Iterators</h3>
 *
 * <p>The {@link #iterator} method can be used to return a standard Java {@code
 * Iterator} that returns the same values that the cursor returns.  For
 * example:</p>
 *
 * <pre class="code">
 * {@code EntityCursor<Employee>} cursor = primaryIndex.entities();
 * try {
 *     {@code Iterator<Employee>} i = cursor.iterator();
 *     while (i.hasNext()) {
 *          Employee entity = i.next();
 *         // Do something with the entity...
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <p>The {@link Iterable} interface is also extended by {@link EntityCursor}
 * to allow using the cursor as the target of a Java "foreach" statement:</p>
 *
 * <pre class="code">
 * {@code EntityCursor<Employee>} cursor = primaryIndex.entities();
 * try {
 *     for (Employee entity : cursor) {
 *         // Do something with the entity...
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <p>The iterator uses the cursor directly, so any changes to the cursor
 * position impact the iterator and vice versa.  The iterator advances the
 * cursor by calling {@link #next()} when {@link Iterator#hasNext} or {@link
 * Iterator#next} is called.  Because of this interaction, to keep things
 * simple it is best not to mix the use of an {@code EntityCursor}
 * {@code Iterator} with the use of the {@code EntityCursor} traversal methods
 * such as {@link #next()}, for a single {@code EntityCursor} object.</p>
 *
 * <h3>Key Ranges</h3>
 *
 * <p>A key range may be specified when opening the cursor, to restrict the
 * key range of the cursor to a subset of the complete range of keys in the
 * index.  A {@code fromKey} and/or {@code toKey} parameter may be specified
 * when calling {@link EntityIndex#keys(Object,boolean,Object,boolean)} or
 * {@link EntityIndex#entities(Object,boolean,Object,boolean)}.  The key
 * arguments may be specified as inclusive or exclusive values.</p>
 *
 * <p>Whenever a cursor with a key range is moved, the key range bounds will be
 * checked, and the cursor will never be positioned outside the range.  The
 * {@link #first} cursor value is the first existing value in the range, and
 * the {@link #last} cursor value is the last existing value in the range.  For
 * example, the following code traverses Employee entities with keys from 100
 * (inclusive) to 200 (exclusive):</p>
 *
 * <pre class="code">
 * {@code EntityCursor<Employee>} cursor = primaryIndex.entities(100, true, 200, false);
 * try {
 *     for (Employee entity : cursor) {
 *         // Do something with the entity...
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <h3>Duplicate Keys</h3>
 *
 * <p>When using a cursor for a {@link SecondaryIndex}, the keys in the index
 * may be non-unique (duplicates) if {@link SecondaryKey#relate} is {@link
 * Relationship#MANY_TO_ONE MANY_TO_ONE} or {@link Relationship#MANY_TO_MANY
 * MANY_TO_MANY}.  For example, a {@code MANY_TO_ONE} {@code
 * Employee.department} secondary key is non-unique because there are multiple
 * Employee entities with the same department key value.  The {@link #nextDup},
 * {@link #prevDup}, {@link #nextNoDup} and {@link #prevNoDup} methods may be
 * used to control how non-unique keys are returned by the cursor.</p>
 *
 * <p>{@link #nextDup} and {@link #prevDup} return the next or previous value
 * only if it has the same key as the current value, and null is returned when
 * a different key is encountered.  For example, these methods can be used to
 * return all employees in a given department.</p>
 *
 * <p>{@link #nextNoDup} and {@link #prevNoDup} return the next or previous
 * value with a unique key, skipping over values that have the same key.  For
 * example, these methods can be used to return the first employee in each
 * department.</p>
 *
 * <p>For example, the following code will find the first employee in each
 * department with {@link #nextNoDup} until it finds a department name that
 * matches a particular regular expression.  For each matching department it
 * will find all employees in that department using {@link #nextDup}.</p>
 *
 * <pre class="code">
 * {@code SecondaryIndex<String,Long,Employee>} secondaryIndex =
 *     store.getSecondaryIndex(primaryIndex, String.class, "department");
 *
 * String regex = ...;
 * {@code EntityCursor<Employee>} cursor = secondaryIndex.entities();
 * try {
 *     for (Employee entity = cursor.first();
 *                   entity != null;
 *                   entity = cursor.nextNoDup()) {
 *         if (entity.department.matches(regex)) {
 *             while (entity != null) {
 *                 // Do something with the matching entities...
 *                 entity = cursor.nextDup();
 *             }
 *         }
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <h3>Updating and Deleting Entities with a Cursor</h3>
 *
 * <p>The {@link #update} and {@link #delete} methods operate on the entity at
 * the current cursor position.  Cursors on any type of index may be used to
 * delete entities.  For example, the following code deletes all employees in
 * departments which have names that match a particular regular expression:</p>
 *
 * <pre class="code">
 * {@code SecondaryIndex<String,Long,Employee>} secondaryIndex =
 *     store.getSecondaryIndex(primaryIndex, String.class, "department");
 *
 * String regex = ...;
 * {@code EntityCursor<Employee>} cursor = secondaryIndex.entities();
 * try {
 *     for (Employee entity = cursor.first();
 *                   entity != null;
 *                   entity = cursor.nextNoDup()) {
 *         if (entity.department.matches(regex)) {
 *             while (entity != null) {
 *                 cursor.delete();
 *                 entity = cursor.nextDup();
 *             }
 *         }
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * <p>Note that the cursor can be moved to the next (or previous) value after
 * deleting the entity at the current position.  This is an important property
 * of cursors, since without it you would not be able to easily delete while
 * processing multiple values with a cursor.  A cursor positioned on a deleted
 * entity is in a special state.  In this state, {@link #current} will return
 * null, {@link #delete} will return false, and {@link #update} will return
 * false.</p>
 *
 * <p>The {@link #update} method is supported only if the value type is an
 * entity class (not a key class) and the underlying index is a {@link
 * PrimaryIndex}; in other words, for a cursor returned by one of the {@link
 * PrimaryIndex#entities} methods.  For example, the following code changes all
 * employee names to uppercase:</p>
 *
 * <pre class="code">
 * {@code EntityCursor<Employee>} cursor = primaryIndex.entities();
 * try {
 *     for (Employee entity = cursor.first();
 *                   entity != null;
 *                   entity = cursor.next()) {
 *         entity.name = entity.name.toUpperCase();
 *         cursor.update(entity);
 *     }
 * } finally {
 *     cursor.close();
 * }</pre>
 *
 * @author Mark Hayes
 */
public interface EntityCursor<V> extends ForwardCursor<V> {

    /**
     * Moves the cursor to the first value and returns it, or returns null if
     * the cursor range is empty.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the first value, or null if the cursor range is empty.
     */
    V first()
        throws DatabaseException;

    /**
     * Moves the cursor to the first value and returns it, or returns null if
     * the cursor range is empty.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the first value, or null if the cursor range is empty.
     */
    V first(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the last value and returns it, or returns null if
     * the cursor range is empty.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the last value, or null if the cursor range is empty.
     */
    V last()
        throws DatabaseException;

    /**
     * Moves the cursor to the last value and returns it, or returns null if
     * the cursor range is empty.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the last value, or null if the cursor range is empty.
     */
    V last(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the next value and returns it, or returns null
     * if there are no more values in the cursor range.  If the cursor is
     * uninitialized, this method is equivalent to {@link #first}.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the next value, or null if there are no more values in the
     * cursor range.
     */
    V next()
        throws DatabaseException;

    /**
     * Moves the cursor to the next value and returns it, or returns null
     * if there are no more values in the cursor range.  If the cursor is
     * uninitialized, this method is equivalent to {@link #first}.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the next value, or null if there are no more values in the
     * cursor range.
     */
    V next(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the next value with the same key (duplicate) and
     * returns it, or returns null if no more values are present for the key at
     * the current position.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the next value with the same key, or null if no more values are
     * present for the key at the current position.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    V nextDup()
        throws DatabaseException;

    /**
     * Moves the cursor to the next value with the same key (duplicate) and
     * returns it, or returns null if no more values are present for the key at
     * the current position.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the next value with the same key, or null if no more values are
     * present for the key at the current position.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    V nextDup(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the next value with a different key and returns it,
     * or returns null if there are no more unique keys in the cursor range.
     * If the cursor is uninitialized, this method is equivalent to {@link
     * #first}.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the next value with a different key, or null if there are no
     * more unique keys in the cursor range.
     */
    V nextNoDup()
        throws DatabaseException;

    /**
     * Moves the cursor to the next value with a different key and returns it,
     * or returns null if there are no more unique keys in the cursor range.
     * If the cursor is uninitialized, this method is equivalent to {@link
     * #first}.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the next value with a different key, or null if there are no
     * more unique keys in the cursor range.
     */
    V nextNoDup(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the previous value and returns it, or returns null
     * if there are no preceding values in the cursor range.  If the cursor is
     * uninitialized, this method is equivalent to {@link #last}.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the previous value, or null if there are no preceding values in
     * the cursor range.
     */
    V prev()
        throws DatabaseException;

    /**
     * Moves the cursor to the previous value and returns it, or returns null
     * if there are no preceding values in the cursor range.  If the cursor is
     * uninitialized, this method is equivalent to {@link #last}.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the previous value, or null if there are no preceding values in
     * the cursor range.
     */
    V prev(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the previous value with the same key (duplicate) and
     * returns it, or returns null if no preceding values are present for the
     * key at the current position.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the previous value with the same key, or null if no preceding
     * values are present for the key at the current position.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    V prevDup()
        throws DatabaseException;

    /**
     * Moves the cursor to the previous value with the same key (duplicate) and
     * returns it, or returns null if no preceding values are present for the
     * key at the current position.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the previous value with the same key, or null if no preceding
     * values are present for the key at the current position.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    V prevDup(LockMode lockMode)
        throws DatabaseException;

    /**
     * Moves the cursor to the preceding value with a different key and returns
     * it, or returns null if there are no preceding unique keys in the cursor
     * range.  If the cursor is uninitialized, this method is equivalent to
     * {@link #last}.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the previous value with a different key, or null if there are no
     * preceding unique keys in the cursor range.
     */
    V prevNoDup()
        throws DatabaseException;

    /**
     * Moves the cursor to the preceding value with a different key and returns
     * it, or returns null if there are no preceding unique keys in the cursor
     * range.  If the cursor is uninitialized, this method is equivalent to
     * {@link #last}.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the previous value with a different key, or null if there are no
     * preceding unique keys in the cursor range.
     */
    V prevNoDup(LockMode lockMode)
        throws DatabaseException;

    /**
     * Returns the value at the cursor position, or null if the value at the
     * cursor position has been deleted.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the value at the cursor position, or null if it has been
     * deleted.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    V current()
        throws DatabaseException;

    /**
     * Returns the value at the cursor position, or null if the value at the
     * cursor position has been deleted.
     *
     * @param lockMode the lock mode to use for this operation, or null to
     * use {@link LockMode#DEFAULT}.
     *
     * @return the value at the cursor position, or null if it has been
     * deleted.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    V current(LockMode lockMode)
        throws DatabaseException;

    /**
     * Returns the number of values (duplicates) for the key at the cursor
     * position, or returns zero if all values for the key have been deleted,
     * Returns one or zero if the underlying index has unique keys.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the number of duplicates, or zero if all values for the current
     * key have been deleted.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     */
    int count()
        throws DatabaseException;

    /**
     * Returns an iterator over the key range, starting with the value
     * following the current position or at the first value if the cursor is
     * uninitialized.
     *
     * <p>{@link LockMode#DEFAULT} is used implicitly.</p>
     *
     * @return the iterator.
     */
    Iterator<V> iterator();

    /**
     * Returns an iterator over the key range, starting with the value
     * following the current position or at the first value if the cursor is
     * uninitialized.
     *
     * @param lockMode the lock mode to use for all operations performed
     * using the iterator, or null to use {@link LockMode#DEFAULT}.
     *
     * @return the iterator.
     */
    Iterator<V> iterator(LockMode lockMode);

    /**
     * Replaces the entity at the cursor position with the given entity.
     *
     * @param entity the entity to replace the entity at the current position.
     *
     * @return true if successful or false if the entity at the current
     * position was previously deleted.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     *
     * @throws UnsupportedOperationException if the index is read only or if
     * the value type is not an entity type.
     */
    boolean update(V entity)
        throws DatabaseException;

    /**
     * Deletes the entity at the cursor position.
     *
     * @throws IllegalStateException if the cursor is uninitialized.
     *
     * @throws UnsupportedOperationException if the index is read only.
     *
     * @return true if successful or false if the entity at the current
     * position has been deleted.
     */
    boolean delete()
        throws DatabaseException;

    /**
     * Duplicates the cursor at the cursor position.  The returned cursor will
     * be initially positioned at the same position as this current cursor, and
     * will inherit this cursor's {@link Transaction} and {@link CursorConfig}.
     *
     * @return the duplicated cursor.
     */
    EntityCursor<V> dup()
        throws DatabaseException;

    /**
     * Closes the cursor.
     */
    void close()
        throws DatabaseException;


}
