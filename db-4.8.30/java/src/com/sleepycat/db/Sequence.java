/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbSequence;

/**
A Sequence handle is used to manipulate a sequence record in a database.
Sequence handles are opened using the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method.
*/
public class Sequence {
    private DbSequence seq;
    private int autoCommitFlag;

    /* package */
    Sequence(final DbSequence seq, SequenceConfig config)
        throws DatabaseException {

        this.seq = seq;
        seq.wrapper = this;
        if (seq.get_db().get_transactional())
                this.autoCommitFlag = DbConstants.DB_AUTO_COMMIT |
                    (SequenceConfig.checkNull(config).getAutoCommitNoSync() ?
                    DbConstants.DB_TXN_NOSYNC : 0);
    }

    /**
    Close a sequence.  Any unused cached values are lost.
    <p>
    The sequence handle may not be used again after this method has been
    called, regardless of the method's success or failure.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void close()
        throws DatabaseException {

        seq.close(0);
    }

    /**
    Return the next available element in the sequence and changes the sequence
    value by <code>delta</code>.  The value of <code>delta</code> must be
    greater than zero.  If there are enough cached values in the sequence
    handle then they will be returned.  Otherwise the next value will be
    fetched from the database and incremented (decremented) by enough to cover
    the <code>delta</code> and the next batch of cached values.
    <p>
    The <code>txn</code> handle must be null if the sequence handle was opened
    with a non-zero cache size.
    <p>
    For maximum concurrency, a non-zero cache size should be specified prior to
    opening the sequence handle, the <code>txn</code> handle should be
    <code>null</code>, and {@link com.sleepycat.db.SequenceConfig#setAutoCommitNoSync SequenceConfig.setAutoCommitNoSync} should
    be called to disable log flushes.
    <p>
    @param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
    <p>
    @param delta
    the amount by which to increment or decrement the sequence
    <p>
    @return
    the next available element in the sequence
    */
    public long get(Transaction txn, int delta)
        throws DatabaseException {

        return seq.get((txn == null) ? null : txn.txn, delta,
            (txn == null) ? autoCommitFlag : 0);
    }

    /**
    Return the Database handle associated with this sequence.
    <p>
    @return
    The Database handle associated with this sequence.
    */
    public Database getDatabase()
        throws DatabaseException {

        return seq.get_db().wrapper;
    }

    /**
    Return the DatabaseEntry used to open this sequence.
    <p>
    @return
    The DatabaseEntry used to open this sequence.
    */
    public DatabaseEntry getKey()
        throws DatabaseException {

        DatabaseEntry key = new DatabaseEntry();
        seq.get_key(key);
        return key;
    }

    /**
    Return statistical information about the sequence.
    <p>
    In the presence of multiple threads or processes accessing an active
    sequence, the information returned by this method may be out-of-date.
    <p>
    The getStats method cannot be transaction-protected. For this reason, it
    should be called in a thread of control that has no open cursors or active
    transactions.
    <p>
    @param config
    The statistics returned; if null, default statistics are returned.
    <p>
    @return
    Sequence statistics.
    */
    public SequenceStats getStats(StatsConfig config)
        throws DatabaseException {

        return seq.stat(config.getFlags());
    }
}
