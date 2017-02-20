/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbTxn;

/**
The Transaction object is the handle for a transaction.  Methods off the
transaction handle are used to configure, abort and commit the
transaction.  Transaction handles are provided to other Berkeley DB
methods in order to transactionally protect those operations.
<p>
Transaction handles are not free-threaded; transactions handles may
be used by multiple threads, but only serially, that is, the application
must serialize access to the handle.  Once the
{@link com.sleepycat.db.Transaction#abort Transaction.abort}, {@link com.sleepycat.db.Transaction#commit Transaction.commit} or
{@link com.sleepycat.db.Transaction#discard Transaction.discard}
methods are called, the handle may
not be accessed again, regardless of the success or failure of the method.
In addition, parent transactions may not issue any Berkeley DB operations
while they have active child transactions (child transactions that have
not yet been committed or aborted) except for {@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction}, {@link com.sleepycat.db.Transaction#abort Transaction.abort} and {@link com.sleepycat.db.Transaction#commit Transaction.commit}.
<p>
To obtain a transaction with default attributes:
<blockquote><pre>
    Transaction txn = myEnvironment.beginTransaction(null, null);
</pre></blockquote>
To customize the attributes of a transaction:
<blockquote><pre>
    TransactionConfig config = new TransactionConfig();
    config.setDirtyRead(true);
    Transaction txn = myEnvironment.beginTransaction(null, config);
</pre></blockquote>
*/
public class Transaction {
    /*package */ final DbTxn txn;

    Transaction(final DbTxn txn) {
        this.txn = txn;
    }

