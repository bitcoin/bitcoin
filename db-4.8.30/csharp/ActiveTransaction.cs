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
    /// The ActiveTransaction class describes a currently active transaction.
    /// </summary>
    public class ActiveTransaction {
        private DB_TXN_ACTIVE txn;
        private LSN _lsn;
        private LSN _read_lsn;
        private byte[] gid;
        private string txnname;

        internal ActiveTransaction(DB_TXN_ACTIVE active, byte[] GlobalID, string Name) {
            txn = active;
                        _lsn = new LSN(txn.lsn.file, txn.lsn.offset);
            _read_lsn = new LSN(txn.read_lsn.file, txn.read_lsn.offset);
            gid = GlobalID;
            txnname = Name;
        }

        /// <summary>
        /// The status of an active transaction.
        /// </summary>
        public enum TransactionStatus {
            /// <summary>
            /// The transaction has been aborted
            /// </summary>
            ABORTED = DB_TXN_ACTIVE_STATUS.TXN_ABORTED,
            /// <summary>
            /// The transaction has been committed
            /// </summary>
            COMMITTED = DB_TXN_ACTIVE_STATUS.TXN_COMMITTED,
            /// <summary>
            /// The transaction has been prepared
            /// </summary>
            PREPARED = DB_TXN_ACTIVE_STATUS.TXN_PREPARED,
            /// <summary>
            /// The transaction is running
            /// </summary>
            RUNNING = DB_TXN_ACTIVE_STATUS.TXN_RUNNING
        }

        /// <summary>
        /// The transaction ID of the transaction.
        /// </summary>
        public uint ID { get { return txn.txnid; } }
        /// <summary>
        /// The transaction ID of the parent transaction (or 0, if no parent). 
        /// </summary>
        public uint ParentID { get { return txn.parentid; } }
        /// <summary>
        /// The process ID of the originator of the transaction.
        /// </summary>
        public int ProcessID { get { return txn.pid; } }
        /// <summary>
        /// The thread of control ID of the originator of the transaction.
        /// </summary>
        public uint ThreadID { get { return txn.tid; } }
        /// <summary>
        /// The current log sequence number when the transaction was begun.
        /// </summary>
        public LSN Begun { get { return _lsn; } }
        /// <summary>
        /// The log sequence number of reads for snapshot transactions. 
        /// </summary>
        public LSN SnapshotReads { get { return _read_lsn; } }
        /// <summary>
        /// The number of MVCC buffer copies created by this transaction that
        /// remain in cache.
        /// </summary>
        public uint BufferCopiesInCache { get { return txn.mvcc_ref; } }
        /// <summary>
        /// Status of the transaction.
        /// </summary>
        public TransactionStatus Status { 
            get { return (TransactionStatus)txn.status; } 
        }
        /// <summary>
        /// If the transaction is a prepare transaction, the transaction's
        /// Global ID. Otherwise, the GlobalID contents are undefined. 
        /// </summary>
        public byte[] GlobalID { get { return gid; } }
        /// <summary>
        /// If a name was specified for the transaction, up to the first 50
        /// bytes of that name. 
        /// </summary>
        public string Name { get { return txnname; } }
    }
}
