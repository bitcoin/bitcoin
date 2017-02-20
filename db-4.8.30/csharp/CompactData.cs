/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class for representing compact operation statistics
    /// </summary>
    public class CompactData {
        private DB_COMPACT cdata;
        private DatabaseEntry _end;

        internal CompactData(DB_COMPACT dbcompact, DatabaseEntry end) {
            cdata = dbcompact;
            _end = end;
        }

        /// <summary>
        /// If no <see cref="Transaction"/> parameter was specified, the
        /// number of deadlocks which occurred. 
        /// </summary>
        public uint Deadlocks {
            get { return  cdata.compact_deadlock; }
        }
        /// <summary>
        /// The number of levels removed from the Btree or Recno database during
        /// the compaction phase. 
        /// </summary>
        public uint Levels {
            get { return  cdata.compact_levels; }
        }
        /// <summary>
        /// The number of database pages reviewed during the compaction phase. 
        /// </summary>
        public uint PagesExamined {
            get { return  cdata.compact_pages_examine; }
        }
        /// <summary>
        /// The number of database pages freed during the compaction phase. 
        /// </summary>
        public uint PagesFreed {
            get { return  cdata.compact_pages_free; }
        }
        /// <summary>
        /// The number of database pages returned to the filesystem.
        /// </summary>
        public uint PagesTruncated {
            get { return  cdata.compact_pages_truncated; }
        }
        /// <summary>
        /// The database key marking the end of the compaction operation.  This
        /// is generally the first key of the page where the operation stopped
        /// and is only non-null if <see cref="CompactConfig.returnEnd"/> was
        /// true.
        /// </summary>
        public DatabaseEntry End {
            get { return _end; }
        }
                
    }
}