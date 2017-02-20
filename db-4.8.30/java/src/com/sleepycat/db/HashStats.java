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
The HashStats object is used to return Hash database statistics.
*/
public class HashStats extends DatabaseStats {
    // no public constructor
    /* package */ HashStats() {}

    private int hash_magic;
    /** TODO */
    public int getMagic() {
        return hash_magic;
    }

    private int hash_version;
    /** TODO */
    public int getVersion() {
        return hash_version;
    }

    private int hash_metaflags;
    /** TODO */
    public int getMetaFlags() {
        return hash_metaflags;
    }

    private int hash_nkeys;
    /** TODO */
    public int getNumKeys() {
        return hash_nkeys;
    }

    private int hash_ndata;
    /** TODO */
    public int getNumData() {
        return hash_ndata;
    }

    private int hash_pagecnt;
    /** TODO */
    public int getPageCount() {
        return hash_pagecnt;
    }

    private int hash_pagesize;
    /** TODO */
    public int getPageSize() {
        return hash_pagesize;
    }

    private int hash_ffactor;
    /** TODO */
    public int getFfactor() {
        return hash_ffactor;
    }

    private int hash_buckets;
    /** TODO */
    public int getBuckets() {
        return hash_buckets;
    }

    private int hash_free;
    /** TODO */
    public int getFree() {
        return hash_free;
    }

    private long hash_bfree;
    /** TODO */
    public long getBFree() {
        return hash_bfree;
    }

    private int hash_bigpages;
    /** TODO */
    public int getBigPages() {
        return hash_bigpages;
    }

    private long hash_big_bfree;
    /** TODO */
    public long getBigBFree() {
        return hash_big_bfree;
    }

    private int hash_overflows;
    /** TODO */
    public int getOverflows() {
        return hash_overflows;
    }

    private long hash_ovfl_free;
    /** TODO */
    public long getOvflFree() {
        return hash_ovfl_free;
    }

    private int hash_dup;
    /** TODO */
    public int getDup() {
        return hash_dup;
    }

    private long hash_dup_free;
    /** TODO */
    public long getDupFree() {
        return hash_dup_free;
    }

    /**
    For convenience, the HashStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "HashStats:"
            + "\n  hash_magic=" + hash_magic
            + "\n  hash_version=" + hash_version
            + "\n  hash_metaflags=" + hash_metaflags
            + "\n  hash_nkeys=" + hash_nkeys
            + "\n  hash_ndata=" + hash_ndata
            + "\n  hash_pagecnt=" + hash_pagecnt
            + "\n  hash_pagesize=" + hash_pagesize
            + "\n  hash_ffactor=" + hash_ffactor
            + "\n  hash_buckets=" + hash_buckets
            + "\n  hash_free=" + hash_free
            + "\n  hash_bfree=" + hash_bfree
            + "\n  hash_bigpages=" + hash_bigpages
            + "\n  hash_big_bfree=" + hash_big_bfree
            + "\n  hash_overflows=" + hash_overflows
            + "\n  hash_ovfl_free=" + hash_ovfl_free
            + "\n  hash_dup=" + hash_dup
            + "\n  hash_dup_free=" + hash_dup_free
            ;
    }
}
