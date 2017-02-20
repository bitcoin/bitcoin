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
    public class SecondaryRecnoDatabase : SecondaryDatabase {
        
        #region Constructors
        internal SecondaryRecnoDatabase(DatabaseEnvironment env, uint flags) 
            : base(env, flags) { }

        private void Config(SecondaryRecnoDatabaseConfig cfg) {
            base.Config((SecondaryDatabaseConfig)cfg);
            db.set_flags(cfg.flags);
            
            if (cfg.delimiterIsSet)
                db.set_re_delim(cfg.Delimiter);
            if (cfg.lengthIsSet)
                db.set_re_len(cfg.Length);
            if (cfg.padIsSet)
                db.set_re_pad(cfg.PadByte);
            if (cfg.BackingFile != null)
                db.set_re_source(cfg.BackingFile);
        }

        /// <summary>
        /// Instantiate a new SecondaryRecnoDatabase object, open the
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
        public static SecondaryRecnoDatabase Open(
            string Filename, SecondaryRecnoDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryRecnoDatabase object, open the
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
        public static SecondaryRecnoDatabase Open(string Filename,
            string DatabaseName, SecondaryRecnoDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryRecnoDatabase object, open the
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
        public static SecondaryRecnoDatabase Open(string Filename,
            SecondaryRecnoDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new SecondaryRecnoDatabase object, open the
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
        public static SecondaryRecnoDatabase Open(string Filename,
            string DatabaseName,
            SecondaryRecnoDatabaseConfig cfg, Transaction txn) {
            SecondaryRecnoDatabase ret = new SecondaryRecnoDatabase(cfg.Env, 0);
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

        #region Properties
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
        /// If true, any <see cref="BackingFile"/> file will be read in its
        /// entirety when <see cref="Open"/> is called. If false,
        /// <see cref="BackingFile"/> may be read lazily. 
        /// </summary>
        public bool Snapshot {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_SNAPSHOT) != 0;
            }
        }
        /// <summary>
        /// The delimiting byte used to mark the end of a record in
        /// <see cref="BackingFile"/>.
        /// </summary>
        public int Delimiter {
            get {
                int ret = 0;
                db.get_re_delim(ref ret);
                return ret;
            }
        }
        /// <summary>
        /// If using fixed-length, not byte-delimited records, the length of the
        /// records. 
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
        /// <summary>
        /// The underlying source file for the Recno access method.
        /// </summary>
        public string BackingFile {
            get {
                string ret = "";
                db.get_re_source(ref ret);
                return ret;
            }
        }
        #endregion Properties

    }
}
