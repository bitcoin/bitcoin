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
    /// A class that provides an arbitrary number of persistent objects that
    /// return an increasing or decreasing sequence of integers.
    /// </summary>
    public class Sequence : IDisposable {
        private DB_SEQUENCE seq;
        private bool isOpen;

        /// <summary>
        /// Instantiate a new Sequence object.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="cfg">Configuration parameters for the Sequence</param>
        public Sequence(SequenceConfig cfg) : this(cfg, null) { }
        /// <summary>
        /// Instantiate a new Sequence object.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="cfg">Configuration parameters for the Sequence</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        public Sequence(SequenceConfig cfg, Transaction txn) {
            seq = new DB_SEQUENCE(cfg.BackingDatabase.db, 0);
            if (cfg.initialValIsSet)
                seq.initial_value(cfg.InitialValue);
            seq.set_flags(cfg.flags);
            if (cfg.rangeIsSet)
                seq.set_range(cfg.Min, cfg.Max);
            if (cfg.cacheSzIsSet)
                seq.set_cachesize(cfg.CacheSize);
            seq.open(Transaction.getDB_TXN(txn),
                cfg.key, cfg.openFlags);
            isOpen = true;
        }

        /// <summary>
        /// Close the sequence handle. Any unused cached values are lost. 
        /// </summary>
        public void Close() {
            isOpen = false;
            seq.close(0);
        }

        /// <summary>
        /// Return the next available element in the sequence and change the
        /// sequence value by <paramref name="Delta"/>.
        /// </summary>
        /// <overloads>
        /// <para>
        /// If there are enough cached values in the sequence handle then they
        /// will be returned. Otherwise the next value will be fetched from the
        /// database and incremented (decremented) by enough to cover the delta
        /// and the next batch of cached values. 
        /// </para>
        /// <para>
        /// For maximum concurrency a non-zero cache size should be specified
        /// prior to opening the sequence handle and <paramref name="NoSync"/>
        /// should be specified for each Get method call.
        /// </para>
        /// <para>
        /// By default, sequence ranges do not wrap; to cause the sequence to
        /// wrap around the beginning or end of its range, set
        /// <paramref name="SequenceConfig.Wrap"/> to true.
        /// </para>
        /// <para>
        /// If <paramref name="P:BackingDatabase"/> was opened in a transaction,
        /// calling Get may result in changes to the sequence object; these
        /// changes will be automatically committed in a transaction internal to
        /// the Berkeley DB library. If the thread of control calling Get has an
        /// active transaction, which holds locks on the same database as the
        /// one in which the sequence object is stored, it is possible for a
        /// thread of control calling Get to self-deadlock because the active
        /// transaction's locks conflict with the internal transaction's locks.
        /// For this reason, it is often preferable for sequence objects to be
        /// stored in their own database. 
        /// </para>
        /// </overloads>
        /// <param name="Delta">
        /// The amount by which to increment the sequence value.  Must be
        /// greater than 0.
        /// </param>
        /// <returns>The next available element in the sequence.</returns>
        public Int64 Get(int Delta) {
            return Get(Delta, false, null);
        }
        /// <summary>
        /// Return the next available element in the sequence and change the
        /// sequence value by <paramref name="Delta"/>.
        /// </summary>
        /// <param name="Delta">
        /// The amount by which to increment the sequence value.  Must be
        /// greater than 0.
        /// </param>
        /// <param name="NoSync">
        /// If true, and if the operation is implicitly transaction protected,
        /// do not synchronously flush the log when the transaction commits.
        /// </param>
        /// <returns>The next available element in the sequence.</returns>
        public Int64 Get(int Delta, bool NoSync) {
            return Get(Delta, NoSync, null);
        }
        /// <summary>
        /// Return the next available element in the sequence and change the
        /// sequence value by <paramref name="Delta"/>.
        /// </summary>
        /// <param name="Delta">
        /// The amount by which to increment the sequence value.  Must be
        /// greater than 0.
        /// </param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.  
        /// Must be null if the sequence was opened with a non-zero cache size. 
        /// </param>
        /// <returns>The next available element in the sequence.</returns>
        public Int64 Get(int Delta, Transaction txn) {
            return Get(Delta, false, txn);
        }
        private Int64 Get(int Delta, bool NoSync, Transaction txn) {
            Int64 ret = DbConstants.DB_AUTO_COMMIT;
            uint flags = NoSync ? DbConstants.DB_TXN_NOSYNC : 0;
            seq.get(Transaction.getDB_TXN(txn), Delta, ref ret, flags);
            return ret;
        }

        /// <summary>
        /// The database used by the sequence.
        /// </summary>
        public Database BackingDatabase {
            get { return Database.fromDB(seq.get_db()); }
        }

        /// <summary>
        /// The key for the sequence.
        /// </summary>
        public DatabaseEntry Key {
            get {
                DatabaseEntry ret = new DatabaseEntry();
                seq.get_key(ret);
                return ret;
            }
        }

        /// <summary>
        /// Print diagnostic information.
        /// </summary>
        public void PrintStats() {
            PrintStats(false);
        }
        /// <summary>
        /// Print diagnostic information.
        /// </summary>
        /// <overloads>
        /// The diagnostic information is described by
        /// <see cref="SequenceStats"/>. 
        /// </overloads>
        /// <param name="ClearStats">
        /// If true, reset statistics after printing.
        /// </param>
        public void PrintStats(bool ClearStats) {
            uint flags = 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;
            seq.stat_print(flags);
        }

        /// <summary>
        /// Remove the sequence from the database.
        /// </summary>
        public void Remove() {
            Remove(false, null);
        }
        /// <summary>
        /// Remove the sequence from the database.
        /// </summary>
        /// <param name="NoSync">
        /// If true, and if the operation is implicitly transaction protected,
        /// do not synchronously flush the log when the transaction commits.
        /// </param>
        public void Remove(bool NoSync) {
            Remove(NoSync, null);
        }
        /// <summary>
        /// Remove the sequence from the database.
        /// </summary>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        public void Remove(Transaction txn) {
            Remove(false, txn);
        }
        private void Remove(bool NoSync, Transaction txn) {
            uint flags = NoSync ? DbConstants.DB_TXN_NOSYNC : 0;
            isOpen = false;
            seq.remove(Transaction.getDB_TXN(txn), flags);
        }

        /// <summary>
        /// Return statistical information for this sequence.
        /// </summary>
        /// <returns>Statistical information for this sequence.</returns>
        public SequenceStats Stats() {
            return Stats(false);
        }
        /// <summary>
        /// Return statistical information for this sequence.
        /// </summary>
        /// <overloads>
        /// <para>
        /// In the presence of multiple threads or processes accessing an active
        /// sequence, the information returned by DB_SEQUENCE->stat() may be
        /// out-of-date.
        /// </para>
        /// <para>
        /// The DB_SEQUENCE->stat() method cannot be transaction-protected. For
        /// this reason, it should be called in a thread of control that has no
        /// open cursors or active transactions. 
        /// </para>
        /// </overloads>
        /// <param name="clear">If true, reset statistics.</param>
        /// <returns>Statistical information for this sequence.</returns>
        public SequenceStats Stats(bool clear) {
            uint flags = 0;
            flags |= clear ? DbConstants.DB_STAT_CLEAR : 0;
            SequenceStatStruct st = seq.stat(flags);
            return new SequenceStats(st);
        }

        /// <summary>
        /// Release the resources held by this object, and close the sequence if
        /// it's still open.
        /// </summary>
        public void Dispose() {
            if (isOpen)
                seq.close(0);
            seq.Dispose();
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// The current cache size. 
        /// </summary>
        public int Cachesize {
            get {
                int ret = 0;
                seq.get_cachesize(ref ret);
                return ret;
            }
        }

        /// <summary>
        /// The minimum value in the sequence.
        /// </summary>
        public Int64 Min {
            get {
                Int64 ret = 0;
                Int64 tmp = 0;
                seq.get_range(ref ret, ref tmp);
                return ret;
            }
        }

        /// <summary>
        /// The maximum value in the sequence.
        /// </summary>
        public Int64 Max {
            get {
                Int64 ret = 0;
                Int64 tmp = 0;
                seq.get_range(ref tmp, ref ret);
                return ret;
            }
        }

        /// <summary>
        /// If true, the sequence should wrap around when it is incremented
        /// (decremented) past the specified maximum (minimum) value. 
        /// </summary>
        public bool Wrap {
            get {
                uint flags = 0;
                seq.get_flags(ref flags);
                return (flags & DbConstants.DB_SEQ_WRAP) != 0;
            }
        }

        /// <summary>
        /// If true, the sequence will be incremented. This is the default. 
        /// </summary>
        public bool Increment {
            get {
                uint flags = 0;
                seq.get_flags(ref flags);
                return (flags & DbConstants.DB_SEQ_INC) != 0;
            }
        }

        /// <summary>
        /// If true, the sequence will be decremented.
        /// </summary>
        public bool Decrement {
            get {
                uint flags = 0;
                seq.get_flags(ref flags);
                return (flags & DbConstants.DB_SEQ_DEC) != 0;
            }
        }
    }
}
