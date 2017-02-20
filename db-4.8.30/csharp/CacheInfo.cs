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
    /// A class to represent information about the Berkeley DB cache
    /// </summary>
    public class CacheInfo {
        /// <summary>
        /// The number of gigabytes in the cache
        /// </summary>
        public uint Gigabytes;
        /// <summary>
        /// The number of bytes in the cache
        /// </summary>
        public uint Bytes;
        /// <summary>
        /// The number of caches
        /// </summary>
        public int NCaches;

        /// <summary>
        /// Create a new CacheInfo object.  The size of the cache is set to 
        /// gbytes gigabytes plus bytes and spread over numCaches separate
        /// caches.
        /// </summary>
        /// <param name="gbytes">The number of gigabytes in the cache</param>
        /// <param name="bytes">The number of bytes in the cache</param>
        /// <param name="numCaches">The number of caches</param>
        public CacheInfo(uint gbytes, uint bytes, int numCaches) {
            Gigabytes = gbytes;
            Bytes = bytes;
            NCaches = numCaches;
        }
    }
}
