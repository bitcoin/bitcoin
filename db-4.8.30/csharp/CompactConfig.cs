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
    /// A class to represent configuration settings for
    /// <see cref="BTreeDatabase.Compact"/> and
    /// <see cref="RecnoDatabase.Compact"/>.
    /// </summary>
    public class CompactConfig {
        private DB_COMPACT cdata;
        
        private bool fillPctSet;
        private uint fillpct;
        private bool pgsSet;
        private uint pgs;
        private bool tmoutSet;
        private uint tmout;
        /// <summary>
        /// Return the database key marking the end of the compaction operation
        /// in a Btree or Recno database. This is generally the first key of the
        /// page where the operation stopped. 
        /// </summary>
        public bool returnEnd;
        /// <summary>
        /// If non-null, the starting point for compaction. Compaction will
        /// start at the smallest key greater than or equal to
        /// <paramref name="start"/>. If null, compaction will start at the
        /// beginning of the database. 
        /// </summary>
        public DatabaseEntry start;
        /// <summary>
        /// If non-null, the stopping point for compaction. Compaction will stop
        /// at the page with the smallest key greater than
        /// <paramref name="stop"/>. If null, compaction will stop at the end of
        /// the database. 
        /// </summary>
        public DatabaseEntry stop;
        /// <summary>
        /// If true, return pages to the filesystem when possible. If false,
        /// pages emptied as a result of compaction will be placed on the free
        /// list for re-use, but never returned to the filesystem.
        /// </summary>
        /// <remarks>
        /// Note that only pages at the end of a file can be returned to the
        /// filesystem. Because of the one-pass nature of the compaction
        /// algorithm, any unemptied page near the end of the file inhibits
        /// returning pages to the file system. A repeated call to
        /// <see cref="BTreeDatabase.Compact"/> or 
        /// <see cref="RecnoDatabase.Compact"/> with a low
        /// <see cref="FillPercentage"/> may be used to return pages in this
        /// case. 
        /// </remarks>
        public bool TruncatePages;

        static internal DB_COMPACT getDB_COMPACT(CompactConfig compactData) {
            if (compactData == null)
                return null;

            if (compactData.cdata == null)
                compactData.doCompaction();
            return compactData.cdata;
        }
        
        /// <summary>
        /// Create a new CompactConfig object
        /// </summary>
        public CompactConfig() { }

        /// <summary>
        /// If non-zero, this provides the goal for filling pages, specified as
        /// a percentage between 1 and 100. Any page not at or above this
        /// percentage full will be considered for compaction. The default
        /// behavior is to consider every page for compaction, regardless of its
        /// page fill percentage. 
        /// </summary>
        public uint FillPercentage {
            get { return fillpct; }
            set {
                fillpct = value;
                fillPctSet = true;
            }
        }
        /// <summary>
        /// If non-zero, compaction will complete after the specified number of
        /// pages have been freed.
        /// </summary>
        public uint Pages {
            get { return pgs; }
            set {
                pgs = value;
                pgsSet = true;
            }
        }
        /// <summary>
        /// If non-zero, and no <see cref="Transaction"/> is specified, this
        /// parameter identifies the lock timeout used for implicit
        /// transactions, in microseconds. 
        /// </summary>
        public uint Timeout {
            get { return tmout; }
            set {
                tmout = value;
                tmoutSet = true;
            }
        }

        private void doCompaction() {
            cdata = new DB_COMPACT();

            if (fillPctSet)
                cdata.compact_fillpercent = fillpct;
            if (pgsSet)
                cdata.compact_pages = pgs;
            if (tmoutSet)
                cdata.compact_timeout = tmout;
        }

        internal uint flags {
            get {
                uint ret = 0;
                ret |= TruncatePages ? DbConstants.DB_FREE_SPACE : 0;
                return ret;
            }
        }
    }
}