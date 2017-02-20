/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
Locking modes for database operations. Locking modes are required
parameters for operations that retrieve data or modify the database.
*/
public final class LockMode {
    private String lockModeName;
    private int flag;

    private LockMode(String lockModeName, int flag) {
        this.lockModeName = lockModeName;
        this.flag = flag;
    }

    /**
        Acquire read locks for read operations and write locks for write
    operations.
    */
    public static final LockMode DEFAULT =
        new LockMode("DEFAULT", 0);
    /**
        Read modified but not yet committed data.
    */
    public static final LockMode READ_UNCOMMITTED =
        new LockMode("READ_UNCOMMITTED", DbConstants.DB_READ_UNCOMMITTED);
    /**
        Read committed isolation provides for cursor stability but not repeatable
    reads.  Data items which have been previously read by this transaction may
    be deleted or modified by other transactions before the cursor is closed or
    the transaction completes.
    <p>
    Note that this LockMode may only be passed to {@link Database} get
    methods, not to {@link Cursor} methods.  To configure a cursor for
    Read committed isolation, use {@link CursorConfig#setReadCommitted}.
    */
    public static final LockMode READ_COMMITTED =
        new LockMode("READ_COMMITTED", DbConstants.DB_READ_COMMITTED);
    /**
        Acquire write locks instead of read locks when doing the retrieval.
    Setting this flag can eliminate deadlock during a read-modify-write
    cycle by acquiring the write lock during the read part of the cycle
    so that another thread of control acquiring a read lock for the same
    item, in its own read-modify-write cycle, will not result in deadlock.
    */
    public static final LockMode RMW =
        new LockMode("RMW", DbConstants.DB_RMW);

    /**
        Read modified but not yet committed data.
        <p>
    @deprecated This has been replaced by {@link #READ_UNCOMMITTED} to conform to ANSI
    database isolation terminology.
    */
    public static final LockMode DIRTY_READ = READ_UNCOMMITTED;
    /**
        Read committed isolation provides for cursor stability but not repeatable
    reads.  Data items which have been previously read by this transaction may
    be deleted or modified by other transactions before the cursor is closed or
    the transaction completes.
    <p>
    Note that this LockMode may only be passed to {@link Database} get
    methods, not to {@link Cursor} methods.  To configure a cursor for
    Read committed isolation, use {@link CursorConfig#setReadCommitted}.
        <p>
    @deprecated This has been replaced by {@link #READ_COMMITTED} to conform to ANSI
    database isolation terminology.
    */
    public static final LockMode DEGREE_2 = READ_COMMITTED;

    /**
    Return the data item irrespective of the state of master leases.  The item
    will be returned under all conditions: if master leases are not configured,
    if the request is made to a client, if the request is made to a master with
    a valid lease, or if the request is made to a master without a valid lease.
    */   
    public static final LockMode IGNORE_LEASES =
	new LockMode("IGNORE_LEASES", DbConstants.DB_IGNORE_LEASE);

    /** {@inheritDoc} */
    public String toString() {
        return "LockMode." + lockModeName;
    }

    /* package */
    static int getFlag(LockMode mode) {
        return ((mode == null) ? DEFAULT : mode).flag;
    }
}
