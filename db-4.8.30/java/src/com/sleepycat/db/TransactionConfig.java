/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbTxn;

/**
Specifies the attributes of a database environment transaction.
*/
public class TransactionConfig implements Cloneable {
    /*
     * For internal use, to allow null as a valid value for
     * the config parameter.
     */
    /**
    Default configuration used if null is passed to methods that create a
    transaction.
    */
    public static final TransactionConfig DEFAULT = new TransactionConfig();

    /* package */
    static TransactionConfig checkNull(TransactionConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    private boolean readUncommitted = false;
    private boolean readCommitted = false;
    private boolean noSync = false;
    private boolean noWait = false;
    private boolean snapshot = false;
    private boolean sync = false;
    private boolean writeNoSync = false;
    private boolean wait = false;

    /**
    An instance created using the default constructor is initialized
    with the system's default settings.
    */
    public TransactionConfig() {
    }

    /**
        Configure the transaction for read committed isolation.
    <p>
    This ensures the stability of the current data item read by the
    cursor but permits data read by this transaction to be modified or
    deleted prior to the commit of the transaction.
    <p>
    @param readCommitted
    If true, configure the transaction for read committed isolation.
    */
    public void setReadCommitted(final boolean readCommitted) {
        this.readCommitted = readCommitted;
    }

    /**
        Return if the transaction is configured for read committed isolation.
    <p>
    @return
    If the transaction is configured for read committed isolation.
    */
    public boolean getReadCommitted() {
        return readCommitted;
    }

    /**
        Configure the transaction for read committed isolation.
    <p>
    This ensures the stability of the current data item read by the
    cursor but permits data read by this transaction to be modified or
    deleted prior to the commit of the transaction.
    <p>
    @param degree2
    If true, configure the transaction for read committed isolation.
        <p>
    @deprecated This has been replaced by {@link #setReadCommitted} to conform to ANSI
    database isolation terminology.
    */
    public void setDegree2(final boolean degree2) {
        setReadCommitted(degree2);
    }

    /**
        Return if the transaction is configured for read committed isolation.
    <p>
    @return
    If the transaction is configured for read committed isolation.
        <p>
    @deprecated This has been replaced by {@link #getReadCommitted} to conform to ANSI
    database isolation terminology.
    */
    public boolean getDegree2() {
        return getReadCommitted();
    }

    /**
        Configure read operations performed by the transaction to return modified
    but not yet committed data.
    <p>
    @param readUncommitted
    If true, configure read operations performed by the transaction to return
    modified but not yet committed data.
    */
    public void setReadUncommitted(final boolean readUncommitted) {
        this.readUncommitted = readUncommitted;
    }

    /**
        Return if read operations performed by the transaction are configured to
    return modified but not yet committed data.
    <p>
    @return
    If read operations performed by the transaction are configured to return
    modified but not yet committed data.
    */
    public boolean getReadUncommitted() {
        return readUncommitted;
    }

    /**
        Configure read operations performed by the transaction to return modified
    but not yet committed data.
    <p>
    @param dirtyRead
    If true, configure read operations performed by the transaction to return
    modified but not yet committed data.
        <p>
    @deprecated This has been replaced by {@link #setReadUncommitted} to conform to ANSI
    database isolation terminology.
    */
    public void setDirtyRead(final boolean dirtyRead) {
        setReadUncommitted(dirtyRead);
    }

    /**
        Return if read operations performed by the transaction are configured to
    return modified but not yet committed data.
    <p>
    @return
    If read operations performed by the transaction are configured to return
    modified but not yet committed data.
        <p>
    @deprecated This has been replaced by {@link #getReadUncommitted} to conform to ANSI
    database isolation terminology.
    */
    public boolean getDirtyRead() {
        return getReadUncommitted();
    }

    /**
    Configure the transaction to not write or synchronously flush the log
    it when commits.
    <p>
    This behavior may be set for a database environment using the
    Environment.setMutableConfig method. Any value specified to this method
    overrides that setting.
    <p>
    The default is false for this class and the database environment.
    <p>
    @param noSync
    If true, transactions exhibit the ACI (atomicity, consistency, and
    isolation) properties, but not D (durability); that is, database
    integrity will be maintained, but if the application or system
    fails, it is possible some number of the most recently committed
    transactions may be undone during recovery. The number of
    transactions at risk is governed by how many log updates can fit
    into the log buffer, how often the operating system flushes dirty
    buffers to disk, and how often the log is checkpointed.
    */
    public void setNoSync(final boolean noSync) {
        this.noSync = noSync;
    }

    /**
    Return if the transaction is configured to not write or synchronously
    flush the log it when commits.
    <p>
    @return
    If the transaction is configured to not write or synchronously flush
    the log it when commits.
    */
    public boolean getNoSync() {
        return noSync;
    }

    /**
    Configure the transaction to not wait if a lock request cannot be
    immediately granted.
    <p>
    The default is false for this class and the database environment.
    <p>
    @param noWait
    If true, transactions will not wait if a lock request cannot be
    immediately granted, instead {@link com.sleepycat.db.DeadlockException DeadlockException} will be thrown.
    */
    public void setNoWait(final boolean noWait) {
        this.noWait = noWait;
    }

    /**
    Return if the transaction is configured to not wait if a lock
    request cannot be immediately granted.
    <p>
    @return
    If the transaction is configured to not wait if a lock request
    cannot be immediately granted.
    */
    public boolean getNoWait() {
        return noWait;
    }

    /**
    This transaction will execute with snapshot isolation.  For databases
    configured with {@link DatabaseConfig#setMultiversion}, data values
    will be read as they are when the transaction begins, without taking
    read locks.
    <p>
    Updates operations performed in the transaction will cause a
    {@link DeadlockException} to be thrown if data is modified
    between reading and writing it.
    */
    public void setSnapshot(final boolean snapshot) {
        this.snapshot = snapshot;
    }

    /**
Return true if the transaction is configured for Snapshot Isolation.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the transaction is configured for Snapshot Isolation.
    */
    public boolean getSnapshot() {
        return snapshot;
    }

    /**
    Configure the transaction to write and synchronously flush the log
    it when commits.
    <p>
    This behavior may be set for a database environment using the
    Environment.setMutableConfig method. Any value specified to this
    method overrides that setting.
    <p>
    The default is false for this class and true for the database
    environment.
    <p>
    If true is passed to both setSync and setNoSync, setSync will take
    precedence.
    <p>
    @param sync
    If true, transactions exhibit all the ACID (atomicity, consistency,
    isolation, and durability) properties.
    */
    public void setSync(final boolean sync) {
        this.sync = sync;
    }

    /**
    Return if the transaction is configured to write and synchronously
    flush the log it when commits.
    <p>
    @return
    If the transaction is configured to write and synchronously flush
    the log it when commits.
    */
    public boolean getSync() {
        return sync;
    }

    /**
    Configure the transaction to wait if a lock request cannot be
    immediately granted.
    <p>
    The default is true unless {@link EnvironmentConfig#setTxnNoWait} is called.
    <p>
    @param wait
    If true, transactions will wait if a lock request cannot be
    immediately granted, instead {@link com.sleepycat.db.DeadlockException DeadlockException} will be thrown.
    */
    public void setWait(final boolean wait) {
        this.wait = wait;
    }

    /**
    Return if the transaction is configured to wait if a lock
    request cannot be immediately granted.
    <p>
    @return
    If the transaction is configured to wait if a lock request
    cannot be immediately granted.
    */
    public boolean getWait() {
        return wait;
    }

    /**
    Configure the transaction to write but not synchronously flush the log
    it when commits.
    <p>
    This behavior may be set for a database environment using the
    Environment.setMutableConfig method. Any value specified to this method
    overrides that setting.
    <p>
    The default is false for this class and the database environment.
    <p>
    @param writeNoSync
    If true, transactions exhibit the ACI (atomicity, consistency, and
    isolation) properties, but not D (durability); that is, database
    integrity will be maintained, but if the operating system
    fails, it is possible some number of the most recently committed
    transactions may be undone during recovery. The number of
    transactions at risk is governed by how often the operating system
    flushes dirty buffers to disk, and how often the log is
    checkpointed.
    */
    public void setWriteNoSync(final boolean writeNoSync) {
        this.writeNoSync = writeNoSync;
    }

    /**
    Return if the transaction is configured to write but not synchronously
    flush the log it when commits.
    <p>
    @return
    If the transaction is configured to not write or synchronously flush
    the log it when commits.
    */
    public boolean getWriteNoSync() {
        return writeNoSync;
    }

    DbTxn beginTransaction(final DbEnv dbenv, final DbTxn parent)
        throws DatabaseException {

        int flags = 0;
        flags |= readCommitted ? DbConstants.DB_READ_COMMITTED : 0;
        flags |= readUncommitted ? DbConstants.DB_READ_UNCOMMITTED : 0;
        flags |= noSync ? DbConstants.DB_TXN_NOSYNC : 0;
        flags |= noWait ? DbConstants.DB_TXN_NOWAIT : 0;
        flags |= snapshot ? DbConstants.DB_TXN_SNAPSHOT : 0;
        flags |= sync ? DbConstants.DB_TXN_SYNC : 0;
        flags |= wait ? DbConstants.DB_TXN_WAIT : 0;
        flags |= writeNoSync ? DbConstants.DB_TXN_WRITE_NOSYNC : 0;

        return dbenv.txn_begin(parent, flags);
    }
}
