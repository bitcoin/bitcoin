/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// Statistical information about the memory pool subsystem
    /// </summary>
    public class MPoolStats {
        private Internal.MPoolStatStruct st;
        private CacheInfo ci;
        private List<MPoolFileStats> mempfiles;

        internal MPoolStats(Internal.MempStatStruct stats) {
            st = stats.st;
            ci = new CacheInfo(st.st_gbytes, st.st_bytes, (int)st.st_max_ncache);
            mempfiles = new List<MPoolFileStats>();
            foreach (Internal.MPoolFileStatStruct file in stats.files)
                mempfiles.Add(new MPoolFileStats(file));
        }
        /// <summary>
        /// Total cache size and number of regions
        /// </summary>
        public CacheInfo CacheSettings { get { return ci; } }
        /// <summary>
        /// Maximum number of regions. 
        /// </summary>
        public uint CacheRegions { get { return st.st_ncache; } }
        /// <summary>
        /// Maximum file size for mmap. 
        /// </summary>
        public ulong MaxMMapSize { get { return (ulong)st.st_mmapsize.ToInt64(); } }
        /// <summary>
        /// Maximum number of open fd's. 
        /// </summary>
        public int MaxOpenFileDescriptors { get { return st.st_maxopenfd; } }
        /// <summary>
        /// Maximum buffers to write. 
        /// </summary>
        public int MaxBufferWrites { get { return st.st_maxwrite; } }
        /// <summary>
        /// Sleep after writing max buffers. 
        /// </summary>
        public uint MaxBufferWritesSleep { get { return st.st_maxwrite_sleep; } }
        /// <summary>
        /// Total number of pages. 
        /// </summary>
        public uint Pages { get { return st.st_pages; } }
        /// <summary>
        /// Pages from mapped files. 
        /// </summary>
        public uint MappedPages { get { return st.st_map; } }
        /// <summary>
        /// Pages found in the cache. 
        /// </summary>
        public ulong PagesInCache { get { return st.st_cache_hit; } }
        /// <summary>
        /// Pages not found in the cache. 
        /// </summary>
        public ulong PagesNotInCache { get { return st.st_cache_miss; } }
        /// <summary>
        /// Pages created in the cache. 
        /// </summary>
        public ulong PagesCreatedInCache { get { return st.st_page_create; } }
        /// <summary>
        /// Pages read in. 
        /// </summary>
        public ulong PagesRead { get { return st.st_page_in; } }
        /// <summary>
        /// Pages written out. 
        /// </summary>
        public ulong PagesWritten { get { return st.st_page_out; } }
        /// <summary>
        /// Clean pages forced from the cache. 
        /// </summary>
        public ulong CleanPagesEvicted { get { return st.st_ro_evict; } }
        /// <summary>
        /// Dirty pages forced from the cache. 
        /// </summary>
        public ulong DirtyPagesEvicted { get { return st.st_rw_evict; } }
        /// <summary>
        /// Pages written by memp_trickle. 
        /// </summary>
        public ulong PagesTrickled { get { return st.st_page_trickle; } }
        /// <summary>
        /// Clean pages. 
        /// </summary>
        public uint CleanPages { get { return st.st_page_clean; } }
        /// <summary>
        /// Dirty pages. 
        /// </summary>
        public uint DirtyPages { get { return st.st_page_dirty; } }
        /// <summary>
        /// Number of hash buckets. 
        /// </summary>
        public uint HashBuckets { get { return st.st_hash_buckets; } }
        /// <summary>
        /// Assumed page size.
        /// </summary>
        public uint PageSize { get { return st.st_pagesize; } }
        /// <summary>
        /// Total hash chain searches. 
        /// </summary>
        public uint HashChainSearches { get { return st.st_hash_searches; } }
        /// <summary>
        /// Longest hash chain searched. 
        /// </summary>
        public uint LongestHashChainSearch { get { return st.st_hash_longest; } }
        /// <summary>
        /// Total hash entries searched. 
        /// </summary>
        public ulong HashEntriesSearched { get { return st.st_hash_examined; } }
        /// <summary>
        /// Hash lock granted with nowait. 
        /// </summary>
        public ulong HashLockNoWait { get { return st.st_hash_nowait; } }
        /// <summary>
        /// Hash lock granted after wait. 
        /// </summary>
        public ulong HashLockWait { get { return st.st_hash_wait; } }
        /// <summary>
        /// Max hash lock granted with nowait. 
        /// </summary>
        public ulong MaxHashLockNoWait { get { return st.st_hash_max_nowait; } }
        /// <summary>
        /// Max hash lock granted after wait. 
        /// </summary>
        public ulong MaxHashLockWait { get { return st.st_hash_max_wait; } }
        /// <summary>
        /// Region lock granted with nowait. 
        /// </summary>
        public ulong RegionLockNoWait { get { return st.st_region_nowait; } }
        /// <summary>
        /// Region lock granted after wait. 
        /// </summary>
        public ulong RegionLockWait { get { return st.st_region_wait; } }
        /// <summary>
        /// Buffers frozen. 
        /// </summary>
        public ulong FrozenBuffers { get { return st.st_mvcc_frozen; } }
        /// <summary>
        /// Buffers thawed. 
        /// </summary>
        public ulong ThawedBuffers { get { return st.st_mvcc_thawed; } }
        /// <summary>
        /// Frozen buffers freed. 
        /// </summary>
        public ulong FrozenBuffersFreed { get { return st.st_mvcc_freed; } }
        /// <summary>
        /// Number of page allocations. 
        /// </summary>
        public ulong PageAllocations { get { return st.st_alloc; } }
        /// <summary>
        /// Buckets checked during allocation. 
        /// </summary>
        public ulong BucketsCheckedDuringAlloc { get { return st.st_alloc_buckets; } }
        /// <summary>
        /// Max checked during allocation. 
        /// </summary>
        public ulong MaxBucketsCheckedDuringAlloc { get { return st.st_alloc_max_buckets; } }
        /// <summary>
        /// Pages checked during allocation. 
        /// </summary>
        public ulong PagesCheckedDuringAlloc { get { return st.st_alloc_pages; } }
        /// <summary>
        /// Max checked during allocation. 
        /// </summary>
        public ulong MaxPagesCheckedDuringAlloc { get { return st.st_alloc_max_pages; } }
        /// <summary>
        /// Thread waited on buffer I/O. 
        /// </summary>
        public ulong BlockedOperations { get { return st.st_io_wait; } }
        /// <summary>
        /// Number of times sync interrupted.
        /// </summary>
        public ulong SyncInterrupted { get { return st.st_sync_interrupted; } }
        /// <summary>
        /// Region size. 
        /// </summary>
        public ulong RegionSize { get { return (ulong)st.st_regsize.ToInt64(); } }
        /// <summary>
        /// Stats for files open in the memory pool
        /// </summary>
        public List<MPoolFileStats> Files { get { return mempfiles; } }
    }
}