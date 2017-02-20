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
The QueueStats object is used to return Queue database statistics.
*/
public class QueueStats extends DatabaseStats {
    // no public constructor
    /* package */ QueueStats() {}

    private int qs_magic;
    /** TODO */
    public int getMagic() {
        return qs_magic;
    }

    private int qs_version;
    /** TODO */
    public int getVersion() {
        return qs_version;
    }

    private int qs_metaflags;
    /** TODO */
    public int getMetaFlags() {
        return qs_metaflags;
    }

    private int qs_nkeys;
    /** TODO */
    public int getNumKeys() {
        return qs_nkeys;
    }

    private int qs_ndata;
    /** TODO */
    public int getNumData() {
        return qs_ndata;
    }

    private int qs_pagesize;
    /** TODO */
    public int getPageSize() {
        return qs_pagesize;
    }

    private int qs_extentsize;
    /** TODO */
    public int getExtentSize() {
        return qs_extentsize;
    }

    private int qs_pages;
    /** TODO */
    public int getPages() {
        return qs_pages;
    }

    private int qs_re_len;
    /** TODO */
    public int getReLen() {
        return qs_re_len;
    }

    private int qs_re_pad;
    /** TODO */
    public int getRePad() {
        return qs_re_pad;
    }

    private int qs_pgfree;
    /** TODO */
    public int getPagesFree() {
        return qs_pgfree;
    }

    private int qs_first_recno;
    /** TODO */
    public int getFirstRecno() {
        return qs_first_recno;
    }

    private int qs_cur_recno;
    /** TODO */
    public int getCurRecno() {
        return qs_cur_recno;
    }

    /**
    For convenience, the QueueStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "QueueStats:"
            + "\n  qs_magic=" + qs_magic
            + "\n  qs_version=" + qs_version
            + "\n  qs_metaflags=" + qs_metaflags
            + "\n  qs_nkeys=" + qs_nkeys
            + "\n  qs_ndata=" + qs_ndata
            + "\n  qs_pagesize=" + qs_pagesize
            + "\n  qs_extentsize=" + qs_extentsize
            + "\n  qs_pages=" + qs_pages
            + "\n  qs_re_len=" + qs_re_len
            + "\n  qs_re_pad=" + qs_re_pad
            + "\n  qs_pgfree=" + qs_pgfree
            + "\n  qs_first_recno=" + qs_first_recno
            + "\n  qs_cur_recno=" + qs_cur_recno
            ;
    }
}
