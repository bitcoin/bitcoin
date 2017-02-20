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
Cache statistics for a database environment.
*/
public class CacheStats {
    // no public constructor
    /* package */ CacheStats() {}

    private int st_gbytes;
    /** TODO */
    public int getGbytes() {
        return st_gbytes;
    }

    private int st_bytes;
    /** TODO */
    public int getBytes() {
        return st_bytes;
    }

    private int st_ncache;
    /** TODO */
    public int getNumCache() {
        return st_ncache;
    }

    private int st_max_ncache;
    /** TODO */
    public int getMaxNumCache() {
        return st_max_ncache;
    }

    private int st_mmapsize;
    /** TODO */
    public int getMmapSize() {
        return st_mmapsize;
    }

    private int st_maxopenfd;
    /** TODO */
    public int getMaxOpenfd() {
        return st_maxopenfd;
    }

    private int st_maxwrite;
    /** TODO */
    public int getMaxWrite() {
        return st_maxwrite;
    }

    private int st_maxwrite_sleep;
    /** TODO */
    public int getMaxWriteSleep() {
        return st_maxwrite_sleep;
    }

    private int st_pages;
    /** TODO */
    public int getPages() {
        return st_pages;
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

    private long st_ro_evict;
    /** TODO */
    public long getRoEvict() {
        return st_ro_evict;
    }

    private long st_rw_evict;
    /** TODO */
    public long getRwEvict() {
        return st_rw_evict;
    }

    private long st_page_trickle;
    /** TODO */
    public long getPageTrickle() {
        return st_page_trickle;
    }

    private int st_page_clean;
    /** TODO */
    public int getPageClean() {
        return st_page_clean;
    }

    private int st_page_dirty;
    /** TODO */
    public int getPageDirty() {
        return st_page_dirty;
    }

    private int st_hash_buckets;
    /** TODO */
    public int getHashBuckets() {
        return st_hash_buckets;
    }

    private int st_pagesize;
    /** TODO */
    public int getPageSize() {
        return st_pagesize;
    }

    private int st_hash_searches;
    /** TODO */
    public int getHashSearches() {
        return st_hash_searches;
    }

    private int st_hash_longest;
    /** TODO */
    public int getHashLongest() {
        return st_hash_longest;
    }

    private long st_hash_examined;
    /** TODO */
    public long getHashExamined() {
        return st_hash_examined;
    }

    private long st_hash_nowait;
    /** TODO */
    public long getHashNowait() {
        return st_hash_nowait;
    }

    private long st_hash_wait;
    /** TODO */
    public long getHashWait() {
        return st_hash_wait;
    }

    private long st_hash_max_nowait;
    /** TODO */
    public long getHashMaxNowait() {
        return st_hash_max_nowait;
    }

    private long st_hash_max_wait;
    /** TODO */
    public long getHashMaxWait() {
        return st_hash_max_wait;
    }

    private long st_region_nowait;
    /** TODO */
    public long getRegionNowait() {
        return st_region_nowait;
    }

    private long st_region_wait;
    /** TODO */
    public long getRegionWait() {
        return st_region_wait;
    }

    private long st_mvcc_frozen;
    /** TODO */
    public long getMultiversionFrozen() {
        return st_mvcc_frozen;
    }

    private long st_mvcc_thawed;
    /** TODO */
    public long getMultiversionThawed() {
        return st_mvcc_thawed;
    }

    private long st_mvcc_freed;
    /** TODO */
    public long getMultiversionFreed() {
        return st_mvcc_freed;
    }

    private long st_alloc;
    /** TODO */
    public long getAlloc() {
        return st_alloc;
    }

    private long st_alloc_buckets;
    /** TODO */
    public long getAllocBuckets() {
        return st_alloc_buckets;
    }

    private long st_alloc_max_buckets;
    /** TODO */
    public long getAllocMaxBuckets() {
        return st_alloc_max_buckets;
    }

    private long st_alloc_pages;
    /** TODO */
    public long getAllocPages() {
        return st_alloc_pages;
    }

    private long st_alloc_max_pages;
    /** TODO */
    public long getAllocMaxPages() {
        return st_alloc_max_pages;
    }

    private long st_io_wait;
    /** TODO */
    public long getIoWait() {
        return st_io_wait;
    }

    private long st_sync_interrupted;
    /** TODO */
    public long getSyncInterrupted() {
        return st_sync_interrupted;
    }

    private int st_regsize;
    /** TODO */
    public int getRegSize() {
        return st_regsize;
    }

    /**
    For convenience, the CacheStats class has a toString method that
    lists all the data fields.
    */
    public String toString() {
        return "CacheStats:"
            + "\n  st_gbytes=" + st_gbytes
            + "\n  st_bytes=" + st_bytes
            + "\n  st_ncache=" + st_ncache
            + "\n  st_max_ncache=" + st_max_ncache
            + "\n  st_mmapsize=" + st_mmapsize
            + "\n  st_maxopenfd=" + st_maxopenfd
            + "\n  st_maxwrite=" + st_maxwrite
            + "\n  st_maxwrite_sleep=" + st_maxwrite_sleep
            + "\n  st_pages=" + st_pages
            + "\n  st_map=" + st_map
            + "\n  st_cache_hit=" + st_cache_hit
            + "\n  st_cache_miss=" + st_cache_miss
            + "\n  st_page_create=" + st_page_create
            + "\n  st_page_in=" + st_page_in
            + "\n  st_page_out=" + st_page_out
            + "\n  st_ro_evict=" + st_ro_evict
            + "\n  st_rw_evict=" + st_rw_evict
            + "\n  st_page_trickle=" + st_page_trickle
            + "\n  st_page_clean=" + st_page_clean
            + "\n  st_page_dirty=" + st_page_dirty
            + "\n  st_hash_buckets=" + st_hash_buckets
            + "\n  st_pagesize=" + st_pagesize
            + "\n  st_hash_searches=" + st_hash_searches
            + "\n  st_hash_longest=" + st_hash_longest
            + "\n  st_hash_examined=" + st_hash_examined
            + "\n  st_hash_nowait=" + st_hash_nowait
            + "\n  st_hash_wait=" + st_hash_wait
            + "\n  st_hash_max_nowait=" + st_hash_max_nowait
            + "\n  st_hash_max_wait=" + st_hash_max_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_mvcc_frozen=" + st_mvcc_frozen
            + "\n  st_mvcc_thawed=" + st_mvcc_thawed
            + "\n  st_mvcc_freed=" + st_mvcc_freed
            + "\n  st_alloc=" + st_alloc
            + "\n  st_alloc_buckets=" + st_alloc_buckets
            + "\n  st_alloc_max_buckets=" + st_alloc_max_buckets
            + "\n  st_alloc_pages=" + st_alloc_pages
            + "\n  st_alloc_max_pages=" + st_alloc_max_pages
            + "\n  st_io_wait=" + st_io_wait
            + "\n  st_sync_interrupted=" + st_sync_interrupted
            + "\n  st_regsize=" + st_regsize
            ;
    }
}
