/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.EntryBinding;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.JoinCursor;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.Transaction;

/**
 * Performs an equality join on two or more secondary keys.
 *
 * <p>{@code EntityJoin} objects are thread-safe.  Multiple threads may safely
 * call the methods of a shared {@code EntityJoin} object.</p>
 *
 * <p>An equality join is a match on all entities in a given primary index that
 * have two or more specific secondary key values.  Note that key ranges may
 * not be matched by an equality join, only exact keys are matched.</p>
 *
 * <p>For example:</p>
 * <pre class="code">
 *  // Index declarations -- see {@link <a href="package-summary.html#example">package summary example</a>}.
 *  //
 *  {@literal PrimaryIndex<String,Person> personBySsn;}
 *  {@literal SecondaryIndex<String,String,Person> personByParentSsn;}
 *  {@literal SecondaryIndex<Long,String,Person> personByEmployerIds;}
 *  Employer employer = ...;
 *
 *  // Match on all Person objects having parentSsn "111-11-1111" and also
 *  // containing an employerId of employer.id.  In other words, match on all
 *  // of Bob's children that work for a given employer.
 *  //
 *  {@literal EntityJoin<String,Person> join = new EntityJoin(personBySsn);}
 *  join.addCondition(personByParentSsn, "111-11-1111");
 *  join.addCondition(personByEmployerIds, employer.id);
 *
 *  // Perform the join operation by traversing the results with a cursor.
 *  //
 *  {@literal ForwardCursor<Person> results = join.entities();}
 *  try {
 *      for (Person person : results) {
 *          System.out.println(person.ssn + ' ' + person.name);
 *      }
 *  } finally {
 *      results.close();
 *  }</pre>
 *
 * @author Mark Hayes
 */
public class EntityJoin<PK,E> {

    private PrimaryIndex<PK,E> primary;
    private List<Condition> conditions;

    /**
     * Creates a join object for a given primary index.
     *
     * @param index the primary index on which the join will operate.
     */
    public EntityJoin(PrimaryIndex<PK,E> index) {
        primary = index;
        conditions = new ArrayList<Condition>();
    }

    /**
     * Adds a secondary key condition to the equality join.  Only entities
     * having the given key value in the given secondary index will be returned
     * by the join operation.
     *
     * @param index the secondary index containing the given key value.
     *
     * @param key the key value to match during the join.
     */
    public <SK> void addCondition(SecondaryIndex<SK,PK,E> index, SK key) {

        /* Make key entry. */
        DatabaseEntry keyEntry = new DatabaseEntry();
        index.getKeyBinding().objectToEntry(key, keyEntry);

        /* Use keys database if available. */
        Database db = index.getKeysDatabase();
        if (db == null) {
            db = index.getDatabase();
        }

        /* Add condition. */
        conditions.add(new Condition(db, keyEntry));
    }

    /**
     * Opens a cursor that returns the entities qualifying for the join.  The
     * join operation is performed as the returned cursor is accessed.
     *
     * <p>The operations performed with the cursor will not be transaction
     * protected, and {@link CursorConfig#DEFAULT} is used implicitly.</p>
     *
     * @return the cursor.
     *
     * @throws IllegalStateException if less than two conditions were added.
     */
    public ForwardCursor<E> entities()
        throws DatabaseException {

        return entities(null, null);
    }

    /**
     * Opens a cursor that returns the entities qualifying for the join.  The
     * join operation is performed as the returned cursor is accessed.
     *
     * @param txn the transaction used to protect all operations performed with
     * the cursor, or null if the operations should not be transaction
     * protected.
     *
     * @param config the cursor configuration that determines the default lock
     * mode used for all cursor operations, or null to implicitly use {@link
     * CursorConfig#DEFAULT}.
     *
     * @return the cursor.
     *
     * @throws IllegalStateException if less than two conditions were added.
     */
    public ForwardCursor<E> entities(Transaction txn, CursorConfig config)
        throws DatabaseException {

        return new JoinForwardCursor<E>(txn, config, false);
    }

    /**
     * Opens a cursor that returns the primary keys of entities qualifying for
     * the join.  The join operation is performed as the returned cursor is
     * accessed.
     *
     * <p>The operations performed with the cursor will not be transaction
     * protected, and {@link CursorConfig#DEFAULT} is used implicitly.</p>
     *
     * @return the cursor.
     *
     * @throws IllegalStateException if less than two conditions were added.
     */
    public ForwardCursor<PK> keys()
        throws DatabaseException {

        return keys(null, null);
    }

