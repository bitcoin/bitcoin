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
    /// A class representing a QueueDatabase. The Queue format supports fast
    /// access to fixed-length records accessed sequentially or by logical
    /// record number.
    /// </summary>
    public class QueueDatabase : Database {

        #region Constructors
        private QueueDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }
        internal QueueDatabase(BaseDatabase clone) : base(clone) { }
        
        private void Config(QueueDatabaseConfig cfg) {
            base.Config(cfg);
            /* 
             * Database.Config calls set_flags, but that doesn't get the Queue
             * specific flags.  No harm in calling it again.
             */
            db.set_flags(cfg.flags);
            
            db.set_re_len(cfg.Length);

            if (cfg.padIsSet)
                db.set_re_pad(cfg.PadByte);
            if (cfg.extentIsSet)
                db.set_q_extentsize(cfg.ExtentSize);
        }

        /// <summary>
        /// Instantiate a new QueueDatabase object and open the database
        /// represented by <paramref name="Filename"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database
        /// object that created it, in circumstances where doing so is safe.
        /// </para>
        /// <para>
        /// If <see cref="DatabaseConfig.AutoCommit"/> is set, the operation
        /// will be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database. In-memory databases never intended to be preserved on disk
        /// may be created by setting this parameter to null.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static QueueDatabase Open(
            string Filename, QueueDatabaseConfig cfg) {
            return Open(Filename, cfg, null);
        }
        /// <summary>
        /// Instantiate a new QueueDatabase object and open the database
        /// represented by <paramref name="Filename"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database
        /// object that created it, in circumstances where doing so is safe.
        /// </para>
        /// <para>
        /// If <paramref name="txn"/> is null, but
        /// <see cref="DatabaseConfig.AutoCommit"/> is set, the operation will
        /// be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open. Also note that the
        /// transaction must be committed before the object is closed.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database. In-memory databases never intended to be preserved on disk
        /// may be created by setting this parameter to null.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>A new, open database object</returns>
        public static QueueDatabase Open(
            string Filename, QueueDatabaseConfig cfg, Transaction txn) {
            QueueDatabase ret = new QueueDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, null, DBTYPE.DB_QUEUE, cfg.openFlags, 0);
            ret.isOpen = true;
            return ret;
        }

        #endregion Constructors

        #region Properties
        /// <summary>
        /// The size of the extents used to hold pages in a
        /// <see cref="QueueDatabase"/>, specified as a number of pages. 
        /// </summary>
        public uint ExtentSize {
            get {
                uint ret = 0;
                db.get_q_extentsize(ref ret);
                return ret;
            }
        }

        /// <summary>
        /// If true, modify the operation of <see cref="QueueDatabase.Consume"/>
        /// to return key/data pairs in order. That is, they will always return
        /// the key/data item from the head of the queue. 
        /// </summary>
        public bool InOrder {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_INORDER) != 0;
            }
        }

        /// <summary>
        /// The length of records in the database.
        /// </summary>
        public uint Length {
            get {
                uint ret = 0;
                db.get_re_len(ref ret);
                return ret;
            }
        }

        /// <summary>
        /// The padding character for short, fixed-length records.
        /// </summary>
        public int PadByte {
            get {
                int ret = 0;
                db.get_re_pad(ref ret);
                return ret;
            }
        }
        #endregion Properties

        #region Methods
        /// <summary>
        /// Append the data item to the end of the database.
        /// </summary>
        /// <param name="data">The data item to store in the database</param>
        /// <returns>The record number allocated to the record</returns>
        public uint Append(DatabaseEntry data) {
            return Append(data, null);
        }
        /// <summary>
        /// Append the data item to the end of the database.
        /// </summary>
        /// <remarks>
        /// There is a minor behavioral difference between
        /// <see cref="RecnoDatabase.Append"/> and
        /// <see cref="QueueDatabase.Append"/>. If a transaction enclosing an
        /// Append operation aborts, the record number may be reallocated in a
        /// subsequent <see cref="RecnoDatabase.Append"/> operation, but it will
        /// not be reallocated in a subsequent
        /// <see cref="QueueDatabase.Append"/> operation.
        /// </remarks>
        /// <param name="data">The data item to store in the database</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>The record number allocated to the record</returns>
        public uint Append(DatabaseEntry data, Transaction txn) {
            DatabaseEntry key = new DatabaseEntry();
            Put(key, data, txn, DbConstants.DB_APPEND);
            return BitConverter.ToUInt32(key.Data, 0);
        }

        /// <summary>
        /// Return the record number and data from the available record closest
        /// to the head of the queue, and delete the record.
        /// </summary>
        /// <param name="wait">
        /// If true and the Queue database is empty, the thread of control will
        /// wait until there is data in the queue before returning.
        /// </param>
        /// <exception cref="LockNotGrantedException">
        /// If lock or transaction timeouts have been specified, a
        /// <see cref="LockNotGrantedException"/> may be thrown. This failure,
        /// by itself, does not require the enclosing transaction be aborted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is the record number and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<uint, DatabaseEntry> Consume(bool wait) {
            return Consume(wait, null, null);
        }
        /// <summary>
        /// Return the record number and data from the available record closest
        /// to the head of the queue, and delete the record.
        /// </summary>
        /// <param name="wait">
        /// If true and the Queue database is empty, the thread of control will
        /// wait until there is data in the queue before returning.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <exception cref="LockNotGrantedException">
        /// If lock or transaction timeouts have been specified, a
        /// <see cref="LockNotGrantedException"/> may be thrown. This failure,
        /// by itself, does not require the enclosing transaction be aborted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is the record number and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<uint, DatabaseEntry> Consume(
            bool wait, Transaction txn) {
            return Consume(wait, txn, null);
        }
        /// <summary>
        /// Return the record number and data from the available record closest
        /// to the head of the queue, and delete the record.
        /// </summary>
        /// <param name="wait">
        /// If true and the Queue database is empty, the thread of control will
        /// wait until there is data in the queue before returning.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <exception cref="LockNotGrantedException">
        /// If lock or transaction timeouts have been specified, a
        /// <see cref="LockNotGrantedException"/> may be thrown. This failure,
        /// by itself, does not require the enclosing transaction be aborted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is the record number and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<uint, DatabaseEntry> Consume(
            bool wait, Transaction txn, LockingInfo info) {
            KeyValuePair<DatabaseEntry, DatabaseEntry> record;
            
            record = Get(null, null, txn, info,
                wait ? DbConstants.DB_CONSUME_WAIT : DbConstants.DB_CONSUME);
            return new KeyValuePair<uint, DatabaseEntry>(
                BitConverter.ToUInt32(record.Key.Data, 0), record.Value);

        }

        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public QueueStats FastStats() {
            return Stats(null, true, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public QueueStats FastStats(Transaction txn) {
            return Stats(txn, true, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <overloads>
        /// <para>
        /// Among other things, this method makes it possible for applications
        /// to request key and record counts without incurring the performance
        /// penalty of traversing the entire database. 
        /// </para>
        /// <para>
        /// The statistical information is described by the
        /// <see cref="BTreeStats"/>, <see cref="HashStats"/>,
        /// <see cref="QueueStats"/>, and <see cref="RecnoStats"/> classes. 
        /// </para>
        /// </overloads>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="isoDegree">
        /// The level of isolation for database reads.
        /// <see cref="Isolation.DEGREE_ONE"/> will be silently ignored for 
        /// databases which did not specify
        /// <see cref="DatabaseConfig.ReadUncommitted"/>.
        /// </param>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public QueueStats FastStats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, true, isoDegree);
        }

        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <returns>Database statistical information.</returns>
        public QueueStats Stats() {
            return Stats(null, false, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>Database statistical information.</returns>
        public QueueStats Stats(Transaction txn) {
            return Stats(txn, false, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <overloads>
        /// The statistical information is described by
        /// <see cref="BTreeStats"/>. 
        /// </overloads>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="isoDegree">
        /// The level of isolation for database reads.
        /// <see cref="Isolation.DEGREE_ONE"/> will be silently ignored for 
        /// databases which did not specify
        /// <see cref="DatabaseConfig.ReadUncommitted"/>.
        /// </param>
        /// <returns>Database statistical information.</returns>
        public QueueStats Stats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, false, isoDegree);
        }
        private QueueStats Stats(Transaction txn, bool fast, Isolation isoDegree) {
            uint flags = 0;
            flags |= fast ? DbConstants.DB_FAST_STAT : 0;
            switch (isoDegree) {
                case Isolation.DEGREE_ONE:
                    flags |= DbConstants.DB_READ_UNCOMMITTED;
                    break;
                case Isolation.DEGREE_TWO:
                    flags |= DbConstants.DB_READ_COMMITTED;
                    break;
            }
            QueueStatStruct st = db.stat_qam(Transaction.getDB_TXN(txn), flags);
            return new QueueStats(st);
        }
        #endregion Methods
    }
}
