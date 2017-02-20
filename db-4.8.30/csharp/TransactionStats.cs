/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// Statistical information about the transaction subsystem
    /// </summary>
    public class TransactionStats {
        private TransactionStatStruct st;
        private LSN lastCkp;
        private List<ActiveTransaction> txns;

        internal TransactionStats(Internal.TxnStatStruct stats) {
            st = stats.st;
            lastCkp = new LSN(st.st_last_ckp.file, st.st_last_ckp.offset);
            txns = new List<ActiveTransaction>();
            for (int i = 0; i < st.st_nactive; i++)
                txns.Add(new ActiveTransaction(
                    stats.st_txnarray[i], stats.st_txngids[i], stats.st_txnnames[i]));
        }

        /// <summary>
        /// Number of aborted transactions 
        /// </summary>
        public ulong Aborted { get { return st.st_naborts; } }
        /// <summary>
        /// Number of active transactions 
        /// </summary>
        public uint Active { get { return st.st_nactive; } }
        /// <summary>
        /// Number of begun transactions 
        /// </summary>
        public ulong Begun { get { return st.st_nbegins; } }
        /// <summary>
        /// Number of committed transactions 
        /// </summary>
        public ulong Committed { get { return st.st_ncommits; } }
        /// <summary>
        /// LSN of the last checkpoint 
        /// </summary>
        public LSN LastCheckpoint { get { return lastCkp; } }
        /// <summary>
        /// Time of last checkpoint 
        /// </summary>
        public Int64 LastCheckpointTime { get { return st.st_time_ckp; } }
        /// <summary>
        /// Last transaction id given out 
        /// </summary>
        public uint LastID { get { return st.st_last_txnid; } }
        /// <summary>
        /// Maximum active transactions 
        /// </summary>
        public uint MaxActive { get { return st.st_maxnactive; } }
        /// <summary>
        /// Maximum snapshot transactions 
        /// </summary>
        public uint MaxSnapshot { get { return st.st_maxnsnapshot; } }
        /// <summary>
        /// Maximum txns possible 
        /// </summary>
        public uint MaxTransactions { get { return st.st_maxtxns; } }
        /// <summary>
        /// Region lock granted without wait. 
        /// </summary>
        public ulong RegionLockNoWait { get { return st.st_region_nowait; } }
        /// <summary>
        /// Region size. 
        /// </summary>
        public ulong RegionSize { get { return (ulong)st.st_regsize.ToInt64(); } }
        /// <summary>
        /// Region lock granted after wait. 
        /// </summary>
        public ulong RegionLockWait { get { return st.st_region_wait; } }
        /// <summary>
        /// Number of restored transactions after recovery.
        /// </summary>
        public uint Restored { get { return st.st_nrestores; } }
        /// <summary>
        /// Number of snapshot transactions 
        /// </summary>
        public uint Snapshot { get { return st.st_nsnapshot; } }
        /// <summary>
        /// List of active transactions
        /// </summary>
        public List<ActiveTransaction> Transactions { get { return txns; } }

    }
}