    /**
    Cause an abnormal termination of the transaction.
    <p>
    The log is played backward, and any necessary undo operations are done.
    Before Transaction.abort returns, any locks held by the transaction will
    have been released.
    <p>
    In the case of nested transactions, aborting a parent transaction
    causes all children (unresolved or not) of the parent transaction
    to be aborted.
    <p>
    All cursors opened within the transaction must be closed before the
    transaction is aborted.
    <p>
    After Transaction.abort has been called, regardless of its return, the
    {@link com.sleepycat.db.Transaction Transaction} handle may not be accessed again.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void abort()
        throws DatabaseException {

        txn.abort();
    }

    /**
    End the transaction.  If the environment is configured for synchronous
commit, the transaction will be committed synchronously to stable
storage before the call returns.  This means the transaction will exhibit
all of the ACID (atomicity, consistency, isolation, and durability)
properties.
<p>
If the environment is not configured for synchronous commit, the commit
will not necessarily have been committed to stable storage before the
call returns.  This means the transaction will exhibit the ACI (atomicity,
consistency, and isolation) properties, but not D (durability); that is,
database integrity will be maintained, but it is possible this transaction
may be undone during recovery.
<p>
In the case of nested transactions, if the transaction is a parent
transaction, committing the parent transaction causes all unresolved
children of the parent to be committed.  In the case of nested
transactions, if the transaction is a child transaction, its locks are
not released, but are acquired by its parent.  Although the commit of the
child transaction will succeed, the actual resolution of the child
transaction is postponed until the parent transaction is committed or
aborted; that is, if its parent transaction commits, it will be
committed; and if its parent transaction aborts, it will be aborted.
<p>
All cursors opened within the transaction must be closed before the
transaction is committed.
<p>
After this method returns the {@link com.sleepycat.db.Transaction Transaction} handle may not be
accessed again, regardless of the method's success or failure. If the
method encounters an error, the transaction and all child transactions
of the transaction will have been aborted when the call returns.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void commit()
        throws DatabaseException {

        txn.commit(0);
    }

    /**
    End the transaction, committing synchronously.  This means the
transaction will exhibit all of the ACID (atomicity, consistency,
isolation, and durability) properties.
<p>
This behavior is the default for database environments unless otherwise
configured using the {@link com.sleepycat.db.EnvironmentConfig#setTxnNoSync EnvironmentConfig.setTxnNoSync} method.  This
behavior may also be set for a single transaction using the
{@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction} method.  Any value specified to
this method overrides both of those settings.
<p>
In the case of nested transactions, if the transaction is a parent
transaction, committing the parent transaction causes all unresolved
children of the parent to be committed.  In the case of nested
transactions, if the transaction is a child transaction, its locks are
not released, but are acquired by its parent.  Although the commit of the
child transaction will succeed, the actual resolution of the child
transaction is postponed until the parent transaction is committed or
aborted; that is, if its parent transaction commits, it will be
committed; and if its parent transaction aborts, it will be aborted.
<p>
All cursors opened within the transaction must be closed before the
transaction is committed.
<p>
After this method returns the {@link com.sleepycat.db.Transaction Transaction} handle may not be
accessed again, regardless of the method's success or failure. If the
method encounters an error, the transaction and all child transactions
of the transaction will have been aborted when the call returns.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void commitSync()
        throws DatabaseException {

        txn.commit(DbConstants.DB_TXN_SYNC);
    }

    /**
    End the transaction, not committing synchronously.
This means the
transaction will exhibit the ACI (atomicity, consistency, and isolation)
properties, but not D (durability); that is, database integrity will be
maintained, but it is possible this transaction may be undone during
recovery.
<p>
This behavior may be set for a database environment using the
{@link com.sleepycat.db.EnvironmentConfig#setTxnNoSync EnvironmentConfig.setTxnNoSync} method or for a single transaction
using the {@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction} method.  Any value
specified to this method overrides both of those settings.
<p>
In the case of nested transactions, if the transaction is a parent
transaction, committing the parent transaction causes all unresolved
children of the parent to be committed.  In the case of nested
transactions, if the transaction is a child transaction, its locks are
not released, but are acquired by its parent.  Although the commit of the
child transaction will succeed, the actual resolution of the child
transaction is postponed until the parent transaction is committed or
aborted; that is, if its parent transaction commits, it will be
committed; and if its parent transaction aborts, it will be aborted.
<p>
All cursors opened within the transaction must be closed before the
transaction is committed.
<p>
After this method returns the {@link com.sleepycat.db.Transaction Transaction} handle may not be
accessed again, regardless of the method's success or failure. If the
method encounters an error, the transaction and all child transactions
of the transaction will have been aborted when the call returns.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void commitNoSync()
        throws DatabaseException {

        txn.commit(DbConstants.DB_TXN_NOSYNC);
    }

    /**
    End the transaction, writing but not flushing the log.
This means the
transaction will exhibit the ACI (atomicity, consistency, and isolation)
properties, but not D (durability); that is, database integrity will be
maintained, but it is possible this transaction may be undone during
recovery in the event that the operating system crashes. This option
provides more durability than an asynchronous commit and has less
performance cost than a synchronous commit.
<p>
This behavior may be set for a database environment using the
{@link com.sleepycat.db.EnvironmentConfig#setTxnWriteNoSync EnvironmentConfig.setTxnWriteNoSync} method or for a single
transaction using the {@link com.sleepycat.db.Environment#beginTransaction Environment.beginTransaction} method.
Any value specified to this method overrides both of those settings.
<p>
In the case of nested transactions, if the transaction is a parent
transaction, committing the parent transaction causes all unresolved
children of the parent to be committed.  In the case of nested
transactions, if the transaction is a child transaction, its locks are
not released, but are acquired by its parent.  Although the commit of the
child transaction will succeed, the actual resolution of the child
transaction is postponed until the parent transaction is committed or
aborted; that is, if its parent transaction commits, it will be
committed; and if its parent transaction aborts, it will be aborted.
<p>
All cursors opened within the transaction must be closed before the
transaction is committed.
<p>
After this method returns the {@link com.sleepycat.db.Transaction Transaction} handle may not be
accessed again, regardless of the method's success or failure. If the
method encounters an error, the transaction and all child transactions
of the transaction will have been aborted when the call returns.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void commitWriteNoSync()
        throws DatabaseException {

        txn.commit(DbConstants.DB_TXN_WRITE_NOSYNC);
    }

    /**
    Free up all the per-process resources associated with the specified
    {@link com.sleepycat.db.Transaction Transaction} handle, neither committing nor aborting the
    transaction.  This call may be used only after calls to
    {@link com.sleepycat.db.Environment#recover Environment.recover} when there are multiple global
    transaction managers recovering transactions in a single database
    environment.  Any transactions returned by {@link com.sleepycat.db.Environment#recover Environment.recover} that are not handled by the current global transaction
    manager should be discarded using this method.
    <p>
    The {@link com.sleepycat.db.Transaction Transaction} handle may not be accessed again after this
    method has been called, regardless of the method's success or failure.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void discard()
        throws DatabaseException {

        txn.discard(0);
    }

    /**
    Return the transaction's unique ID.
        <p>
    Locking calls made on behalf of this transaction should use the
    value returned from this method as the locker parameter to the
    {@link com.sleepycat.db.Environment#getLock Environment.getLock} or {@link com.sleepycat.db.Environment#lockVector Environment.lockVector}
    calls.
    <p>
    @return
    The transaction's unique ID.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public int getId()
        throws DatabaseException {

        return txn.id();
    }

    /**
    Get the user visible name for the transaction.
    <p>
    @return
    The user visible name for the transaction.
    <p>
    */
    public String getName()
        throws DatabaseException {

        return txn.get_name();
    }

