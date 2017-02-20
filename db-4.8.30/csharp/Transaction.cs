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
    /// A class representing Berkeley DB transactions
    /// </summary>
    /// <remarks>
    /// <para>
    /// Calling <see cref="Transaction.Abort"/>,
    /// <see cref="Transaction.Commit"/> or
    /// <see cref="Transaction.Discard"/> will release the resources held by
    /// the created object.
    /// </para>
    /// <para>
    /// Transactions may only span threads if they do so serially; that is,
    /// each transaction must be active in only a single thread of control
    /// at a time. This restriction holds for parents of nested transactions
    /// as well; no two children may be concurrently active in more than one
    /// thread of control at any one time.
    /// </para>
    /// <para>
    /// Cursors may not span transactions; that is, each cursor must be
    /// opened and closed within a single transaction.
    /// </para>
    /// <para>
    /// A parent transaction may not issue any Berkeley DB operations —
    /// except for <see cref="DatabaseEnvironment.BeginTransaction"/>, 
    /// <see cref="Transaction.Abort"/>and <see cref="Transaction.Commit"/>
    /// — while it has active child transactions (child transactions that
    /// have not yet been committed or aborted).
    /// </para>
    /// </remarks>
    public class Transaction {
        /// <summary>
        /// The size of the global transaction ID
        /// </summary>
        public static uint GlobalIdLength = DbConstants.DB_GID_SIZE;

        internal DB_TXN dbtxn;
        
        internal Transaction(DB_TXN txn) {
            dbtxn = txn;
        }

        private bool idCached = false;
        private uint _id;
        /// <summary>
        /// The unique transaction id associated with this transaction.
        /// </summary>
        public uint Id {
            get {
                if (!idCached) {
                    _id = dbtxn.id();
                    idCached = true;
                }
                return _id;
            }
        }

        /// <summary>
        /// Cause an abnormal termination of the transaction.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Before Abort returns, any locks held by the transaction will have
        /// been released.
        /// </para>
        /// <para>
        /// In the case of nested transactions, aborting a parent transaction
        /// causes all children (unresolved or not) of the parent transaction to
        /// be aborted.
        /// </para>
        /// <para>
        /// All cursors opened within the transaction must be closed before the
        /// transaction is aborted.
        /// </para>
        /// </remarks>
        public void Abort() {
            dbtxn.abort();
        }
        
        /// <summary>
        /// End the transaction.
        /// </summary>
        /// <overloads>
        /// <para>
        /// In the case of nested transactions, if the transaction is a parent
        /// transaction, committing the parent transaction causes all unresolved
        /// children of the parent to be committed. In the case of nested
        /// transactions, if the transaction is a child transaction, its locks
        /// are not released, but are acquired by its parent. Although the
        /// commit of the child transaction will succeed, the actual resolution
        /// of the child transaction is postponed until the parent transaction
        /// is committed or aborted; that is, if its parent transaction commits,
        /// it will be committed; and if its parent transaction aborts, it will
        /// be aborted.
        /// </para>
        /// <para>
        /// All cursors opened within the transaction must be closed before the
        /// transaction is committed.
        /// </para>
        /// </overloads>
        public void Commit() {
            dbtxn.commit(0);
        }
        /// <summary>
        /// End the transaction.
        /// </summary>
        /// <remarks>
        /// Synchronously flushing the log is the default for Berkeley DB
        /// environments unless
        /// <see cref="DatabaseEnvironmentConfig.TxnNoSync"/> was specified.
        /// Synchronous log flushing may also be set or unset for a single
        /// transaction using
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>. The
        /// value of <paramref name="syncLog"/> overrides both of those
        /// settings.
        /// </remarks>
        /// <param name="syncLog">If true, synchronously flush the log.</param>
        public void Commit(bool syncLog) {
            dbtxn.commit(
                syncLog ? DbConstants.DB_TXN_SYNC : DbConstants.DB_TXN_NOSYNC);
        }

        /// <summary>
        /// Free up all the per-process resources associated with the specified
        /// Transaction instance, neither committing nor aborting the
        /// transaction.
        /// </summary>
        /// <remarks>
        /// This call may be used only after calls to
        /// <see cref="DatabaseEnvironment.Recover"/> when there are multiple
        /// global transaction managers recovering transactions in a single
        /// Berkeley DB environment. Any transactions returned by 
        /// <see cref="DatabaseEnvironment.Recover"/> that are not handled by
        /// the current global transaction manager should be discarded using 
        /// Discard.
        /// </remarks>
        public void Discard() {
            dbtxn.discard(0);
        }

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
            get {
                string ret = "";
                dbtxn.get_name(ref ret);
                return ret;
            }
            set { dbtxn.set_name(value); }
        }

        /// <summary>
        /// Initiate the beginning of a two-phase commit.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In a distributed transaction environment, Berkeley DB can be used as
        /// a local transaction manager. In this case, the distributed
        /// transaction manager must send prepare messages to each local
        /// manager. The local manager must then call Prepare and await its
        /// successful return before responding to the distributed transaction
        /// manager. Only after the distributed transaction manager receives
        /// successful responses from all of its prepare messages should it
        /// issue any commit messages.
        /// </para>
        /// <para>
        /// In the case of nested transactions, preparing the parent causes all
        /// unresolved children of the parent transaction to be committed. Child
        /// transactions should never be explicitly prepared. Their fate will be
        /// resolved along with their parent's during global recovery.
        /// </para>
        /// </remarks>
        /// <param name="globalId">
        /// The global transaction ID by which this transaction will be known.
        /// This global transaction ID will be returned in calls to 
        /// <see cref="DatabaseEnvironment.Recover"/> telling the
        /// application which global transactions must be resolved.
        /// </param>
        public void Prepare(byte[] globalId) {
            if (globalId.Length != Transaction.GlobalIdLength)
                throw new ArgumentException(
                    "Global ID length must be " + Transaction.GlobalIdLength);
            dbtxn.prepare(globalId);
        }

        /// <summary>
        /// Set the timeout value for locks for this transaction. 
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
        /// whole. See <see cref="DatabaseEnvironment.LockTimeout"/> for more
        /// information. 
        /// </para>
        /// </remarks>
        /// <param name="timeout">
        /// An unsigned 32-bit number of microseconds, limiting the maximum
        /// timeout to roughly 71 minutes. A value of 0 disables timeouts for
        /// the transaction.
        /// </param>
        public void SetLockTimeout(uint timeout) {
            dbtxn.set_timeout(timeout, DbConstants.DB_SET_LOCK_TIMEOUT);
        }

        /// <summary>
        /// Set the timeout value for transactions for this transaction. 
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
        /// whole. See <see cref="DatabaseEnvironment.TxnTimeout"/> for more
        /// information. 
        /// </para>
        /// </remarks>
        /// <param name="timeout">
        /// An unsigned 32-bit number of microseconds, limiting the maximum
        /// timeout to roughly 71 minutes. A value of 0 disables timeouts for
        /// the transaction.
        /// </param>
        public void SetTxnTimeout(uint timeout) {
            dbtxn.set_timeout(timeout, DbConstants.DB_SET_TXN_TIMEOUT);
        }

        static internal DB_TXN getDB_TXN(Transaction txn) {
            return txn == null ? null : txn.dbtxn;
        }
    }
}
