/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.Db;
import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbSequence;
import com.sleepycat.db.internal.DbTxn;

/**
Specify the attributes of a sequence.
*/
public class SequenceConfig implements Cloneable {
    /*
     * For internal use, final to allow null as a valid value for
     * the config parameter.
     */
    /**
    Default configuration used if null is passed to methods that create a
    cursor.
    */
    public static final SequenceConfig DEFAULT = new SequenceConfig();

    /* package */
    static SequenceConfig checkNull(SequenceConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    /* Parameters */
    private int cacheSize = 0;
    private long rangeMin = Long.MIN_VALUE;
    private long rangeMax = Long.MAX_VALUE;
    private long initialValue = 0L;

    /* Flags */
    private boolean allowCreate = false;
    private boolean decrement = false;
    private boolean exclusiveCreate = false;
    private boolean autoCommitNoSync = false;
    private boolean wrap = false;

    /**
    An instance created using the default constructor is initialized with
    the system's default settings.
    */
    public SequenceConfig() {
    }

    /**
Configure the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method to
    create the sequence if it does not already exist.
<p>
This method may be called at any time during the life of the application.
<p>
@param allowCreate
If true,
configure the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method to
    create the sequence if it does not already exist.
    */
    public void setAllowCreate(final boolean allowCreate) {
        this.allowCreate = allowCreate;
    }

    /**
Return true if the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method is configured
    to create the sequence if it does not already exist.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method is configured
    to create the sequence if it does not already exist.
    */
    public boolean getAllowCreate() {
        return allowCreate;
    }

    /**
Set the
Configure the number of elements cached by a sequence handle.
<p>
This method may be called at any time during the life of the application.
<p>
@param cacheSize
The Configure the number of elements cached by a sequence handle.
    */
    public void setCacheSize(final int cacheSize) {
        this.cacheSize = cacheSize;
    }

    /**
Return the number of elements cached by a sequence handle..
<p>
This method may be called at any time during the life of the application.
<p>
@return
The number of elements cached by a sequence handle..
    */
    public int getCacheSize() {
        return cacheSize;
    }

    /**
Specify that the sequence should be decremented.
<p>
This method may be called at any time during the life of the application.
<p>
@param decrement
If true,
specify that the sequence should be decremented.
    */
    public void setDecrement(boolean decrement) {
        this.decrement = decrement;
    }

    /**
Return true if the sequence is configured to decrement.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the sequence is configured to decrement.
    */
    public boolean getDecrement() {
         return decrement;
    }

    /**
Configure the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method to
    fail if the database already exists.
<p>
This method may be called at any time during the life of the application.
<p>
@param exclusiveCreate
If true,
configure the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method to
    fail if the database already exists.
    */
    public void setExclusiveCreate(final boolean exclusiveCreate) {
        this.exclusiveCreate = exclusiveCreate;
    }

    /**
Return true if the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method is configured to
    fail if the database already exists.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the {@link com.sleepycat.db.Database#openSequence Database.openSequence} method is configured to
    fail if the database already exists.
    */
    public boolean getExclusiveCreate() {
        return exclusiveCreate;
    }

    /**
Set the
Set the initial value for a sequence.
<p>This call is only
    effective when the sequence is being created.
<p>
This method may be called at any time during the life of the application.
<p>
@param initialValue
The Set the initial value for a sequence.
    */
    public void setInitialValue(long initialValue) {
        this.initialValue = initialValue;
    }

    /**
Return the initial value for a sequence..
<p>
This method may be called at any time during the life of the application.
<p>
@return
The initial value for a sequence..
    */
    public long getInitialValue() {
        return initialValue;
    }

    /**
Configure auto-commit operations on the sequence to not flush
    the transaction log.
<p>
This method may be called at any time during the life of the application.
<p>
@param autoCommitNoSync
If true,
configure auto-commit operations on the sequence to not flush
    the transaction log.
    */
    public void setAutoCommitNoSync(final boolean autoCommitNoSync) {
        this.autoCommitNoSync = autoCommitNoSync;
    }

    /**
Return true if the auto-commit operations on the sequence are configure to not
    flush the transaction log..
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the auto-commit operations on the sequence are configure to not
    flush the transaction log..
    */
    public boolean getAutoCommitNoSync() {
        return autoCommitNoSync;
    }

    /**
    Configure a sequence range.  This call is only effective when the
    sequence is being created.
    <p>
    @param min
    The minimum value for the sequence.
    @param max
    The maximum value for the sequence.
    */
    public void setRange(final long min, final long max) {
        this.rangeMin = min;
        this.rangeMax = max;
    }

    /**
Return the minimum value for the sequence.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The minimum value for the sequence.
    */
    public long getRangeMin() {
        return rangeMin;
    }

    /**
Return the maximum value for the sequence.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The maximum value for the sequence.
    */
    public long getRangeMax() {
        return rangeMax;
    }

    /**
Specify that the sequence should wrap around when it is
    incremented (decremented) past the specified maximum (minimum) value.
<p>
This method may be called at any time during the life of the application.
<p>
@param wrap
If true,
specify that the sequence should wrap around when it is
    incremented (decremented) past the specified maximum (minimum) value.
    */
    public void setWrap(final boolean wrap) {
        this.wrap = wrap;
    }

    /**
Return true if the sequence will wrap around when it is incremented
    (decremented) past the specified maximum (minimum) value.
<p>
This method may be called at any time during the life of the application.
<p>
@return
True if the sequence will wrap around when it is incremented
    (decremented) past the specified maximum (minimum) value.
    */
    public boolean getWrap() {
        return wrap;
    }

    /* package */
    DbSequence createSequence(final Db db)
        throws DatabaseException {

        int createFlags = 0;

        return new DbSequence(db, createFlags);
    }

    /* package */
    DbSequence openSequence(final Db db,
                    final DbTxn txn,
                    final DatabaseEntry key)
        throws DatabaseException {

        final DbSequence seq = createSequence(db);
        // The DB_THREAD flag is inherited from the database
        boolean threaded = ((db.get_open_flags() & DbConstants.DB_THREAD) != 0);

        int openFlags = 0;
        openFlags |= allowCreate ? DbConstants.DB_CREATE : 0;
        openFlags |= exclusiveCreate ? DbConstants.DB_EXCL : 0;
        openFlags |= threaded ? DbConstants.DB_THREAD : 0;

        if (db.get_transactional() && txn == null)
            openFlags |= DbConstants.DB_AUTO_COMMIT;

        configureSequence(seq, DEFAULT);
        boolean succeeded = false;
        try {
            seq.open(txn, key, openFlags);
            succeeded = true;
            return seq;
        } finally {
            if (!succeeded)
                try {
                    seq.close(0);
                } catch (Throwable t) {
                    // Ignore it -- an exception is already in flight.
                }
        }
    }

    /* package */
    void configureSequence(final DbSequence seq, final SequenceConfig oldConfig)
        throws DatabaseException {

        int seqFlags = 0;
        seqFlags |= decrement ? DbConstants.DB_SEQ_DEC : DbConstants.DB_SEQ_INC;
        seqFlags |= wrap ? DbConstants.DB_SEQ_WRAP : 0;

        if (seqFlags != 0)
            seq.set_flags(seqFlags);

        if (rangeMin != oldConfig.rangeMin || rangeMax != oldConfig.rangeMax)
            seq.set_range(rangeMin, rangeMax);

        if (initialValue != oldConfig.initialValue)
            seq.initial_value(initialValue);

        if (cacheSize != oldConfig.cacheSize)
            seq.set_cachesize(cacheSize);
    }

    /* package */
    SequenceConfig(final DbSequence seq)
        throws DatabaseException {

        // XXX: can't get open flags
        final int openFlags = 0;
        allowCreate = (openFlags & DbConstants.DB_CREATE) != 0;
        exclusiveCreate = (openFlags & DbConstants.DB_EXCL) != 0;

        final int seqFlags = seq.get_flags();
        decrement = (seqFlags & DbConstants.DB_SEQ_DEC) != 0;
        wrap = (seqFlags & DbConstants.DB_SEQ_WRAP) != 0;

        // XXX: can't get initial value
        final long initialValue = 0;

        cacheSize = seq.get_cachesize();

        rangeMin = seq.get_range_min();
        rangeMax = seq.get_range_max();
    }
}
