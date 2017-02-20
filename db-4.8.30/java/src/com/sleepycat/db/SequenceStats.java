/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

/**
A SequenceStats object is used to return sequence statistics.
*/
public class SequenceStats {
    // no public constructor
    /* package */ SequenceStats() {}

    private long st_wait;
    /** TODO */
    public long getWait() {
        return st_wait;
    }

    private long st_nowait;
    /** TODO */
    public long getNowait() {
        return st_nowait;
    }

    private long st_current;
    /** TODO */
    public long getCurrent() {
        return st_current;
    }

    private long st_value;
    /** TODO */
    public long getValue() {
        return st_value;
    }

    private long st_last_value;
    /** TODO */
    public long getLastValue() {
        return st_last_value;
    }

    private long st_min;
    /** TODO */
    public long getMin() {
        return st_min;
    }

    private long st_max;
    /** TODO */
    public long getMax() {
        return st_max;
    }

    private int st_cache_size;
    /** TODO */
    public int getCacheSize() {
        return st_cache_size;
    }

    private int st_flags;
    /** TODO */
    public int getFlags() {
        return st_flags;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "SequenceStats:"
            + "\n  st_wait=" + st_wait
            + "\n  st_nowait=" + st_nowait
            + "\n  st_current=" + st_current
            + "\n  st_value=" + st_value
            + "\n  st_last_value=" + st_last_value
            + "\n  st_min=" + st_min
            + "\n  st_max=" + st_max
            + "\n  st_cache_size=" + st_cache_size
            + "\n  st_flags=" + st_flags
            ;
    }
}
