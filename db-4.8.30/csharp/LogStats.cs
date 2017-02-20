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
    /// Statistical information about the logging subsystem
    /// </summary>
    public class LogStats {
        private Internal.LogStatStruct st;
        internal LogStats(Internal.LogStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Log buffer size. 
        /// </summary>
        public uint BufferSize { get { return st.st_lg_bsize; } }
        /// <summary>
        /// Bytes to log. 
        /// </summary>
        public uint Bytes { get { return st.st_w_bytes; } }
        /// <summary>
        /// Bytes to log since checkpoint. 
        /// </summary>
        public uint BytesSinceCheckpoint { get { return st.st_wc_bytes; } }
        /// <summary>
        /// Current log file number. 
        /// </summary>
        public uint CurrentFile { get { return st.st_cur_file; } }
        /// <summary>
        /// Current log file offset. 
        /// </summary>
        public uint CurrentOffset { get { return st.st_cur_offset; } }
        /// <summary>
        /// Known on disk log file number. 
        /// </summary>
        public uint DiskFileNumber { get { return st.st_disk_file; } }
        /// <summary>
        /// Known on disk log file offset. 
        /// </summary>
        public uint DiskOffset { get { return st.st_disk_offset; } }
        /// <summary>
        /// Log file size. 
        /// </summary>
        public uint FileSize { get { return st.st_lg_size; } }
        /// <summary>
        /// Megabytes to log. 
        /// </summary>
        public uint MBytes { get { return st.st_w_mbytes; } }
        /// <summary>
        /// Megabytes to log since checkpoint. 
        /// </summary>
        public uint MBytesSinceCheckpoint { get { return st.st_wc_mbytes; } }
        /// <summary>
        /// Log file magic number. 
        /// </summary>
        public uint MagicNumber { get { return st.st_magic; } }
        /// <summary>
        /// Max number of commits in a flush. 
        /// </summary>
        public uint MaxCommitsPerFlush { get { return st.st_maxcommitperflush; } }
        /// <summary>
        /// Min number of commits in a flush. 
        /// </summary>
        public uint MinCommitsPerFlush { get { return st.st_mincommitperflush; } }
        /// <summary>
        /// Overflow writes to the log. 
        /// </summary>
        public ulong OverflowWrites { get { return st.st_wcount_fill; } }
        /// <summary>
        /// Log file permissions mode. 
        /// </summary>
        public int PermissionsMode { get { return st.st_mode;}}
        /// <summary>
        /// Total I/O reads from the log. 
        /// </summary>
        public ulong Reads { get { return st.st_rcount; } }
        /// <summary>
        /// Records entered into the log. 
        /// </summary>
        public ulong Records { get { return st.st_record; } }
        /// <summary>
        /// Region lock granted without wait. 
        /// </summary>
        public ulong RegionLockNoWait { get { return st.st_region_nowait; } }
        /// <summary>
        /// Region lock granted after wait. 
        /// </summary>
        public ulong RegionLockWait { get { return st.st_region_wait; } }
        /// <summary>
        /// Region size. 
        /// </summary>
        public ulong RegionSize { get { return (ulong)st.st_regsize.ToInt64(); } }
        /// <summary>
        /// Total syncs to the log. 
        /// </summary>
        public ulong Syncs { get { return st.st_scount; } }
        /// <summary>
        /// Total I/O writes to the log. 
        /// </summary>
        public ulong Writes { get { return st.st_wcount; } }
        /// <summary>
        /// Log file version number. 
        /// </summary>
        public uint Version { get { return st.st_version; } }
        
    }
}