    /**
    Initiate the beginning of a two-phase commit.
    <p>
    In a distributed transaction environment, Berkeley DB can be used
    as a local transaction manager.  In this case, the distributed
    transaction manager must send <em>prepare</em> messages to each
    local manager.  The local manager must then issue a
    {@link com.sleepycat.db.Transaction#prepare Transaction.prepare} call and await its successful return
    before responding to the distributed transaction manager.  Only
    after the distributed transaction manager receives successful
    responses from all of its <em>prepare</em> messages should it issue
    any <em>commit</em> messages.
    <p>
    In the case of nested transactions, preparing the parent causes all
    unresolved children of the parent transaction to be committed.
    Child transactions should never be explicitly prepared.  Their fate
    will be resolved along with their parent's during global recovery.
    <p>
    @param gid
    The global transaction ID by which this transaction will be known.
    This global transaction ID will be returned in calls to
    {@link com.sleepycat.db.Environment#recover Environment.recover} method, telling the application which
    global transactions must be resolved.  The gid parameter must be sized
    at least DB_XIDDATASIZE (currently 128) bytes; only the first
    DB_XIDDATASIZE bytes are used.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void prepare(final byte[] gid)
        throws DatabaseException {

        txn.prepare(gid);
    }

    /**
    Set the user visible name for the transaction.
    <p>
    @param name
    The user visible name for the transaction.
    <p>
    */
    public void setName(final String name)
        throws DatabaseException {

        txn.set_name(name);
    }

    /**
    Configure the timeout value for the transaction lifetime.
    <p>
    If the transaction runs longer than this time, the transaction may
    may throw {@link com.sleepycat.db.DatabaseException DatabaseException}.
    <p>
    Timeouts are checked whenever a thread of control blocks on a lock
    or when deadlock detection is performed.  For this reason, the
    accuracy of the timeout depends on how often deadlock detection is
    performed.
    <p>
    @param timeOut
    The timeout value for the transaction lifetime, in microseconds.  As
    the value is an unsigned 32-bit number of microseconds, the maximum
    timeout is roughly 71 minutes.  A value of 0 disables timeouts for
    the transaction.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void setTxnTimeout(final long timeOut)
        throws DatabaseException {

        txn.set_timeout(timeOut, DbConstants.DB_SET_TXN_TIMEOUT);
    }

    /**
    Configure the lock request timeout value for the transaction.
    <p>
    If a lock request cannot be granted in this time, the transaction
    may throw {@link com.sleepycat.db.DatabaseException DatabaseException}.
    <p>
    Timeouts are checked whenever a thread of control blocks on a lock
    or when deadlock detection is performed.  For this reason, the
    accuracy of the timeout depends on how often deadlock detection is
    performed.
    <p>
    @param timeOut
    The lock request timeout value for the transaction, in microseconds.
    As the value is an unsigned 32-bit number of microseconds, the maximum
    timeout is roughly 71 minutes.  A value of 0 disables timeouts for the
    transaction.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void setLockTimeout(final long timeOut)
        throws DatabaseException {

        txn.set_timeout(timeOut, DbConstants.DB_SET_LOCK_TIMEOUT);
    }
}
