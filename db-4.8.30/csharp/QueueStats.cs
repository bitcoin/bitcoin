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
    /// Statistical information about a QueueDatabase
    /// </summary>
    public class QueueStats {
        private Internal.QueueStatStruct st;
        internal QueueStats(Internal.QueueStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Data pages. 
        /// </summary>
        public uint DataPages { get { return st.qs_pages; } }
        /// <summary>
        /// Bytes free in data pages. 
        /// </summary>
        public uint DataPagesBytesFree { get { return st.qs_pgfree; } }
        /// <summary>
        /// First not deleted record. 
        /// </summary>
        public uint FirstRecordNumber { get { return st.qs_first_recno; } }
        /// <summary>
        /// Magic number. 
        /// </summary>
        public uint MagicNumber { get { return st.qs_magic; } }
        /// <summary>
        /// Metadata flags. 
        /// </summary>
        public uint MetadataFlags { get { return st.qs_metaflags; } }
        /// <summary>
        /// Next available record number. 
        /// </summary>
        public uint NextRecordNumber { get { return st.qs_cur_recno; } }
        /// <summary>
        /// Number of data items. 
        /// </summary>
        public uint nData { get { return st.qs_ndata; } }
        /// <summary>
        /// Number of unique keys. 
        /// </summary>
        public uint nKeys { get { return st.qs_nkeys; } }
        /// <summary>
        /// Page size. 
        /// </summary>
        public uint PageSize { get { return st.qs_pagesize; } }
        /// <summary>
        /// Pages per extent. 
        /// </summary>
        public uint PagesPerExtent { get { return st.qs_extentsize; } }
        /// <summary>
        /// Fixed-length record length. 
        /// </summary>
        public uint RecordLength { get { return st.qs_re_len; } }
        /// <summary>
        /// Fixed-length record pad. 
        /// </summary>
        public uint RecordPadByte { get { return st.qs_re_pad; } }
        /// <summary>
        /// Version number. 
        /// </summary>
        public uint Version { get { return st.qs_version; } }
    }
}