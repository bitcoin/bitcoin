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
    /// Statistical information about a Sequence
    /// </summary>
    public class SequenceStats {
        private Internal.SequenceStatStruct st;
        internal SequenceStats(Internal.SequenceStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Cache size. 
        /// </summary>
        public int CacheSize { get { return st.st_cache_size; } }
        /// <summary>
        /// Current cached value. 
        /// </summary>
        public long CachedValue { get { return st.st_value; } }
        /// <summary>
        /// Flag value. 
        /// </summary>
        public uint Flags { get { return st.st_flags; } }
        /// <summary>
        /// Last cached value. 
        /// </summary>
        public long LastCachedValue { get { return st.st_last_value; } }
        /// <summary>
        /// Sequence lock granted w/o wait. 
        /// </summary>
        public ulong LockWait { get { return st.st_wait; } }
        /// <summary>
        /// Sequence lock granted after wait. 
        /// </summary>
        public ulong LockNoWait { get { return st.st_nowait; } }
        /// <summary>
        /// Maximum value. 
        /// </summary>
        public long Max { get { return st.st_max; } }
        /// <summary>
        /// Minimum value. 
        /// </summary>
        public long Min { get { return st.st_min; } }
        /// <summary>
        /// Current value in db. 
        /// </summary>
        public long StoredValue { get { return st.st_current; } }
    }
}