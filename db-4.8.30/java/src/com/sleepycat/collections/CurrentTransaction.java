/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.WeakHashMap;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.Transaction;
import com.sleepycat.db.TransactionConfig;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * Provides access to the current transaction for the current thread within the
 * context of a Berkeley DB environment.  This class provides explicit
 * transaction control beyond that provided by the {@link TransactionRunner}
 * class.  However, both methods of transaction control manage per-thread
 * transactions.
 *
 * @author Mark Hayes
 */
public class CurrentTransaction {

    /* For internal use, this class doubles as an Environment wrapper. */

    private static WeakHashMap<Environment,CurrentTransaction> envMap =
        new WeakHashMap<Environment,CurrentTransaction>();

    private LockMode writeLockMode;
    private boolean cdbMode;
    private boolean txnMode;
    private boolean lockingMode;
    private ThreadLocal localTrans = new ThreadLocal();
    private ThreadLocal localCdbCursors;

    /*
     * Use a WeakReference to the Environment to avoid pinning the environment
     * in the envMap.  The WeakHashMap envMap uses the Environment as a weak
     * key, but this won't prevent GC of the Environment if the map's value has
     * a hard reference to the Environment.  [#15444]
     */
    private WeakReference<Environment> envRef;

    /**
     * Gets the CurrentTransaction accessor for a specified Berkeley DB
     * environment.  This method always returns the same reference when called
     * more than once with the same environment parameter.
     *
     * @param env is an open Berkeley DB environment.
     *
     * @return the CurrentTransaction accessor for the given environment, or
     * null if the environment is not transactional.
     */
    public static CurrentTransaction getInstance(Environment env) {

        CurrentTransaction currentTxn = getInstanceInternal(env);
        return currentTxn.isTxnMode() ? currentTxn : null;
    }

    /**
     * Gets the CurrentTransaction accessor for a specified Berkeley DB
     * environment.  Unlike getInstance(), this method never returns null.
     *
     * @param env is an open Berkeley DB environment.
     */
    static CurrentTransaction getInstanceInternal(Environment env) {
        synchronized (envMap) {
            CurrentTransaction ct = envMap.get(env);
            if (ct == null) {
                ct = new CurrentTransaction(env);
                envMap.put(env, ct);
            }
            return ct;
        }
    }

    private CurrentTransaction(Environment env) {
        envRef = new WeakReference<Environment>(env);
        try {
            EnvironmentConfig config = env.getConfig();
            txnMode = config.getTransactional();
            lockingMode = DbCompat.getInitializeLocking(config);
            if (txnMode || lockingMode) {
                writeLockMode = LockMode.RMW;
            } else {
                writeLockMode = LockMode.DEFAULT;
            }
            cdbMode = DbCompat.getInitializeCDB(config);
            if (cdbMode) {
                localCdbCursors = new ThreadLocal();
            }
        } catch (DatabaseException e) {
            throw new RuntimeExceptionWrapper(e);
        }
    }

    /**
     * Returns whether environment is configured for locking.
     */
    final boolean isLockingMode() {

        return lockingMode;
    }

    /**
     * Returns whether this is a transactional environment.
     */
    final boolean isTxnMode() {

        return txnMode;
    }

    /**
     * Returns whether this is a Concurrent Data Store environment.
     */
    final boolean isCdbMode() {

        return cdbMode;
    }

    /**
     * Return the LockMode.RMW or null, depending on whether locking is
     * enabled.  LockMode.RMW will cause an error if passed when locking
     * is not enabled.  Locking is enabled if locking or transactions were
     * specified for this environment.
     */
    final LockMode getWriteLockMode() {

        return writeLockMode;
    }

    /**
     * Returns the underlying Berkeley DB environment.
     */
    public final Environment getEnvironment() {

        return envRef.get();
    }

    /**
     * Returns the transaction associated with the current thread for this
     * environment, or null if no transaction is active.
     */
    public final Transaction getTransaction() {

        Trans trans = (Trans) localTrans.get();
        return (trans != null) ? trans.txn : null;
    }

    /**
     * Returns whether auto-commit may be performed by the collections API.
     * True is returned if no collections API transaction is currently active,
     * and no XA transaction is currently active.
     */
    boolean isAutoCommitAllowed()
	throws DatabaseException {

        return getTransaction() == null &&
               DbCompat.getThreadTransaction(getEnvironment()) == null;
    }

