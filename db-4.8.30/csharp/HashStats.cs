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
    /// Statistical information about a HashDatabase
    /// </summary>
    public class HashStats {
        private Internal.HashStatStruct st;
        internal HashStats(Internal.HashStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Number of big key/data pages. 
        /// </summary>
        public uint BigPages { get { return st.hash_bigpages; } }
        /// <summary>
        /// Bytes free on big item pages. 
        /// </summary>
        public ulong BigPagesFreeBytes { get { return st.hash_big_bfree; } }
        /// <summary>
        /// Bytes free on bucket pages. 
        /// </summary>
        public ulong BucketPagesFreeBytes { get { return st.hash_bfree; } }
        /// <summary>
        /// Number of dup pages. 
        /// </summary>
        public uint DuplicatePages { get { return st.hash_dup; } }
        /// <summary>
        /// Bytes free on duplicate pages. 
        /// </summary>
        public ulong DuplicatePagesFreeBytes { get { return st.hash_dup_free; } }
        /// <summary>
        /// Fill factor specified at create. 
        /// </summary>
        public uint FillFactor { get { return st.hash_ffactor; } }
        /// <summary>
        /// Pages on the free list. 
        /// </summary>
        public uint FreePages { get { return st.hash_free; } }
        /// <summary>
        /// Metadata flags. 
        /// </summary>
        public uint MetadataFlags { get { return st.hash_metaflags; } }
        /// <summary>
        /// Magic number. 
        /// </summary>
        public uint MagicNumber { get { return st.hash_magic; } }
        /// <summary>
        /// Number of data items. 
        /// </summary>
        public uint nData { get { return st.hash_ndata; } }
        /// <summary>
        /// Number of hash buckets. 
        /// </summary>
        public uint nHashBuckets { get { return st.hash_buckets; } }
        /// <summary>
        /// Number of unique keys. 
        /// </summary>
        public uint nKeys { get { return st.hash_nkeys; } }
        /// <summary>
        /// Number of overflow pages. 
        /// </summary>
        public uint OverflowPages { get { return st.hash_overflows; } }
        /// <summary>
        /// Bytes free on ovfl pages. 
        /// </summary>
        public ulong OverflowPagesFreeBytes { get { return st.hash_ovfl_free; } }
        /// <summary>
        /// Page count. 
        /// </summary>
        public uint nPages { get { return st.hash_pagecnt; } }
        /// <summary>
        /// Page size. 
        /// </summary>
        public uint PageSize { get { return st.hash_pagesize; } }
        /// <summary>
        /// Version number. 
        /// </summary>
        public uint Version { get { return st.hash_version; } }

    }
}