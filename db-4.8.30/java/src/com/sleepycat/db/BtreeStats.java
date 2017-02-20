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
The BtreeStats object is used to return Btree
or Recno database statistics.
*/
public class BtreeStats extends DatabaseStats {
    // no public constructor
    /* package */ BtreeStats() {}

    private int bt_magic;
    /** TODO */
    public int getMagic() {
        return bt_magic;
    }

    private int bt_version;
    /** TODO */
    public int getVersion() {
        return bt_version;
    }

    private int bt_metaflags;
    /** TODO */
    public int getMetaFlags() {
        return bt_metaflags;
    }

    private int bt_nkeys;
    /** TODO */
    public int getNumKeys() {
        return bt_nkeys;
    }

    private int bt_ndata;
    /** TODO */
    public int getNumData() {
        return bt_ndata;
    }

    private int bt_pagecnt;
    /** TODO */
    public int getPageCount() {
        return bt_pagecnt;
    }

    private int bt_pagesize;
    /** TODO */
    public int getPageSize() {
        return bt_pagesize;
    }

    private int bt_minkey;
    /** TODO */
    public int getMinKey() {
        return bt_minkey;
    }

    private int bt_re_len;
    /** TODO */
    public int getReLen() {
        return bt_re_len;
    }

    private int bt_re_pad;
    /** TODO */
    public int getRePad() {
        return bt_re_pad;
    }

    private int bt_levels;
    /** TODO */
    public int getLevels() {
        return bt_levels;
    }

    private int bt_int_pg;
    /** TODO */
    public int getIntPages() {
        return bt_int_pg;
    }

    private int bt_leaf_pg;
    /** TODO */
    public int getLeafPages() {
        return bt_leaf_pg;
    }

    private int bt_dup_pg;
    /** TODO */
    public int getDupPages() {
        return bt_dup_pg;
    }

    private int bt_over_pg;
    /** TODO */
    public int getOverPages() {
        return bt_over_pg;
    }

    private int bt_empty_pg;
    /** TODO */
    public int getEmptyPages() {
        return bt_empty_pg;
    }

    private int bt_free;
    /** TODO */
    public int getFree() {
        return bt_free;
    }

    private long bt_int_pgfree;
    /** TODO */
    public long getIntPagesFree() {
        return bt_int_pgfree;
    }

    private long bt_leaf_pgfree;
    /** TODO */
    public long getLeafPagesFree() {
        return bt_leaf_pgfree;
    }

    private long bt_dup_pgfree;
    /** TODO */
    public long getDupPagesFree() {
        return bt_dup_pgfree;
    }

    private long bt_over_pgfree;
    /** TODO */
    public long getOverPagesFree() {
        return bt_over_pgfree;
    }

    /**
    For convenience, the BtreeStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "BtreeStats:"
            + "\n  bt_magic=" + bt_magic
            + "\n  bt_version=" + bt_version
            + "\n  bt_metaflags=" + bt_metaflags
            + "\n  bt_nkeys=" + bt_nkeys
            + "\n  bt_ndata=" + bt_ndata
            + "\n  bt_pagecnt=" + bt_pagecnt
            + "\n  bt_pagesize=" + bt_pagesize
            + "\n  bt_minkey=" + bt_minkey
            + "\n  bt_re_len=" + bt_re_len
            + "\n  bt_re_pad=" + bt_re_pad
            + "\n  bt_levels=" + bt_levels
            + "\n  bt_int_pg=" + bt_int_pg
            + "\n  bt_leaf_pg=" + bt_leaf_pg
            + "\n  bt_dup_pg=" + bt_dup_pg
            + "\n  bt_over_pg=" + bt_over_pg
            + "\n  bt_empty_pg=" + bt_empty_pg
            + "\n  bt_free=" + bt_free
            + "\n  bt_int_pgfree=" + bt_int_pgfree
            + "\n  bt_leaf_pgfree=" + bt_leaf_pgfree
            + "\n  bt_dup_pgfree=" + bt_dup_pgfree
            + "\n  bt_over_pgfree=" + bt_over_pgfree
            ;
    }
}
