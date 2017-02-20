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
    /// A class representing a RecnoDatabase. The Recno format supports fixed-
    /// or variable-length records, accessed sequentially or by logical record
    /// number, and optionally backed by a flat text file. 
    /// </summary>
    public class RecnoDatabase : Database {
        private BDB_AppendRecnoDelegate doAppendRef;
        private AppendRecordDelegate appendHandler;

        #region Constructors
        private RecnoDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }
        internal RecnoDatabase(BaseDatabase clone) : base(clone) { }

        private void Config(RecnoDatabaseConfig cfg) {
            base.Config(cfg);
            /*
             * Database.Config calls set_flags, but that doesn't get the Recno
             * specific flags.  No harm in calling it again.
             */
            db.set_flags(cfg.flags);
            
            if (cfg.delimiterIsSet)
                RecordDelimiter = cfg.Delimiter;
            if (cfg.lengthIsSet)
                RecordLength = cfg.Length;
            if (cfg.padIsSet)
                RecordPad = cfg.PadByte;
            if (cfg.BackingFile != null)
                SourceFile = cfg.BackingFile;
        }

        /// <summary>
        /// Instantiate a new RecnoDatabase object and open the database
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
        public static RecnoDatabase Open(
            string Filename, RecnoDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new RecnoDatabase object and open the database
        /// represented by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If both <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/> are null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database 
        /// object that created it, in circumstances where doing so is safe. If
        /// <paramref name="Filename"/> is null and
        /// <paramref name="DatabaseName"/> is non-null, the database can be
        /// opened by other threads of control and will be replicated to client
        /// sites in any replication group.
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
        /// <param name="DatabaseName">
        /// This parameter allows applications to have multiple databases in a
        /// single file. Although no DatabaseName needs to be specified, it is
        /// an error to attempt to open a second database in a file that was not
        /// initially created using a database name.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static RecnoDatabase Open(
            string Filename, string DatabaseName, RecnoDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new RecnoDatabase object and open the database
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
        public static RecnoDatabase Open(
            string Filename, RecnoDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new RecnoDatabase object and open the database
        /// represented by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If both <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/> are null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database 
        /// object that created it, in circumstances where doing so is safe. If
        /// <paramref name="Filename"/> is null and
        /// <paramref name="DatabaseName"/> is non-null, the database can be
        /// opened by other threads of control and will be replicated to client
        /// sites in any replication group.
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
        /// <param name="DatabaseName">
        /// This parameter allows applications to have multiple databases in a
        /// single file. Although no DatabaseName needs to be specified, it is
        /// an error to attempt to open a second database in a file that was not
        /// initially created using a database name.
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
        public static RecnoDatabase Open(string Filename,
            string DatabaseName, RecnoDatabaseConfig cfg, Transaction txn) {
            RecnoDatabase ret = new RecnoDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, DatabaseName, DBTYPE.DB_RECNO, cfg.openFlags, 0);
            ret.isOpen = true;
            return ret;
        }
        #endregion Constructors

        #region Callbacks
        private static void doAppend(IntPtr dbp, IntPtr dbtp1, uint recno) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            RecnoDatabase rdb = (RecnoDatabase)(db.api_internal);

            rdb.AppendCallback(DatabaseEntry.fromDBT(dbt1), recno);
        }
        #endregion Callbacks

        #region Properties
        /// <summary>
        /// A function to call after the record number has been selected but
        /// before the data has been stored into the database.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When using <see cref="QueueDatabase.Append"/>, it may be useful to
        /// modify the stored data based on the generated key. If a delegate is
        /// specified, it will be called after the record number has been
        /// selected, but before the data has been stored.
        /// </para>
        /// </remarks>
        public AppendRecordDelegate AppendCallback {
            get { return appendHandler; }
            set {
                if (value == null)
                    db.set_append_recno(null);
                else if (appendHandler == null) {
                    if (doAppendRef == null)
                        doAppendRef = new BDB_AppendRecnoDelegate(doAppend);
                    db.set_append_recno(doAppendRef);
                }
                appendHandler = value;
            }
        }

        /// <summary>
        /// The delimiting byte used to mark the end of a record in
        /// <see cref="SourceFile"/>.
        /// </summary>
        public int RecordDelimiter {
            get {
                int ret = 0;
                db.get_re_delim(ref ret);
                return ret;
            }
            private set {
                db.set_re_delim(value);
            }
        }

        /// <summary>
        /// If using fixed-length, not byte-delimited records, the length of the
        /// records. 
        /// </summary>
        public uint RecordLength {
            get {
                uint ret = 0;
                db.get_re_len(ref ret);
                return ret;
            }
            private set {
                db.set_re_len(value);
            }
        }

        /// <summary>
        /// The padding character for short, fixed-length records.
        /// </summary>
        public int RecordPad {
            get {
                int ret = 0;
                db.get_re_pad(ref ret);
                return ret;
            }
            private set {
                db.set_re_pad(value);
            }
        }

        /// <summary>
        /// If true, the logical record numbers are mutable, and change as
        /// records are added to and deleted from the database.
        /// </summary>
        public bool Renumber {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_RENUMBER) != 0;
            }
        }

        /// <summary>
        /// If true, any <see cref="SourceFile"/> file will be read in its
        /// entirety when <see cref="Open"/> is called. If false,
        /// <see cref="SourceFile"/> may be read lazily. 
        /// </summary>
        public bool Snapshot {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_SNAPSHOT) != 0;
            }
        }

        /// <summary>
        /// The underlying source file for the Recno access method.
        /// </summary>
        public string SourceFile {
            get {
                string ret = "";
                db.get_re_source(ref ret);
                return ret;
            }
            private set {
                db.set_re_source(value);
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
        /// Compact the database, and optionally return unused database pages to
        /// the underlying filesystem. 
        /// </summary>
        /// <remarks>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected using multiple
        /// transactions. These transactions will be periodically committed to
        /// avoid locking large sections of the tree. Any deadlocks encountered
        /// cause the compaction operation to be retried from the point of the
        /// last transaction commit.
        /// </remarks>
        /// <param name="cdata">Compact configuration parameters</param>
        /// <returns>Compact operation statistics</returns>
        public CompactData Compact(CompactConfig cdata) {
            return Compact(cdata, null);
        }
        /// <summary>
        /// Compact the database, and optionally return unused database pages to
        /// the underlying filesystem. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="txn"/> is non-null, then the operation is
        /// performed using that transaction. In this event, large sections of
        /// the tree may be locked during the course of the transaction.
        /// </para>
        /// <para>
        /// If <paramref name="txn"/> is null, but the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected using multiple transactions. These transactions will be
        /// periodically committed to avoid locking large sections of the tree.
        /// Any deadlocks encountered cause the compaction operation to be
        /// retried from the point of the last transaction commit.
        /// </para>
        /// </remarks>
        /// <param name="cdata">Compact configuration parameters</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>Compact operation statistics</returns>
        public CompactData Compact(CompactConfig cdata, Transaction txn) {
            DatabaseEntry end = null;
            if (cdata.returnEnd)
                end = new DatabaseEntry();

            db.compact(Transaction.getDB_TXN(txn),
                cdata.start,
                cdata.stop,
                CompactConfig.getDB_COMPACT(cdata),
                cdata.flags, end);
            return new CompactData(CompactConfig.getDB_COMPACT(cdata), end);
        }

        /// <summary>
        /// Create a database cursor.
        /// </summary>
        /// <returns>A newly created cursor</returns>
        public new RecnoCursor Cursor() {
            return Cursor(new CursorConfig(), null);
        }
        /// <summary>
        /// Create a database cursor with the given configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new RecnoCursor Cursor(CursorConfig cfg) {
            return Cursor(cfg, null);
        }
        /// <summary>
        /// Create a transactionally protected database cursor.
        /// </summary>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new RecnoCursor Cursor(Transaction txn) {
            return Cursor(new CursorConfig(), txn);
        }
        /// <summary>
        /// Create a transactionally protected database cursor with the given
        /// configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new RecnoCursor Cursor(CursorConfig cfg, Transaction txn) {
            return new RecnoCursor(
                db.cursor(Transaction.getDB_TXN(txn), cfg.flags), Pagesize);
        }

        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public RecnoStats FastStats() {
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
        public RecnoStats FastStats(Transaction txn) {
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
        public RecnoStats FastStats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, true, isoDegree);
        }

        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <returns>Database statistical information.</returns>
        public RecnoStats Stats() {
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
        public RecnoStats Stats(Transaction txn) {
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
        public RecnoStats Stats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, false, isoDegree);
        }
        private RecnoStats Stats(
            Transaction txn, bool fast, Isolation isoDegree) {
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
            BTreeStatStruct st = db.stat_bt(Transaction.getDB_TXN(txn), flags);
            return new RecnoStats(st);
        }

        /// <summary>
        /// Return pages to the filesystem that are already free and at the end
        /// of the file.
        /// </summary>
        /// <returns>
        /// The number of database pages returned to the filesystem
        /// </returns>
        public uint TruncateUnusedPages() {
            return TruncateUnusedPages(null);
        }
        /// <summary>
        /// Return pages to the filesystem that are already free and at the end
        /// of the file.
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
        /// The number of database pages returned to the filesystem
        /// </returns>
        public uint TruncateUnusedPages(Transaction txn) {
            DB_COMPACT cdata = new DB_COMPACT();
            db.compact(Transaction.getDB_TXN(txn),
                null, null, cdata, DbConstants.DB_FREELIST_ONLY, null);
            return cdata.compact_pages_truncated;
        }
        #endregion Methods
    }
}