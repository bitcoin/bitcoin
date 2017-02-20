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
    /// Statistical information about the mutex subsystem
    /// </summary>
    public class MutexStats {
        private Internal.MutexStatStruct st;
        internal MutexStats(Internal.MutexStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Mutex alignment 
        /// </summary>
        public uint Alignment { get { return st.st_mutex_align; } }
        /// <summary>
        /// Available mutexes 
        /// </summary>
        public uint Available { get { return st.st_mutex_free; } }
        /// <summary>
        /// Mutex count 
        /// </summary>
        public uint Count { get { return st.st_mutex_cnt; } }
        /// <summary>
        /// Mutexes in use 
        /// </summary>
        public uint InUse { get { return st.st_mutex_inuse; } }
        /// <summary>
        /// Maximum mutexes ever in use 
        /// </summary>
        public uint MaxInUse { get { return st.st_mutex_inuse_max; } }
        /// <summary>
        /// Region lock granted without wait. 
        /// </summary>
        public ulong RegionNoWait { get { return st.st_region_nowait; } }
        /// <summary>
        /// Region size. 
        /// </summary>
        public ulong RegionSize { get { return (ulong)st.st_regsize.ToInt64(); } }
        /// <summary>
        /// Region lock granted after wait. 
        /// </summary>
        public ulong RegionWait { get { return st.st_region_wait; } }
        /// <summary>
        /// Mutex test-and-set spins 
        /// </summary>
        public uint TASSpins { get { return st.st_mutex_tas_spins; } }
    }
}