    /**
     * Begins a new transaction for this environment and associates it with
     * the current thread.  If a transaction is already active for this
     * environment and thread, a nested transaction will be created.
     *
     * @param config the transaction configuration used for calling
     * {@link Environment#beginTransaction}, or null to use the default
     * configuration.
     *
     * @return the new transaction.
     *
     * @throws DatabaseException if the transaction cannot be started, in which
     * case any existing transaction is not affected.
     *
     * @throws IllegalStateException if a transaction is already active and
     * nested transactions are not supported by the environment.
     */
    public final Transaction beginTransaction(TransactionConfig config)
        throws DatabaseException {

        Environment env = getEnvironment();
        Trans trans = (Trans) localTrans.get();
        if (trans != null) {
            if (trans.txn != null) {
                if (!DbCompat.NESTED_TRANSACTIONS) {
                    throw new IllegalStateException(
                            "Nested transactions are not supported");
                }
                Transaction parentTxn = trans.txn;
                trans = new Trans(trans, config);
                trans.txn = env.beginTransaction(parentTxn, config);
                localTrans.set(trans);
            } else {
                trans.txn = env.beginTransaction(null, config);
                trans.config = config;
            }
        } else {
            trans = new Trans(null, config);
            trans.txn = env.beginTransaction(null, config);
            localTrans.set(trans);
        }
        return trans.txn;
    }

    /**
     * Commits the transaction that is active for the current thread for this
     * environment and makes the parent transaction (if any) the current
     * transaction.
     *
     * @return the parent transaction or null if the committed transaction was
     * not nested.
     *
     * @throws DatabaseException if an error occurs committing the transaction.
     * The transaction will still be closed and the parent transaction will
     * become the current transaction.
     *
     * @throws IllegalStateException if no transaction is active for the
     * current thread for this environment.
     */
    public final Transaction commitTransaction()
        throws DatabaseException, IllegalStateException {

        Trans trans = (Trans) localTrans.get();
        if (trans != null && trans.txn != null) {
            Transaction parent = closeTxn(trans);
            trans.txn.commit();
            return parent;
        } else {
            throw new IllegalStateException("No transaction is active");
        }
    }

    /**
     * Aborts the transaction that is active for the current thread for this
     * environment and makes the parent transaction (if any) the current
     * transaction.
     *
     * @return the parent transaction or null if the aborted transaction was
     * not nested.
     *
     * @throws DatabaseException if an error occurs aborting the transaction.
     * The transaction will still be closed and the parent transaction will
     * become the current transaction.
     *
     * @throws IllegalStateException if no transaction is active for the
     * current thread for this environment.
     */
    public final Transaction abortTransaction()
        throws DatabaseException, IllegalStateException {

        Trans trans = (Trans) localTrans.get();
        if (trans != null && trans.txn != null) {
            Transaction parent = closeTxn(trans);
            trans.txn.abort();
            return parent;
        } else {
            throw new IllegalStateException("No transaction is active");
        }
    }

    /**
     * Returns whether the current transaction is a readUncommitted
     * transaction.
     */
    final boolean isReadUncommitted() {

        Trans trans = (Trans) localTrans.get();
        if (trans != null && trans.config != null) {
            return trans.config.getReadUncommitted();
        } else {
            return false;
        }
    }

    private Transaction closeTxn(Trans trans) {

        localTrans.set(trans.parent);
        return (trans.parent != null) ? trans.parent.txn : null;
    }

    private static class Trans {

        private Trans parent;
        private Transaction txn;
        private TransactionConfig config;

        private Trans(Trans parent, TransactionConfig config) {

            this.parent = parent;
            this.config = config;
        }
    }

