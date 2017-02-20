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
    /// Statistical information about a file in the memory pool
    /// </summary>
    public class MPoolFileStats {
        private Internal.MPoolFileStatStruct st;
        internal MPoolFileStats(Internal.MPoolFileStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// File name.
        /// </summary>
        public string FileName { get { return st.file_name; } }
        /// <summary>
        /// Pages from mapped files. 
        /// </summary>
        public uint MappedPages { get { return st.st_map; } }
        /// <summary>
        /// Pages created in the cache. 
        /// </summary>
        public ulong PagesCreatedInCache { get { return st.st_page_create; } }
        /// <summary>
        /// Pages found in the cache. 
        /// </summary>
        public ulong PagesInCache { get { return st.st_cache_hit; } }
        /// <summary>
        /// Pages not found in the cache. 
        /// </summary>
        public ulong PagesNotInCache { get { return st.st_cache_miss; } }
        /// <summary>
        /// Pages read in. 
        /// </summary>
        public ulong PagesRead { get { return st.st_page_in; } }
        /// <summary>
        /// Page size. 
        /// </summary>
        public uint PageSize { get { return st.st_pagesize; } }
        /// <summary>
        /// Pages written out. 
        /// </summary>
        public ulong PagesWritten { get { return st.st_page_out; } }
    }
}