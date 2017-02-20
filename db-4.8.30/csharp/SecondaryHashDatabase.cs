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
    /// A class representing a SecondaryHashDatabase. The Hash format is an
    /// extensible, dynamic hashing scheme.
    /// </summary>
    public class SecondaryHashDatabase : SecondaryDatabase {
        private HashFunctionDelegate hashHandler;
        private EntryComparisonDelegate compareHandler;
        private EntryComparisonDelegate dupCompareHandler;
        private BDB_CompareDelegate doCompareRef;
        private BDB_HashDelegate doHashRef;
        private BDB_CompareDelegate doDupCompareRef;
        
        #region Constructors
        internal SecondaryHashDatabase(DatabaseEnvironment env, uint flags) : base(env, flags) { }
       
        private void Config(SecondaryHashDatabaseConfig cfg) {
            base.Config(cfg);
            db.set_flags(cfg.flags);
            
            if (cfg.HashFunction != null)
                HashFunction = cfg.HashFunction;
            if (cfg.DuplicateCompare != null)
                DupCompare = cfg.DuplicateCompare;
            if (cfg.fillFactorIsSet)
                db.set_h_ffactor(cfg.FillFactor);
            if (cfg.nelemIsSet)
                db.set_h_nelem(cfg.TableSize);
            if (cfg.Compare != null)
                Compare = cfg.Compare;
        }

        /// <summary>
        /// Instantiate a new SecondaryHashDatabase object, open the
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
        public static SecondaryHashDatabase Open(
            string Filename, SecondaryHashDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryHashDatabase object, open the
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
        public static SecondaryHashDatabase Open(string Filename,
            string DatabaseName, SecondaryHashDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryHashDatabase object, open the
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
        public static SecondaryHashDatabase Open(string Filename,
            SecondaryHashDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new SecondaryHashDatabase object, open the
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
        public static SecondaryHashDatabase Open(
            string Filename, string DatabaseName,
            SecondaryHashDatabaseConfig cfg, Transaction txn) {
            SecondaryHashDatabase ret = new SecondaryHashDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn), Filename,
                DatabaseName, cfg.DbType.getDBTYPE(), cfg.openFlags, 0);
            ret.isOpen = true;
            
            ret.doAssocRef =
                new BDB_AssociateDelegate(SecondaryDatabase.doAssociate);
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

            SecondaryHashDatabase tmp = (SecondaryHashDatabase)db.api_internal;
            return tmp.DupCompare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static uint doHash(IntPtr dbp, IntPtr datap, uint len) {
            DB db = new DB(dbp, false);
            byte[] t_data = new byte[len];
            Marshal.Copy(datap, t_data, 0, (int)len);
            
            SecondaryHashDatabase tmp = (SecondaryHashDatabase)db.api_internal;
            return tmp.HashFunction(t_data);
        }
        private static int doCompare(IntPtr dbp, IntPtr dbtp1, IntPtr dbtp2) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            DBT dbt2 = new DBT(dbtp2, false);

            SecondaryHashDatabase tmp = (SecondaryHashDatabase)db.api_internal;
            return tmp.Compare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        #endregion Callbacks

        #region Properties
        /// <summary>
        /// The secondary Hash key comparison function. The comparison function
        /// is called whenever it is necessary to compare a key specified by the
        /// application with a key currently stored in the tree. 
        /// </summary>
        public EntryComparisonDelegate Compare {
            get { return compareHandler; }
            private set {
                if (value == null)
                    db.set_h_compare(null);
                else if (compareHandler == null) {
                    if (doCompareRef == null)
                        doCompareRef = new BDB_CompareDelegate(doCompare);
                    db.set_h_compare(doCompareRef);
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
        /// The desired density within the hash table.
        /// </summary>
        public uint FillFactor {
            get {
                uint ret = 0;
                db.get_h_ffactor(ref ret);
                return ret;
            }
        }
        /// <summary>
        /// A user-defined hash function; if no hash function is specified, a
        /// default hash function is used. 
        /// </summary>
        public HashFunctionDelegate HashFunction {
            get { return hashHandler; }
            private set {
                if (value == null)
                    db.set_h_hash(null);
                else if (hashHandler == null) {
                    if (doHashRef == null)
                        doHashRef = new BDB_HashDelegate(doHash);
                    db.set_h_hash(doHashRef);
                }
                hashHandler = value;
            }
        }
        /// <summary>
        /// An estimate of the final size of the hash table.
        /// </summary>
        public uint TableSize {
            get {
                uint ret = 0;
                db.get_h_nelem(ref ret);
                return ret;
            }
        }
        #endregion Properties
    }
}
