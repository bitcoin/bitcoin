/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbLock;

/**
A LockNotGrantedException is thrown when a lock requested using the
{@link com.sleepycat.db.Environment#getLock Environment.getLock} or {@link com.sleepycat.db.Environment#lockVector Environment.lockVector}
methods, where the noWait flag or lock timers were configured, could not
be granted before the wait-time expired.
<p>
Additionally, LockNotGrantedException is thrown when a Concurrent Data
Store database environment configured for lock timeouts was unable to
grant a lock in the allowed time.
<p>
Additionally, LockNotGrantedException is thrown when lock or transaction
timeouts have been configured and a database operation has timed out.
*/
public class LockNotGrantedException extends DeadlockException {
    private int index;
    private Lock lock;
    private int mode;
    private DatabaseEntry obj;
    private int op;

    /* package */ LockNotGrantedException(final String message,
                                      final int op,
                                      final int mode,
                                      final DatabaseEntry obj,
                                      final DbLock lock,
                                      final int index,
                                      final DbEnv dbenv) {
        super(message, DbConstants.DB_LOCK_NOTGRANTED, dbenv);
        this.op = op;
        this.mode = mode;
        this.obj = obj;
        this.lock = (lock == null) ? null : lock.wrapper;
        this.index = index;
    }

    /**
    Returns -1 when {@link com.sleepycat.db.Environment#getLock Environment.getLock} was called, and
    returns the index of the failed LockRequest when {@link com.sleepycat.db.Environment#lockVector Environment.lockVector} was called.
    */
    public int getIndex() {
        return index;
    }

    /**
    Returns null when {@link com.sleepycat.db.Environment#getLock Environment.getLock} was called, and
    returns the lock in the failed LockRequest when {@link com.sleepycat.db.Environment#lockVector Environment.lockVector} was called.
    */
    public Lock getLock() {
        return lock;
    }

    /**
    Returns the mode parameter when {@link com.sleepycat.db.Environment#getLock Environment.getLock} was
    called, and returns the mode for the failed LockRequest when
    {@link com.sleepycat.db.Environment#lockVector Environment.lockVector} was called.
    */
    public int getMode() {
        return mode;
    }

    /**
    Returns the object parameter when {@link com.sleepycat.db.Environment#getLock Environment.getLock} was
    called, and returns the object for the failed LockRequest when
    {@link com.sleepycat.db.Environment#lockVector Environment.lockVector} was called.
    */
    public DatabaseEntry getObj() {
        return obj;
    }

    /**
    Returns 0 when {@link com.sleepycat.db.Environment#getLock Environment.getLock} was called, and returns
    the op parameter for the failed LockRequest when {@link com.sleepycat.db.Environment#lockVector Environment.lockVector} was called.
    */
    public int getOp() {
        return op;
    }
}
