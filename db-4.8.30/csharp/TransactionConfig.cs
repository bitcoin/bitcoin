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
    /// A class representing configuration parameters for a
    /// <see cref="Transaction"/>.
    /// </summary>
    public class TransactionConfig {
        /// <summary>
        /// Specifies the log flushing behavior on transaction commit
        /// </summary>
        public enum LogFlush {
            /// <summary>
            /// Use Berkeley DB's default behavior of syncing the log on commit.
            /// </summary>
            DEFAULT,
            /// <summary>
            /// Berkeley DB will not write or synchronously flush the log on
            /// transaction commit or prepare.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This means the transaction will exhibit the ACI (atomicity,
            /// consistency, and isolation) properties, but not D (durability);
            /// that is, database integrity will be maintained but it is
            /// possible that this transaction may be undone during recovery. 
            /// </para>
            /// </remarks>
            NOSYNC,
            /// <summary>
            /// Berkeley DB will write, but will not synchronously flush, the
            /// log on transaction commit or prepare. 
            /// </summary>
            /// <remarks>
            /// <para>
            /// This means that transactions exhibit the ACI (atomicity,
            /// consistency, and isolation) properties, but not D (durability);
            /// that is, database integrity will be maintained, but if the
            /// system fails, it is possible some number of the most recently
            /// committed transactions may be undone during recovery. The number
            /// of transactions at risk is governed by how often the system
            /// flushes dirty buffers to disk and how often the log is
            /// checkpointed.
            /// </para>
            /// <para>
            /// For consistent behavior across the environment, all 
            /// <see cref="DatabaseEnvironment"/> objects opened in the
            /// environment must either set WRITE_NOSYNC, or the
            /// DB_TXN_WRITE_NOSYNC flag should be specified in the DB_CONFIG
            /// configuration file.
            /// </para>
            /// </remarks>
            WRITE_NOSYNC,
            /// <summary>
            /// Berkeley DB will synchronously flush the log on transaction
            /// commit or prepare. 
            /// </summary>
            /// <remarks>
            /// This means the transaction will exhibit all of the ACID
            /// (atomicity, consistency, isolation, and durability) properties.
            /// </remarks>
            SYNC
        };
        /// <summary>
        /// The degree of isolation for this transaction
        /// </summary>
        public Isolation IsolationDegree;
        /// <summary>
        /// If true and a lock is unavailable for any Berkeley DB operation
        /// performed in the context of a transaction, cause the operation to
        /// throw a <see cref="DeadlockException"/>
        /// (or <see cref="LockNotGrantedException"/> if configured with
        /// <see cref="DatabaseEnvironmentConfig.TimeNotGranted"/>).
        /// </summary>
        /// <remarks>
        /// <para>
        /// This setting overrides the behavior specified by
        /// <see cref="DatabaseEnvironmentConfig.TxnNoWait"/>.
        /// </para>
        /// </remarks>    
        public bool NoWait;
        /// <summary>
        /// If true, this transaction will execute with snapshot isolation.
        /// </summary>
        /// <remarks>
        /// <para>
        /// For databases with <see cref="DatabaseConfig.UseMVCC"/> set, data
        /// values will be read as they are when the transaction begins, without
        /// taking read locks. Silently ignored for operations on databases with
        /// <see cref="DatabaseConfig.UseMVCC"/> not set on the underlying
        /// database (read locks are acquired).
        /// </para>
        /// <para>
        /// A <see cref="DeadlockException"/> will be thrown from update
        /// operations if a snapshot transaction attempts to update data which
        /// was modified after the snapshot transaction read it.
        /// </para>
        /// </remarks>
        public bool Snapshot;
        /// <summary>
        /// Log sync behavior on transaction commit or prepare.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This setting overrides the behavior specified by
        /// <see cref="DatabaseEnvironmentConfig.TxnNoSync"/> and
        /// <see cref="DatabaseEnvironmentConfig.TxnWriteNoSync"/>.
        /// </para>
        /// </remarks>
        public LogFlush SyncAction;

        internal uint flags {
            get {
                uint ret = 0;

                switch (IsolationDegree) {
                    case (Isolation.DEGREE_ONE):
                        ret |= DbConstants.DB_READ_UNCOMMITTED;
                        break;
                    case (Isolation.DEGREE_TWO):
                        ret |= DbConstants.DB_READ_COMMITTED;
                        break;
                }

                ret |= NoWait ? DbConstants.DB_TXN_NOWAIT : 0;
                ret |= Snapshot ? DbConstants.DB_TXN_SNAPSHOT : 0;

                switch (SyncAction) {
                    case (LogFlush.NOSYNC):
                        ret |= DbConstants.DB_TXN_NOSYNC;
                        break;
                    case (LogFlush.SYNC):
                        ret |= DbConstants.DB_TXN_SYNC;
                        break;
                    case (LogFlush.WRITE_NOSYNC):
                        ret |= DbConstants.DB_TXN_WRITE_NOSYNC;
                        break;
                }

                return ret;
            }
        }

        private uint _lckTimeout;
        internal bool lockTimeoutIsSet;
        /// <summary>
        /// The timeout value for locks for the transaction. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Timeouts are checked whenever a thread of control blocks on a lock
        /// or when deadlock detection is performed. This timeout is for any
        /// single lock request. As timeouts are only checked when the lock
        /// request first blocks or when deadlock detection is performed, the
        /// accuracy of the timeout depends on how often deadlock detection is
        /// performed.
        /// </para>
        /// <para>
        /// Timeout values may be specified for the database environment as a
        /// whole. See <see cref="DatabaseEnvironmentConfig.LockTimeout"/> for
        /// more information.
        /// </para>
        /// </remarks>
        public uint LockTimeout {
            get { return _lckTimeout; }
            set {
                lockTimeoutIsSet = true;
                _lckTimeout = value;
            }
        }

        private string _name;
        internal bool nameIsSet;
        /// <summary>
        /// The transaction's name. The name is returned by
        /// <see cref="DatabaseEnvironment.TransactionSystemStats"/>
        /// and displayed by
        /// <see cref="DatabaseEnvironment.PrintTransactionSystemStats"/>.
        /// </summary>
        /// <remarks>
        /// If the database environment has been configured for logging and the
        /// Berkeley DB library was built in Debug mode (or with DIAGNOSTIC
        /// defined), a debugging log record is written including the
        /// transaction ID and the name.
        /// </remarks>
        public string Name {
            get { return _name; }
            set {
                nameIsSet = (value != null);
                _name = value;
            }
        }

        private uint _txnTimeout;
        internal bool txnTimeoutIsSet;
        /// <summary>
        /// The timeout value for locks for the transaction. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Timeouts are checked whenever a thread of control blocks on a lock
        /// or when deadlock detection is performed. This timeout is for the
        /// life of the transaction. As timeouts are only checked when the lock
        /// request first blocks or when deadlock detection is performed, the
        /// accuracy of the timeout depends on how often deadlock detection is
        /// performed.
        /// </para>
        /// <para>
        /// Timeout values may be specified for the database environment as a
        /// whole. See <see cref="DatabaseEnvironmentConfig.TxnTimeout"/> for
        /// more information.
        /// </para>
        /// </remarks>
        public uint TxnTimeout {
            get { return _txnTimeout; }
            set {
                txnTimeoutIsSet = true;
                _txnTimeout = value;
            }
        }

        /// <summary>
        /// Instantiate a new TransactionConfig object
        /// </summary>
        public TransactionConfig() {
            IsolationDegree = Isolation.DEGREE_THREE;
            SyncAction = LogFlush.DEFAULT;
        }
    }
}