    /**
     * Opens a cursor for a given database, dup'ing an existing CDB cursor if
     * one is open for the current thread.
     */
    Cursor openCursor(Database db,
                      CursorConfig cursorConfig,
                      boolean writeCursor,
                      Transaction txn)
        throws DatabaseException {

        if (cdbMode) {
            CdbCursors cdbCursors = null;
            WeakHashMap cdbCursorsMap = (WeakHashMap) localCdbCursors.get();
            if (cdbCursorsMap == null) {
                cdbCursorsMap = new WeakHashMap();
                localCdbCursors.set(cdbCursorsMap);
            } else {
                cdbCursors = (CdbCursors) cdbCursorsMap.get(db);
            }
            if (cdbCursors == null) {
                cdbCursors = new CdbCursors();
                cdbCursorsMap.put(db, cdbCursors);
            }

            /*
             * In CDB mode the cursorConfig specified by the user is ignored
             * and only the writeCursor parameter is honored.  This is the only
             * meaningful cursor attribute for CDB, and here we count on
             * writeCursor flag being set correctly by the caller.
             */
            List cursors;
            CursorConfig cdbConfig;
            if (writeCursor) {
                if (cdbCursors.readCursors.size() > 0) {

                    /*
                     * Although CDB allows opening a write cursor when a read
                     * cursor is open, a self-deadlock will occur if a write is
                     * attempted for a record that is read-locked; we should
                     * avoid self-deadlocks at all costs
                     */
                    throw new IllegalStateException(
                      "cannot open CDB write cursor when read cursor is open");
                }
                cursors = cdbCursors.writeCursors;
                cdbConfig = new CursorConfig();
                DbCompat.setWriteCursor(cdbConfig, true);
            } else {
                cursors = cdbCursors.readCursors;
                cdbConfig = null;
            }
            Cursor cursor;
            if (cursors.size() > 0) {
                Cursor other = ((Cursor) cursors.get(0));
                cursor = other.dup(false);
            } else {
                cursor = db.openCursor(null, cdbConfig);
            }
            cursors.add(cursor);
            return cursor;
        } else {
            return db.openCursor(txn, cursorConfig);
        }
    }

    /**
     * Duplicates a cursor for a given database.
     *
     * @param writeCursor true to open a write cursor in a CDB environment, and
     * ignored for other environments.
     *
     * @param samePosition is passed through to Cursor.dup().
     *
     * @return the open cursor.
     *
     * @throws DatabaseException if a database problem occurs.
     */
    Cursor dupCursor(Cursor cursor, boolean writeCursor, boolean samePosition)
        throws DatabaseException {

        if (cdbMode) {
            WeakHashMap cdbCursorsMap = (WeakHashMap) localCdbCursors.get();
            if (cdbCursorsMap != null) {
                Database db = cursor.getDatabase();
                CdbCursors cdbCursors = (CdbCursors) cdbCursorsMap.get(db);
                if (cdbCursors != null) {
                    List cursors = writeCursor ? cdbCursors.writeCursors
                                               : cdbCursors.readCursors;
                    if (cursors.contains(cursor)) {
                        Cursor newCursor = cursor.dup(samePosition);
                        cursors.add(newCursor);
                        return newCursor;
                    }
                }
            }
            throw new IllegalStateException("cursor to dup not tracked");
        } else {
            return cursor.dup(samePosition);
        }
    }

    /**
     * Closes a cursor.
     *
     * @param cursor the cursor to close.
     *
     * @throws DatabaseException if a database problem occurs.
     */
    void closeCursor(Cursor cursor)
        throws DatabaseException {

        if (cursor == null) {
            return;
        }
        if (cdbMode) {
            WeakHashMap cdbCursorsMap = (WeakHashMap) localCdbCursors.get();
            if (cdbCursorsMap != null) {
                Database db = cursor.getDatabase();
                CdbCursors cdbCursors = (CdbCursors) cdbCursorsMap.get(db);
                if (cdbCursors != null) {
                    if (cdbCursors.readCursors.remove(cursor) ||
                        cdbCursors.writeCursors.remove(cursor)) {
                        cursor.close();
                        return;
                    }
                }
            }
            throw new IllegalStateException(
              "closing CDB cursor that was not known to be open");
        } else {
            cursor.close();
        }
    }

    /**
     * Returns true if a CDB cursor is open and therefore a Database write
     * operation should not be attempted since a self-deadlock may result.
     */
    boolean isCDBCursorOpen(Database db) {
        if (cdbMode) {
            WeakHashMap cdbCursorsMap = (WeakHashMap) localCdbCursors.get();
            if (cdbCursorsMap != null) {
                CdbCursors cdbCursors = (CdbCursors) cdbCursorsMap.get(db);

                if (cdbCursors != null &&
                    (cdbCursors.readCursors.size() > 0 ||
                     cdbCursors.writeCursors.size() > 0)) {
                    return true;
                }
            }
        }
        return false;
    }

    static final class CdbCursors {

        List writeCursors = new ArrayList();
        List readCursors = new ArrayList();
    }
}
