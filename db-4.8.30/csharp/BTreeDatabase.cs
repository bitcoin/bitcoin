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
    /// A class representing a BTreeDatabase.  The Btree format is a
    /// representation of a sorted, balanced tree structure. 
    /// </summary>
    public class BTreeDatabase : Database {
        private BTreeCompressDelegate compressHandler;
        private BTreeDecompressDelegate decompressHandler;
        private EntryComparisonDelegate compareHandler, prefixCompareHandler;
        private EntryComparisonDelegate dupCompareHandler;
        private BDB_CompareDelegate doCompareRef;
        private BDB_CompareDelegate doPrefixCompareRef;
        private BDB_CompareDelegate doDupCompareRef;
        private BDB_CompressDelegate doCompressRef;
        private BDB_DecompressDelegate doDecompressRef;

        #region Constructors
        private BTreeDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }
        internal BTreeDatabase(BaseDatabase clone) : base(clone) { }

        private void Config(BTreeDatabaseConfig cfg) {
            base.Config(cfg);
            /* 
             * Database.Config calls set_flags, but that doesn't get the BTree
             * specific flags.  No harm in calling it again.
             */
            db.set_flags(cfg.flags);
            
            if (cfg.BTreeCompare != null)
                Compare = cfg.BTreeCompare;

            if (cfg.BTreePrefixCompare != null)
                PrefixCompare = cfg.BTreePrefixCompare;

            // The duplicate comparison function cannot change.
            if (cfg.DuplicateCompare != null)
                DupCompare = cfg.DuplicateCompare;

            if (cfg.minkeysIsSet)
                db.set_bt_minkey(cfg.MinKeysPerPage);
                
            if (cfg.compressionIsSet) {
                Compress = cfg.Compress;
                Decompress = cfg.Decompress;
                if (Compress == null)
                    doCompressRef = null;
                else
                    doCompressRef = new BDB_CompressDelegate(doCompress);
                if (Decompress == null)
                    doDecompressRef = null;
                else
                    doDecompressRef = new BDB_DecompressDelegate(doDecompress);
                db.set_bt_compress(doCompressRef, doDecompressRef);
            }
        }

        /// <summary>
        /// Instantiate a new BTreeDatabase object and open the database
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
        public static BTreeDatabase Open(
            string Filename, BTreeDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new BTreeDatabase object and open the database
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
        public static BTreeDatabase Open(
            string Filename, string DatabaseName, BTreeDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new BTreeDatabase object and open the database
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
        public static BTreeDatabase Open(
            string Filename, BTreeDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new BTreeDatabase object and open the database
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
        public static BTreeDatabase Open(string Filename,
            string DatabaseName, BTreeDatabaseConfig cfg, Transaction txn) {
            BTreeDatabase ret = new BTreeDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, DatabaseName, DBTYPE.DB_BTREE, cfg.openFlags, 0);
            ret.isOpen = true;
            return ret;
        }
        #endregion Constructors

        #region Callbacks
        private static int doCompare(IntPtr dbp, IntPtr dbtp1, IntPtr dbtp2) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            DBT dbt2 = new DBT(dbtp2, false);
            BTreeDatabase btdb = (BTreeDatabase)(db.api_internal);
            return btdb.Compare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static int doCompress(IntPtr dbp, IntPtr prevKeyp,
            IntPtr prevDatap, IntPtr keyp, IntPtr datap, IntPtr destp) {
            DB db = new DB(dbp, false);
            DatabaseEntry prevKey =
                DatabaseEntry.fromDBT(new DBT(prevKeyp, false));
            DatabaseEntry prevData =
                DatabaseEntry.fromDBT(new DBT(prevDatap, false));
            DatabaseEntry key = DatabaseEntry.fromDBT(new DBT(keyp, false));
            DatabaseEntry data = DatabaseEntry.fromDBT(new DBT(datap, false));
            DBT dest = new DBT(destp, false);
            BTreeDatabase btdb = (BTreeDatabase)(db.api_internal);
            byte[] arr = new byte[(int)dest.ulen];
            int len;
            try {
                if (btdb.Compress(prevKey, prevData, key, data, ref arr, out len)) {
                    Marshal.Copy(arr, 0, dest.dataPtr, len);
                    dest.size = (uint)len;
                    return 0;
                } else {
                    return DbConstants.DB_BUFFER_SMALL;
                }
            } catch (Exception) {
                return -1;
            }
        }
        private static int doDecompress(IntPtr dbp, IntPtr prevKeyp,
            IntPtr prevDatap, IntPtr cmpp, IntPtr destKeyp, IntPtr destDatap) {
            DB db = new DB(dbp, false);
            DatabaseEntry prevKey =
                DatabaseEntry.fromDBT(new DBT(prevKeyp, false));
            DatabaseEntry prevData =
                DatabaseEntry.fromDBT(new DBT(prevDatap, false));
            DBT compressed = new DBT(cmpp, false);
            DBT destKey = new DBT(destKeyp, false);
            DBT destData = new DBT(destDatap, false);
            BTreeDatabase btdb = (BTreeDatabase)(db.api_internal);
            uint size;
            try {
                KeyValuePair<DatabaseEntry, DatabaseEntry> kvp = btdb.Decompress(prevKey, prevData, compressed.data, out size);
                int keylen = kvp.Key.Data.Length;
                int datalen = kvp.Value.Data.Length;
                destKey.size = (uint)keylen;
                destData.size = (uint)datalen;
                if (keylen > destKey.ulen ||
                    datalen > destData.ulen)
                    return DbConstants.DB_BUFFER_SMALL;
                Marshal.Copy(kvp.Key.Data, 0, destKey.dataPtr, keylen);
                Marshal.Copy(kvp.Value.Data, 0, destData.dataPtr, datalen);
                compressed.size = size;
                return 0;
            } catch (Exception) {
                return -1;
            }
        }
        private static int doDupCompare(
            IntPtr dbp, IntPtr dbt1p, IntPtr dbt2p) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbt1p, false);
            DBT dbt2 = new DBT(dbt2p, false);

            BTreeDatabase btdb = (BTreeDatabase)(db.api_internal);
            return btdb.DupCompare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static int doPrefixCompare(
            IntPtr dbp, IntPtr dbtp1, IntPtr dbtp2) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            DBT dbt2 = new DBT(dbtp2, false);
            BTreeDatabase btdb = (BTreeDatabase)(db.api_internal);
            return btdb.PrefixCompare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        #endregion Callbacks

        #region Properties
        // Sorted alpha by property name
        /// <summary>
        /// The Btree key comparison function. The comparison function is called
        /// whenever it is necessary to compare a key specified by the
        /// application with a key currently stored in the tree. 
        /// </summary>
        public EntryComparisonDelegate Compare {
            get { return compareHandler; }
            private set {
                if (value == null)
                    db.set_bt_compare(null);
                else if (compareHandler == null) {
                    if (doCompareRef == null)
                        doCompareRef = new BDB_CompareDelegate(doCompare);
                    db.set_bt_compare(doCompareRef);
                }
                compareHandler = value;
            }
        }

        /// <summary>
        /// The compression function used to store key/data pairs in the
        /// database.
        /// </summary>
        public BTreeCompressDelegate Compress {
            get { return compressHandler; }
            private set { compressHandler = value; }
        }

        /// <summary>
        /// The decompression function used to retrieve key/data pairs from the
        /// database.
        /// </summary>
        public BTreeDecompressDelegate Decompress {
            get { return decompressHandler; }
            private set { decompressHandler = value; }
        }

        /// <summary>
        /// The duplicate data item comparison function.
        /// </summary>
        public EntryComparisonDelegate DupCompare {
            get { return dupCompareHandler; }
            private set {
                /* Cannot be called after open. */
                if (value == null)
                    db.set_dup_compare(null);
                else if (dupCompareHandler == null) {
                    if (doDupCompareRef == null)
                        doDupCompareRef = new BDB_CompareDelegate(doDupCompare);
                    db.set_dup_compare(doDupCompareRef);
                }
                dupCompareHandler = value;
            }
        }
        /// <summary>
        /// Whether the insertion of duplicate data items in the database is
        /// permitted, and whether duplicates items are sorted.
        /// </summary>
        public DuplicatesPolicy Duplicates {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                if ((flags & DbConstants.DB_DUPSORT) != 0)
                    return DuplicatesPolicy.SORTED;
                else if ((flags & DbConstants.DB_DUP) != 0)
                    return DuplicatesPolicy.UNSORTED;
                else
                    return DuplicatesPolicy.NONE;
            }
        }

        /// <summary>
        /// The minimum number of key/data pairs intended to be stored on any
        /// single Btree leaf page.
        /// </summary>
        public uint MinKeysPerPage {
            get {
                uint ret = 0;
                db.get_bt_minkey(ref ret);
                return ret;
            }
        }

        /// <summary>
        /// The Btree prefix function. The prefix function is used to determine
        /// the amount by which keys stored on the Btree internal pages can be
        /// safely truncated without losing their uniqueness.
        /// </summary>
        public EntryComparisonDelegate PrefixCompare {
            get { return prefixCompareHandler; }
            private set {
                if (value == null)
                    db.set_bt_prefix(null);
                else if (prefixCompareHandler == null) {
                    if (doPrefixCompareRef == null)
                        doPrefixCompareRef =
            new BDB_CompareDelegate(doPrefixCompare);
                    db.set_bt_prefix(doPrefixCompareRef);
                }
                prefixCompareHandler = value;
            }
        }

        /// <summary>
        /// If true, this object supports retrieval from the Btree using record
        /// numbers.
        /// </summary>
        public bool RecordNumbers {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_RECNUM) != 0;
            }
        }

        /// <summary>
        /// If false, empty pages will not be coalesced into higher-level pages.
        /// </summary>
        public bool ReverseSplit {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_REVSPLITOFF) == 0;
            }
        }

        #endregion Properties

        #region Methods
        // Sorted alpha by method name

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

            db.compact(Transaction.getDB_TXN(txn), cdata.start, cdata.stop,
                CompactConfig.getDB_COMPACT(cdata), cdata.flags, end);
            return new CompactData(CompactConfig.getDB_COMPACT(cdata), end);
        }

        /// <summary>
        /// Create a database cursor.
        /// </summary>
        /// <returns>A newly created cursor</returns>
        public new BTreeCursor Cursor() {
            return Cursor(new CursorConfig(), null);
        }
        /// <summary>
        /// Create a database cursor with the given configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new BTreeCursor Cursor(CursorConfig cfg) {
            return Cursor(cfg, null);
        }
        /// <summary>
        /// Create a transactionally protected database cursor.
        /// </summary>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new BTreeCursor Cursor(Transaction txn) {
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
        public new BTreeCursor Cursor(CursorConfig cfg, Transaction txn) {
            return new BTreeCursor(
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
        public BTreeStats FastStats() {
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
        public BTreeStats FastStats(Transaction txn) {
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
        public BTreeStats FastStats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, true, isoDegree);
        }

        /// <summary>
        /// Retrieve a specific numbered key/data pair from the database.
        /// </summary>
        /// <param name="recno">
        /// The record number of the record to be retrieved.
        /// </param>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Get(uint recno) {
            return Get(recno, null, null);
        }
        /// <summary>
        /// Retrieve a specific numbered key/data pair from the database.
        /// </summary>
        /// <param name="recno">
        /// The record number of the record to be retrieved.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Get(
            uint recno, Transaction txn) {
            return Get(recno, txn, null);
        }
        /// <summary>
        /// Retrieve a specific numbered key/data pair from the database.
        /// </summary>
        /// <param name="recno">
        /// The record number of the record to be retrieved.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Get(
            uint recno, Transaction txn, LockingInfo info) {
            DatabaseEntry key = new DatabaseEntry();
            key.Data = BitConverter.GetBytes(recno);

            return Get(key, null, txn, info, DbConstants.DB_SET_RECNO);
        }

        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            uint recno) {
            return GetMultiple(recno, (int)Pagesize, null, null);
        }
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            uint recno, int BufferSize) {
            return GetMultiple(recno, BufferSize, null, null);
        }
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            uint recno, int BufferSize, Transaction txn) {
            return GetMultiple(recno, BufferSize, txn, null);
        }
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            uint recno, int BufferSize, Transaction txn, LockingInfo info) {
            KeyValuePair<DatabaseEntry, DatabaseEntry> kvp;

            DatabaseEntry key = new DatabaseEntry();
            key.Data = BitConverter.GetBytes(recno);

            DatabaseEntry data = new DatabaseEntry();

            for (; ; ) {
                data.UserData = new byte[BufferSize];
                try {
                    kvp = Get(key, data, txn, info,
                        DbConstants.DB_MULTIPLE | DbConstants.DB_SET_RECNO);
                    break;
                } catch (MemoryException) {
                    int sz = (int)data.size;
                    if (sz > BufferSize)
                        BufferSize = sz;
                    else
                        BufferSize *= 2;
                }
            }
            MultipleDatabaseEntry dbe = new MultipleDatabaseEntry(kvp.Value);
            return new KeyValuePair<DatabaseEntry, MultipleDatabaseEntry>(
                kvp.Key, dbe);
        }

        /// <summary>
        /// Return an estimate of the proportion of keys that are less than,
        /// equal to, and greater than the specified key.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <returns>
        /// An estimate of the proportion of keys that are less than, equal to,
        /// and greater than the specified key.
        /// </returns>
        public KeyRange KeyRange(DatabaseEntry key) {
            return KeyRange(key, null);
        }
        /// <summary>
        /// Return an estimate of the proportion of keys that are less than,
        /// equal to, and greater than the specified key.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// An estimate of the proportion of keys that are less than, equal to,
        /// and greater than the specified key.
        /// </returns>
        public KeyRange KeyRange(DatabaseEntry key, Transaction txn) {
            DB_KEY_RANGE range = new DB_KEY_RANGE();
            db.key_range(Transaction.getDB_TXN(txn), key, range, 0);
            return new KeyRange(range);
        }

        /// <summary>
        /// Store the key/data pair in the database only if it does not already
        /// appear in the database. 
        /// </summary>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        public void PutNoDuplicate(DatabaseEntry key, DatabaseEntry data) {
            PutNoDuplicate(key, data, null);
        }
        /// <summary>
        /// Store the key/data pair in the database only if it does not already
        /// appear in the database. 
        /// </summary>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        public void PutNoDuplicate(
            DatabaseEntry key, DatabaseEntry data, Transaction txn) {
            Put(key, data, txn, DbConstants.DB_NODUPDATA);
        }

        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <returns>Database statistical information.</returns>
        public BTreeStats Stats() {
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
        public BTreeStats Stats(Transaction txn) {
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
        public BTreeStats Stats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, false, isoDegree);
        }
        private BTreeStats Stats(
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
            return new BTreeStats(st);
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
