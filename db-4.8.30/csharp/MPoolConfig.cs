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
    /// A class representing configuration parameters for a
    /// <see cref="DatabaseEnvironment"/>'s memory pool subsystem.
    /// </summary>
    public class MPoolConfig {
        /// <summary>
        /// The size of the shared memory buffer pool — that is, the cache.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The cache should be the size of the normal working data set of the
        /// application, with some small amount of additional memory for unusual
        /// situations. (Note: the working set is not the same as the number of
        /// pages accessed simultaneously, and is usually much larger.)
        /// </para>
        /// <para>
        /// The default cache size is 256KB, and may not be specified as less
        /// than 20KB. Any cache size less than 500MB is automatically increased
        /// by 25% to account for buffer pool overhead; cache sizes larger than
        /// 500MB are used as specified. The maximum size of a single cache is
        /// 4GB on 32-bit systems and 10TB on 64-bit systems. (All sizes are in
        /// powers-of-two, that is, 256KB is 2^18 not 256,000.) For information
        /// on tuning the Berkeley DB cache size, see Selecting a cache size in
        /// the Programmer's Reference Guide.
        /// </para>
        /// </remarks>
        public CacheInfo CacheSize;
        /// <summary>
        /// The maximum cache size.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The specified size is rounded to the nearest multiple of the cache
        /// region size, which is the initial cache size divided by
        /// <see cref="CacheInfo.NCaches">CacheSize.NCaches</see>. If no value
        /// is specified, it defaults to the initial cache size.
        /// </para>
        /// </remarks>
        public CacheInfo MaxCacheSize;
        
        internal bool maxOpenFDIsSet;
        private int maxOpenFD;
        /// <summary>
        /// The number of file descriptors the library will open concurrently
        /// when flushing dirty pages from the cache.
        /// </summary>
        public int MaxOpenFiles {
            get { return maxOpenFD; }
            set {
                maxOpenFDIsSet = true;
                maxOpenFD = value;
            }
        }

        internal bool maxSeqWriteIsSet;
        private int maxSeqWrites;
        private uint maxSeqWritesPause;
        /// <summary>
        /// Limit the number of sequential write operations scheduled by the
        /// library when flushing dirty pages from the cache.
        /// </summary>
        /// <param name="maxWrites">
        /// The maximum number of sequential write operations scheduled by the
        /// library when flushing dirty pages from the cache, or 0 if there is
        /// no limitation on the number of sequential write operations.
        /// </param>
        /// <param name="pause">
        /// The number of microseconds the thread of control should pause before
        /// scheduling further write operations. It must be specified as an
        /// unsigned 32-bit number of microseconds, limiting the maximum pause
        /// to roughly 71 minutes.
        /// </param>
        public void SetMaxSequentialWrites(int maxWrites, uint pause) {
            maxSeqWriteIsSet = true;
            maxSeqWrites = maxWrites;
            maxSeqWritesPause = pause;
        }
        /// <summary>
        /// The number of microseconds the thread of control should pause before
        /// scheduling further write operations.
        /// </summary>
        public uint SequentialWritePause { get { return maxSeqWritesPause; } }
        /// <summary>
        /// The number of sequential write operations scheduled by the library
        /// when flushing dirty pages from the cache. 
        /// </summary>
        public int MaxSequentialWrites { get { return maxSeqWrites; } }

        internal bool mmapSizeSet;
        private uint mmap_size;
        /// <summary>
        /// The maximum file size, in bytes, for a file to be mapped into the
        /// process address space. If no value is specified, it defaults to
        /// 10MB. 
        /// </summary>
        /// <remarks>
        /// Files that are opened read-only in the cache (and that satisfy a few
        /// other criteria) are, by default, mapped into the process address
        /// space instead of being copied into the local cache. This can result
        /// in better-than-usual performance because available virtual memory is
        /// normally much larger than the local cache, and page faults are
        /// faster than page copying on many systems. However, it can cause
        /// resource starvation in the presence of limited virtual memory, and
        /// it can result in immense process sizes in the presence of large
        /// databases.
        /// </remarks>
        public uint MMapSize {
            get { return mmap_size; }
            set {
                mmapSizeSet = true;
                mmap_size = value;
            }
        }

    }
}
