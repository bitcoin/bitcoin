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
    /// A class representing a SecondaryBTreeDatabase.  The Btree format is a
    /// representation of a sorted, balanced tree structure. 
    /// </summary>
    public class SecondaryBTreeDatabase : SecondaryDatabase {
        private EntryComparisonDelegate compareHandler;
        private EntryComparisonDelegate prefixCompareHandler;
        private EntryComparisonDelegate dupCompareHandler;
        private BDB_CompareDelegate doCompareRef;
        private BDB_CompareDelegate doPrefixCompareRef;
        private BDB_CompareDelegate doDupCompareRef;
        
        #region Constructors
        internal SecondaryBTreeDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }

        private void Config(SecondaryBTreeDatabaseConfig cfg) {
            base.Config((SecondaryDatabaseConfig)cfg);
            db.set_flags(cfg.flags);
            if (cfg.Compare != null)
                Compare = cfg.Compare;
            if (cfg.PrefixCompare != null)
                PrefixCompare = cfg.PrefixCompare;
            if (cfg.DuplicateCompare != null)
                DupCompare = cfg.DuplicateCompare;
            if (cfg.minkeysIsSet)
                db.set_bt_minkey(cfg.MinKeysPerPage);
        }

        /// <summary>
        /// Instantiate a new SecondaryBTreeDatabase object, open the
        /// database represented by <paramref name="Filename"/> and associate 
        /// the database with the
        /// <see cref="SecondaryDatabaseConfig.Primary">primary index</see>.
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
        public static SecondaryBTreeDatabase Open(
            string Filename, SecondaryBTreeDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryBTreeDatabase object, open the
        /// database represented by <paramref name="Filename"/> and associate 
        /// the database with the
        /// <see cref="SecondaryDatabaseConfig.Primary">primary index</see>.
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
        public static SecondaryBTreeDatabase Open(string Filename,
            string DatabaseName, SecondaryBTreeDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryBTreeDatabase object, open the
        /// database represented by <paramref name="Filename"/> and associate 
        /// the database with the
        /// <see cref="SecondaryDatabaseConfig.Primary">primary index</see>.
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
        public static SecondaryBTreeDatabase Open(string Filename,
            SecondaryBTreeDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new SecondaryBTreeDatabase object, open the
        /// database represented by <paramref name="Filename"/> and associate 
        /// the database with the
        /// <see cref="SecondaryDatabaseConfig.Primary">primary index</see>.
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
        public static SecondaryBTreeDatabase Open(
            string Filename, string DatabaseName,
            SecondaryBTreeDatabaseConfig cfg, Transaction txn) {
            SecondaryBTreeDatabase ret = new SecondaryBTreeDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn), Filename,
                DatabaseName, cfg.DbType.getDBTYPE(), cfg.openFlags, 0);
            ret.isOpen = true;
            ret.doAssocRef = new BDB_AssociateDelegate(
                SecondaryDatabase.doAssociate);
            cfg.Primary.db.associate(Transaction.getDB_TXN(null),
                ret.db, ret.doAssocRef, cfg.assocFlags);

            if (cfg.ForeignKeyDatabase != null) {
                if (cfg.OnForeignKeyDelete == ForeignKeyDeleteAction.NULLIFY)
                    ret.doNullifyRef =
                        new BDB_AssociateForeignDelegate(doNullify);
                else
                    ret.doNullifyRef = null;
                cfg.ForeignKeyDatabase.db.associate_foreign(
                    ret.db, ret.doNullifyRef, cfg.foreignFlags);
            }
            return ret;
        }

        #endregion Constructors

        #region Callbacks
        private static int doDupCompare(
            IntPtr dbp, IntPtr dbt1p, IntPtr dbt2p) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbt1p, false);
            DBT dbt2 = new DBT(dbt2p, false);

            return ((SecondaryBTreeDatabase)(db.api_internal)).DupCompare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static int doCompare(IntPtr dbp, IntPtr dbtp1, IntPtr dbtp2) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            DBT dbt2 = new DBT(dbtp2, false);
            SecondaryBTreeDatabase tmp =
                (SecondaryBTreeDatabase)db.api_internal;
            return tmp.compareHandler(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static int doPrefixCompare(
            IntPtr dbp, IntPtr dbtp1, IntPtr dbtp2) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            DBT dbt2 = new DBT(dbtp2, false);
            SecondaryBTreeDatabase tmp =
                            (SecondaryBTreeDatabase)db.api_internal;
            return tmp.prefixCompareHandler(
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
                    db.set_bt_compare(doCompare);
                }
                compareHandler = value;
            }
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
        /// If false, empty pages will not be coalesced into higher-level pages.
        /// </summary>
        public bool ReverseSplit {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_REVSPLITOFF) == 0;
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
        
        #endregion Properties
    }
}