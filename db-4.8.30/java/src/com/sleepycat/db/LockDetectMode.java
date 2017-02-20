/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/** Deadlock detection modes. */
public final class LockDetectMode {
    /**
    Turn off deadlock detection.
    */
    public static final LockDetectMode NONE =
        new LockDetectMode("NONE", 0);

    /**
    Use whatever lock policy was specified when the database environment
    was created.  If no lock policy has yet been specified, set the lock
    policy to DB_LOCK_RANDOM.
    */
    public static final LockDetectMode DEFAULT =
        new LockDetectMode("DEFAULT", DbConstants.DB_LOCK_DEFAULT);

    /**
    Reject lock requests which have timed out.  No other deadlock detection
    is performed.
    */
    public static final LockDetectMode EXPIRE =
        new LockDetectMode("EXPIRE", DbConstants.DB_LOCK_EXPIRE);

    /**
    Reject the lock request for the locker ID with the most locks.
    */
    public static final LockDetectMode MAXLOCKS =
        new LockDetectMode("MAXLOCKS", DbConstants.DB_LOCK_MAXLOCKS);

    /**
    Reject the lock request for the locker ID with the most write locks.
    */
    public static final LockDetectMode MAXWRITE =
        new LockDetectMode("MAXWRITE", DbConstants.DB_LOCK_MAXWRITE);

    /**
    Reject the lock request for the locker ID with the fewest locks.
    */
    public static final LockDetectMode MINLOCKS =
        new LockDetectMode("MINLOCKS", DbConstants.DB_LOCK_MINLOCKS);

    /**
    Reject the lock request for the locker ID with the fewest write locks.
    */
    public static final LockDetectMode MINWRITE =
        new LockDetectMode("MINWRITE", DbConstants.DB_LOCK_MINWRITE);

    /**
    Reject the lock request for the locker ID with the oldest lock.
    */
    public static final LockDetectMode OLDEST =
        new LockDetectMode("OLDEST", DbConstants.DB_LOCK_OLDEST);

    /**
    Reject the lock request for a random locker ID.
    */
    public static final LockDetectMode RANDOM =
        new LockDetectMode("RANDOM", DbConstants.DB_LOCK_RANDOM);

    /**
    Reject the lock request for the locker ID with the youngest lock.
    */
    public static final LockDetectMode YOUNGEST =
        new LockDetectMode("YOUNGEST", DbConstants.DB_LOCK_YOUNGEST);

    /* package */
    static LockDetectMode fromFlag(int flag) {
        switch (flag) {
        case 0:
            return NONE;
        case DbConstants.DB_LOCK_DEFAULT:
            return DEFAULT;
        case DbConstants.DB_LOCK_EXPIRE:
            return EXPIRE;
        case DbConstants.DB_LOCK_MAXLOCKS:
            return MAXLOCKS;
        case DbConstants.DB_LOCK_MAXWRITE:
            return MAXWRITE;
        case DbConstants.DB_LOCK_MINLOCKS:
            return MINLOCKS;
        case DbConstants.DB_LOCK_MINWRITE:
            return MINWRITE;
        case DbConstants.DB_LOCK_OLDEST:
            return OLDEST;
        case DbConstants.DB_LOCK_RANDOM:
            return RANDOM;
        case DbConstants.DB_LOCK_YOUNGEST:
            return YOUNGEST;
        default:
            throw new IllegalArgumentException(
                "Unknown lock detect mode: " + flag);
        }
    }

    private String modeName;
    private int flag;

    private LockDetectMode(final String modeName, final int flag) {
        this.modeName = modeName;
        this.flag = flag;
    }

    /* package */
    int getFlag() {
        return flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "LockDetectMode." + modeName;
    }
}