    /**
     * Opens a cursor that returns the primary keys of entities qualifying for
     * the join.  The join operation is performed as the returned cursor is
     * accessed.
     *
     * @param txn the transaction used to protect all operations performed with
     * the cursor, or null if the operations should not be transaction
     * protected.
     *
     * @param config the cursor configuration that determines the default lock
     * mode used for all cursor operations, or null to implicitly use {@link
     * CursorConfig#DEFAULT}.
     *
     * @return the cursor.
     *
     * @throws IllegalStateException if less than two conditions were added.
     */
    public ForwardCursor<PK> keys(Transaction txn, CursorConfig config)
        throws DatabaseException {

        return new JoinForwardCursor<PK>(txn, config, true);
    }

    private static class Condition {

        private Database db;
        private DatabaseEntry key;

        Condition(Database db, DatabaseEntry key) {
            this.db = db;
            this.key = key;
        }

        Cursor openCursor(Transaction txn, CursorConfig config)
            throws DatabaseException {

            OperationStatus status;
            Cursor cursor = db.openCursor(txn, config);
            try {
                DatabaseEntry data = BasicIndex.NO_RETURN_ENTRY;
                status = cursor.getSearchKey(key, data, null);
            } catch (DatabaseException e) {
                try {
                    cursor.close();
                } catch (DatabaseException ignored) {}
                throw e;
            }
            if (status == OperationStatus.SUCCESS) {
                return cursor;
            } else {
                cursor.close();
                return null;
            }
        }
    }

    private class JoinForwardCursor<V> implements ForwardCursor<V> {

        private Cursor[] cursors;
        private JoinCursor joinCursor;
        private boolean doKeys;

        JoinForwardCursor(Transaction txn, CursorConfig config, boolean doKeys)
            throws DatabaseException {

            this.doKeys = doKeys;
            try {
                cursors = new Cursor[conditions.size()];
                for (int i = 0; i < cursors.length; i += 1) {
                    Condition cond = conditions.get(i);
                    Cursor cursor = cond.openCursor(txn, config);
                    if (cursor == null) {
                        /* Leave joinCursor null. */
                        doClose(null);
                        return;
                    }
                    cursors[i] = cursor;
                }
                joinCursor = primary.getDatabase().join(cursors, null);
            } catch (DatabaseException e) {
                /* doClose will throw e. */
                doClose(e);
            }
        }

        public V next()
            throws DatabaseException {

            return next(null);
        }

        public V next(LockMode lockMode)
            throws DatabaseException {

            if (joinCursor == null) {
                return null;
            }
            if (doKeys) {
                DatabaseEntry key = new DatabaseEntry();
                OperationStatus status = joinCursor.getNext(key, lockMode);
                if (status == OperationStatus.SUCCESS) {
                    EntryBinding binding = primary.getKeyBinding();
                    return (V) binding.entryToObject(key);
                }
            } else {
                DatabaseEntry key = new DatabaseEntry();
                DatabaseEntry data = new DatabaseEntry();
                OperationStatus status =
                    joinCursor.getNext(key, data, lockMode);
                if (status == OperationStatus.SUCCESS) {
                    EntityBinding binding = primary.getEntityBinding();
                    return (V) binding.entryToObject(key, data);
                }
            }
            return null;
        }

        public Iterator<V> iterator() {
            return iterator(null);
        }

        public Iterator<V> iterator(LockMode lockMode) {
            return new BasicIterator<V>(this, lockMode);
        }

        public void close()
            throws DatabaseException {

            doClose(null);
        }

        private void doClose(DatabaseException firstException)
            throws DatabaseException {

            if (joinCursor != null) {
                try {
                    joinCursor.close();
                    joinCursor = null;
                } catch (DatabaseException e) {
                    if (firstException == null) {
                        firstException = e;
                    }
                }
            }
            for (int i = 0; i < cursors.length; i += 1) {
                Cursor cursor = cursors[i];
                if (cursor != null) {
                    try {
                        cursor.close();
                        cursors[i] = null;
                    } catch (DatabaseException e) {
                        if (firstException == null) {
                            firstException = e;
                        }
                    }
                }
            }
            if (firstException != null) {
                throw firstException;
            }
        }
    }
}
