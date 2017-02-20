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
    /// A class representing a SecondaryQueueDatabase. The Queue format supports
    /// fast access to fixed-length records accessed sequentially or by logical
    /// record number.
    /// </summary>
    public class SecondaryQueueDatabase : SecondaryDatabase {
        
        #region Constructors
        internal SecondaryQueueDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }

        private void Config(SecondaryQueueDatabaseConfig cfg) {
            base.Config((SecondaryDatabaseConfig)cfg);
            db.set_flags(cfg.flags);
            if (cfg.lengthIsSet)
                db.set_re_len(cfg.Length);
            if (cfg.padIsSet)
                db.set_re_pad(cfg.PadByte);
            if (cfg.extentIsSet)
                db.set_q_extentsize(cfg.ExtentSize);
        }

        /// <summary>
        /// Instantiate a new SecondaryQueueDatabase object, open the
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
        public static SecondaryQueueDatabase Open(
            string Filename, SecondaryQueueDatabaseConfig cfg) {
            return Open(Filename, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryQueueDatabase object, open the
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
        public static SecondaryQueueDatabase Open(string Filename,
            SecondaryQueueDatabaseConfig cfg, Transaction txn) {
            SecondaryQueueDatabase ret = new SecondaryQueueDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, null, DBTYPE.DB_QUEUE, cfg.openFlags, 0);
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
    }
}
