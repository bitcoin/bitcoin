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
    /// Statistical information about a BTreeDatabase
    /// </summary>
    public class BTreeStats {
        private Internal.BTreeStatStruct st;
        internal BTreeStats(Internal.BTreeStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Duplicate pages. 
        /// </summary>
        public uint DuplicatePages { get { return st.bt_dup_pg; } }
        /// <summary>
        /// Bytes free in duplicate pages. 
        /// </summary>
        public ulong DuplicatePagesFreeBytes { get { return st.bt_dup_pgfree; } }
        /// <summary>
        /// Empty pages. 
        /// </summary>
        public uint EmptyPages { get { return st.bt_empty_pg; } }
        /// <summary>
        /// Pages on the free list. 
        /// </summary>
        public uint FreePages { get { return st.bt_free; } }
        /// <summary>
        /// Internal pages. 
        /// </summary>
        public uint InternalPages { get { return st.bt_int_pg; } }
        /// <summary>
        /// Bytes free in internal pages. 
        /// </summary>
        public ulong InternalPagesFreeBytes { get { return st.bt_int_pgfree; } }
        /// <summary>
        /// Leaf pages. 
        /// </summary>
        public uint LeafPages { get { return st.bt_leaf_pg; } }
        /// <summary>
        /// Bytes free in leaf pages. 
        /// </summary>
        public ulong LeafPagesFreeBytes { get { return st.bt_leaf_pgfree; } }
        /// <summary>
        /// Tree levels. 
        /// </summary>
        public uint Levels { get { return st.bt_levels; } }
        /// <summary>
        /// Magic number. 
        /// </summary>
        public uint MagicNumber { get { return st.bt_magic; } }
        /// <summary>
        /// Metadata flags. 
        /// </summary>
        public uint MetadataFlags { get { return st.bt_metaflags; } }
        /// <summary>
        /// Minkey value. 
        /// </summary>
        public uint MinKey { get { return st.bt_minkey; } }
        /// <summary>
        /// Number of data items. 
        /// </summary>
        public uint nData { get { return st.bt_ndata; } }
        /// <summary>
        /// Number of unique keys. 
        /// </summary>
        public uint nKeys { get { return st.bt_nkeys; } }
        /// <summary>
        /// Page count. 
        /// </summary>
        public uint nPages { get { return st.bt_pagecnt; } }
        /// <summary>
        /// Overflow pages. 
        /// </summary>
        public uint OverflowPages { get { return st.bt_over_pg; } }
        /// <summary>
        /// Bytes free in overflow pages. 
        /// </summary>
        public ulong OverflowPagesFreeBytes { get { return st.bt_over_pgfree; } }
        /// <summary>
        /// Page size. 
        /// </summary>
        public uint PageSize { get { return st.bt_pagesize; } }
        /// <summary>
        /// Version number. 
        /// </summary>
        public uint Version { get { return st.bt_version; } }
    }
}
