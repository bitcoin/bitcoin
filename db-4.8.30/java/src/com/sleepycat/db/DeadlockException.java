/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
DeadlockException is thrown to a thread of control when multiple threads
competing for a lock are
deadlocked, when a lock request has timed out
or when a lock request would need to block and the transaction has been
configured to not wait for locks.
*/
public class DeadlockException extends DatabaseException {
    /* package */ DeadlockException(final String s,
                                final int errno,
                                final DbEnv dbenv) {
        super(s, errno, dbenv);
    }
}
