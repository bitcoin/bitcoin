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
Statistics about mutexes in a Berkeley DB database environment, returned
by {@link Environment#getMutexStats}
**/
public class MutexStats {
    // no public constructor
    /* package */ MutexStats() {}

    private int st_mutex_align;
    /** TODO */
    public int getMutexAlign() {
        return st_mutex_align;
    }

    private int st_mutex_tas_spins;
    /** TODO */
    public int getMutexTasSpins() {
        return st_mutex_tas_spins;
    }

    private int st_mutex_cnt;
    /** TODO */
    public int getMutexCount() {
        return st_mutex_cnt;
    }

    private int st_mutex_free;
    /** TODO */
    public int getMutexFree() {
        return st_mutex_free;
    }

    private int st_mutex_inuse;
    /** TODO */
    public int getMutexInuse() {
        return st_mutex_inuse;
    }

    private int st_mutex_inuse_max;
    /** TODO */
    public int getMutexInuseMax() {
        return st_mutex_inuse_max;
    }

    private long st_region_wait;
    /** TODO */
    public long getRegionWait() {
        return st_region_wait;
    }

    private long st_region_nowait;
    /** TODO */
    public long getRegionNowait() {
        return st_region_nowait;
    }

    private int st_regsize;
    /** TODO */
    public int getRegSize() {
        return st_regsize;
    }

    /**
    For convenience, the MutexStats class has a toString method that lists
    all the data fields.
    */
    public String toString() {
        return "MutexStats:"
            + "\n  st_mutex_align=" + st_mutex_align
            + "\n  st_mutex_tas_spins=" + st_mutex_tas_spins
            + "\n  st_mutex_cnt=" + st_mutex_cnt
            + "\n  st_mutex_free=" + st_mutex_free
            + "\n  st_mutex_inuse=" + st_mutex_inuse
            + "\n  st_mutex_inuse_max=" + st_mutex_inuse_max
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_regsize=" + st_regsize
            ;
    }
}
