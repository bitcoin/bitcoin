/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbLock;

/**
The LockRequest object is used to encapsulate a single lock request.
*/
public class LockRequest {
    private DbLock lock;
    private LockRequestMode mode;
    private int modeFlag;
    private DatabaseEntry obj;
    private int op;
    private int timeout;

    /**
    Construct a LockRequest with the specified operation and mode for the
    specified object.
    <p>
    @param mode
    The permissions mode for the object.
    <p>
    @param obj
    The object being locked.
    <p>
    @param op
    The operation being performed.
    */
    public LockRequest(final LockOperation op,
                       final LockRequestMode mode,
                       final DatabaseEntry obj) {

        this(op, mode, obj, null, 0);
    }

    /**
    Construct a LockRequest with the specified operation, mode and lock,
    for the specified object.
    <p>
    @param mode
    The permissions mode for the object.
    <p>
    @param obj
    The object being locked.
    <p>
    @param op
    The operation being performed.
    <p>
    @param lock
    The lock type for the object.
    */
    public LockRequest(final LockOperation op,
                       final LockRequestMode mode,
                       final DatabaseEntry obj,
                       final Lock lock) {

        this(op, mode, obj, lock, 0);
    }

    /**
    Construct a LockRequest with the specified operation, mode, lock and
    timeout for the specified object.
    <p>
    @param lock
    The lock type for the object.
    <p>
    @param mode
    The permissions mode for the object.
    <p>
    @param obj
    The object being locked.
    <p>
    @param op
    The operation being performed.
    <p>
    @param timeout
    The timeout value for the lock.
    */
    public LockRequest(final LockOperation op,
                       final LockRequestMode mode,
                       final DatabaseEntry obj,
                       final Lock lock,
                       final int timeout) {

        this.setOp(op);
        this.setMode(mode);
        this.setObj(obj);
        this.setLock(lock);
        this.setTimeout(timeout);
    }

    /**
    Set the lock reference.
    <p>
    @param lock
    The lock reference.
    */
    public void setLock(final Lock lock) {
        this.lock = (lock == null) ? null : lock.unwrap();
    }

    /**
    Set the lock mode.
    <p>
    @param mode
    the lock mode.
    */
    public void setMode(final LockRequestMode mode) {
        this.mode = mode;
        this.modeFlag = mode.getFlag();
    }

    /**
    Set the lock object.
    <p>
    @param obj
    The lock object.
    */
    public void setObj(final DatabaseEntry obj) {
        this.obj = obj;
    }

    /**
    Set the operation.
    <p>
    @param op
    The operation.
    */
    public void setOp(final LockOperation op) {
        this.op = op.getFlag();
    }

    /**
    Set the lock timeout value.
    <p>
    @param timeout
    The lock timeout value.
    */
    public void setTimeout(final int timeout) {
        this.timeout = timeout;
    }

    /**
    Return the lock reference.
    */
    public Lock getLock() {
        if (lock != null && lock.wrapper != null)
            return lock.wrapper;
        else
            return Lock.wrap(lock);
    }

    /**
    Return the lock mode.
    */
    public LockRequestMode getMode() {
        return mode;
    }

    /**
    Return the lock object.
    */
    public DatabaseEntry getObj() {
        return obj;
    }

    /**
    Return the lock operation.
    */
    public LockOperation getOp() {
        return LockOperation.fromFlag(op);
    }

    /**
    Return the lock timeout value.
    */
    public int getTimeout() {
        return timeout;
    }
}
