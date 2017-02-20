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
Statistics for a file in the cache.
*/
public class CacheFileStats {
    // no public constructor
    /* package */ CacheFileStats() {}

    private String file_name;
    /** TODO */
    public String getFileName() {
        return file_name;
    }

    private int st_pagesize;
    /** TODO */
    public int getPageSize() {
        return st_pagesize;
    }

    private int st_map;
    /** TODO */
    public int getMap() {
        return st_map;
    }

    private long st_cache_hit;
    /** TODO */
    public long getCacheHit() {
        return st_cache_hit;
    }

    private long st_cache_miss;
    /** TODO */
    public long getCacheMiss() {
        return st_cache_miss;
    }

    private long st_page_create;
    /** TODO */
    public long getPageCreate() {
        return st_page_create;
    }

    private long st_page_in;
    /** TODO */
    public long getPageIn() {
        return st_page_in;
    }

    private long st_page_out;
    /** TODO */
    public long getPageOut() {
        return st_page_out;
    }

    /**
    For convenience, the CacheFileStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "CacheFileStats:"
            + "\n  file_name=" + file_name
            + "\n  st_pagesize=" + st_pagesize
            + "\n  st_map=" + st_map
            + "\n  st_cache_hit=" + st_cache_hit
            + "\n  st_cache_miss=" + st_cache_miss
            + "\n  st_page_create=" + st_page_create
            + "\n  st_page_in=" + st_page_in
            + "\n  st_page_out=" + st_page_out
            ;
    }
}
