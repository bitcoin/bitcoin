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
Specifies the attributes of a statistics retrieval operation.
*/
public class StatsConfig {
    /*
     * For internal use, to allow null as a valid value for
     * the config parameter.
     */
    public static final StatsConfig DEFAULT = new StatsConfig();

    /* package */
    static StatsConfig checkNull(StatsConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    private boolean clear = false;
    private boolean fast = false;

    /**
    An instance created using the default constructor is initialized
    with the system's default settings.
    */
    public StatsConfig() {
    }

    /**
    Configure the statistics operation to reset statistics after they
    are returned. The default value is false.
    <p>
    @param clear
    If set to true, configure the statistics operation to reset
    statistics after they are returned.
    */
    public void setClear(boolean clear) {
        this.clear = clear;
    }

    /**
    Return if the statistics operation is configured to reset
    statistics after they are returned.
    <p>
    @return
    If the statistics operation is configured to reset statistics after
    they are returned.
    */
    public boolean getClear() {
        return clear;
    }

    /**
    Configure the statistics operation to return only the values which
    do not incur some performance penalty.
    <p>
    The default value is false.
    <p>
    For example, skip stats that require a traversal of the database or
    in-memory tree, or which lock down the lock table for a period of
    time.
        <p>
    Among other things, this flag makes it possible for applications to
    request key and record counts without incurring the performance
    penalty of traversing the entire database.  If the underlying
    database is of type Recno, or of type Btree and the database was
    configured to support retrieval by record number, the count of keys
    will be exact.  Otherwise, the count of keys will be the value saved
    the last time the database was traversed, or 0 if no count of keys
    has ever been made.  If the underlying database is of type Recno,
    the count of data items will be exact, otherwise, the count of data
    items will be the value saved the last time the database was
    traversed, or 0 if no count of data items has ever been done.
    <p>
    @param fast
    If set to true, configure the statistics operation to return only
    the values which do not incur some performance penalty.
    */
    public void setFast(boolean fast) {
        this.fast = fast;
    }

    /**
    Return if the statistics operation is configured to return only the
    values which do not require expensive actions.
    <p>
    @return
    If the statistics operation is configured to return only the values
    which do not require expensive actions.
    */
    public boolean getFast() {
        return fast;
    }

    int getFlags() {
        int flags = 0;
        if (fast)
                flags |= DbConstants.DB_FAST_STAT;
        if (clear)
                flags |= DbConstants.DB_STAT_CLEAR;
        return flags;
    }
}
