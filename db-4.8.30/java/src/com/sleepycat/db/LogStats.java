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
Log statistics for a database environment.
*/
public class LogStats {
    // no public constructor
    /* package */ LogStats() {}

    private int st_magic;
    /** TODO */
    public int getMagic() {
        return st_magic;
    }

    private int st_version;
    /** TODO */
    public int getVersion() {
        return st_version;
    }

    private int st_mode;
    /** TODO */
    public int getMode() {
        return st_mode;
    }

    private int st_lg_bsize;
    /** TODO */
    public int getLgBSize() {
        return st_lg_bsize;
    }

    private int st_lg_size;
    /** TODO */
    public int getLgSize() {
        return st_lg_size;
    }

    private int st_wc_bytes;
    /** TODO */
    public int getWcBytes() {
        return st_wc_bytes;
    }

    private int st_wc_mbytes;
    /** TODO */
    public int getWcMbytes() {
        return st_wc_mbytes;
    }

    private long st_record;
    /** TODO */
    public long getRecord() {
        return st_record;
    }

    private int st_w_bytes;
    /** TODO */
    public int getWBytes() {
        return st_w_bytes;
    }

    private int st_w_mbytes;
    /** TODO */
    public int getWMbytes() {
        return st_w_mbytes;
    }

    private long st_wcount;
    /** TODO */
    public long getWCount() {
        return st_wcount;
    }

    private long st_wcount_fill;
    /** TODO */
    public long getWCountFill() {
        return st_wcount_fill;
    }

    private long st_rcount;
    /** TODO */
    public long getRCount() {
        return st_rcount;
    }

    private long st_scount;
    /** TODO */
    public long getSCount() {
        return st_scount;
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

    private int st_cur_file;
    /** TODO */
    public int getCurFile() {
        return st_cur_file;
    }

    private int st_cur_offset;
    /** TODO */
    public int getCurOffset() {
        return st_cur_offset;
    }

    private int st_disk_file;
    /** TODO */
    public int getDiskFile() {
        return st_disk_file;
    }

    private int st_disk_offset;
    /** TODO */
    public int getDiskOffset() {
        return st_disk_offset;
    }

    private int st_maxcommitperflush;
    /** TODO */
    public int getMaxCommitperflush() {
        return st_maxcommitperflush;
    }

    private int st_mincommitperflush;
    /** TODO */
    public int getMinCommitperflush() {
        return st_mincommitperflush;
    }

    private int st_regsize;
    /** TODO */
    public int getRegSize() {
        return st_regsize;
    }

    /**
    For convenience, the LogStats class has a toString method that lists
    all the data fields.
    */
    public String toString() {
        return "LogStats:"
            + "\n  st_magic=" + st_magic
            + "\n  st_version=" + st_version
            + "\n  st_mode=" + st_mode
            + "\n  st_lg_bsize=" + st_lg_bsize
            + "\n  st_lg_size=" + st_lg_size
            + "\n  st_wc_bytes=" + st_wc_bytes
            + "\n  st_wc_mbytes=" + st_wc_mbytes
            + "\n  st_record=" + st_record
            + "\n  st_w_bytes=" + st_w_bytes
            + "\n  st_w_mbytes=" + st_w_mbytes
            + "\n  st_wcount=" + st_wcount
            + "\n  st_wcount_fill=" + st_wcount_fill
            + "\n  st_rcount=" + st_rcount
            + "\n  st_scount=" + st_scount
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_cur_file=" + st_cur_file
            + "\n  st_cur_offset=" + st_cur_offset
            + "\n  st_disk_file=" + st_disk_file
            + "\n  st_disk_offset=" + st_disk_offset
            + "\n  st_maxcommitperflush=" + st_maxcommitperflush
            + "\n  st_mincommitperflush=" + st_mincommitperflush
            + "\n  st_regsize=" + st_regsize
            ;
    }
}
