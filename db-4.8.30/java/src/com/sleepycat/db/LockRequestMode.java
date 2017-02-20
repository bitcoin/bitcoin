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
When using the default lock conflict matrix, the LockRequestMode class
defines the set of possible lock modes.
*/
public final class LockRequestMode {
    /**
    Read (shared).
    */
    public static final LockRequestMode READ =
        new LockRequestMode("READ", DbConstants.DB_LOCK_READ);
    /**
    Write (exclusive).
    */
    public static final LockRequestMode WRITE =
        new LockRequestMode("WRITE", DbConstants.DB_LOCK_WRITE);
    /**
    Intention to write (shared).
    */
    public static final LockRequestMode IWRITE =
        new LockRequestMode("IWRITE", DbConstants.DB_LOCK_IWRITE);
    /**
    Intention to read (shared).
    */
    public static final LockRequestMode IREAD =
        new LockRequestMode("IREAD", DbConstants.DB_LOCK_IREAD);
    /**
    Intention to read and write (shared).
    */
    public static final LockRequestMode IWR =
        new LockRequestMode("IWR", DbConstants.DB_LOCK_IWR);

    /* package */
    private final String operationName;
    private final int flag;

    /**
    Construct a custom lock request mode.
    <p>
    @param operationName
    Name used to display the mode
    <p>
    @param flag
    Flag value used as an index into the lock conflict matrix
    */
    public LockRequestMode(final String operationName, final int flag) {
        this.operationName = operationName;
        this.flag = flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "LockRequestMode." + operationName;
    }

    /* package */
    int getFlag() {
        return flag;
    }
}
