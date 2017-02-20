/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing a Berkeley DB database, a base class for access
    /// method specific classes.
    /// </summary>
    public class Database : BaseDatabase, IDisposable {
        private static BDB_FileWriteDelegate writeToFileRef;

        #region Constructors
        /// <summary>
        /// Protected constructor
        /// </summary>
        /// <param name="env">
        /// The environment in which to create this database
        /// </param>
        /// <param name="flags">Flags to pass to the DB->create() method</param>
        protected Database(DatabaseEnvironment env, uint flags)
            : base(env, flags) {
        }
        /// <summary>
        /// Create a new database object with the same underlying DB handle as
        /// <paramref name="clone"/>.  Used during Database.Open to get an
        /// object of the correct DBTYPE.
        /// </summary>
        /// <param name="clone">Database to clone</param>
        protected Database(BaseDatabase clone) : base(clone) { }
        internal static Database fromDB(DB dbp) {
            try {
                return (Database)dbp.api_internal;
            } catch { }
            return null;
        }

        /// <summary>
        /// Instantiate a new Database object and open the database represented
        /// by <paramref name="Filename"/>. The file specified by
        /// <paramref name="Filename"/> must exist.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <see cref="DatabaseConfig.AutoCommit"/> is set, the operation
        /// will be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static Database Open(string Filename, DatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new Database object and open the database represented
        /// by <paramref name="Filename"/> and <paramref name="DatabaseName"/>.
        /// The file specified by <paramref name="Filename"/> must exist.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null and 
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
        /// may be created by setting this parameter to null.</param>
        /// <param name="DatabaseName">
        /// This parameter allows applications to have multiple databases in a
        /// single file. Although no DatabaseName needs to be specified, it is
        /// an error to attempt to open a second database in a file that was not
        /// initially created using a database name.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static Database Open(
            string Filename, string DatabaseName, DatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new Database object and open the database represented
        /// by <paramref name="Filename"/>. The file specified by
        /// <paramref name="Filename"/> must exist.
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
        public static Database Open(
            string Filename, DatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new Database object and open the database represented
        /// by <paramref name="Filename"/> and <paramref name="DatabaseName"/>. 
        /// The file specified by <paramref name="Filename"/> must exist.
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
        public static new Database Open(string Filename,
            string DatabaseName, DatabaseConfig cfg, Transaction txn) {
            Database ret;
            BaseDatabase db = BaseDatabase.Open(
                Filename, DatabaseName, cfg, txn);
            switch (db.Type.getDBTYPE()) {
                case DBTYPE.DB_BTREE:
                    ret = new BTreeDatabase(db);
                    break;
                case DBTYPE.DB_HASH:
                    ret = new HashDatabase(db);
                    break;
                case DBTYPE.DB_QUEUE:
                    ret = new QueueDatabase(db);
                    break;
                case DBTYPE.DB_RECNO:
                    ret = new RecnoDatabase(db);
                    break;
                default:
                    throw new DatabaseException(0);
            }
            db.Dispose();
            ret.isOpen = true;
            return ret;
        }
        #endregion Constructor

        private static int writeToFile(TextWriter OutputStream, string data) {
            OutputStream.Write(data);
            return 0;
        }

        #region Methods
        /// <summary>
        /// If a key/data pair in the database matches <paramref name="key"/>
        /// and <paramref name="data"/>, return the key and all duplicate data
        /// items.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="data">The data to search for</param>
        /// <exception cref="NotFoundException">
        /// A NotFoundException is thrown if <paramref name="key"/> and
        /// <paramref name="data"/> are not in the database. 
        /// </exception>
        /// <exception cref="KeyEmptyException">
        /// A KeyEmptyException is thrown if the database is a
        /// <see cref="QueueDatabase"/> or <see cref="RecnoDatabase"/> 
        /// database and <paramref name="key"/> exists, but was never explicitly
        /// created by the application or was later deleted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetBothMultiple(
            DatabaseEntry key, DatabaseEntry data) {
            /*
             * Make sure we pass a buffer that's big enough to hold data.Data
             * and is a multiple of the page size.  Cache this.Pagesize to avoid
             * multiple P/Invoke calls.
             */
            uint pgsz = Pagesize;
            uint npgs = (data.size / pgsz) + 1;
            return GetBothMultiple(key, data, (int)(npgs * pgsz), null, null);
        }
        /// <summary>
        /// If a key/data pair in the database matches <paramref name="key"/>
        /// and <paramref name="data"/>, return the key and all duplicate data
        /// items.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="data">The data to search for</param>
        /// <param name="BufferSize">
        /// The initial size of the buffer to fill with duplicate data items. If
        /// the buffer is not large enough, it will be automatically resized.
        /// </param>
        /// <exception cref="NotFoundException">
        /// A NotFoundException is thrown if <paramref name="key"/> and
        /// <paramref name="data"/> are not in the database. 
        /// </exception>
        /// <exception cref="KeyEmptyException">
        /// A KeyEmptyException is thrown if the database is a
        /// <see cref="QueueDatabase"/> or <see cref="RecnoDatabase"/> 
        /// database and <paramref name="key"/> exists, but was never explicitly
        /// created by the application or was later deleted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetBothMultiple(
            DatabaseEntry key, DatabaseEntry data, int BufferSize) {
            return GetBothMultiple(key, data, BufferSize, null, null);
        }
        /// <summary>
        /// If a key/data pair in the database matches <paramref name="key"/>
        /// and <paramref name="data"/>, return the key and all duplicate data
        /// items.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="data">The data to search for</param>
        /// <param name="BufferSize">
        /// The initial size of the buffer to fill with duplicate data items. If
        /// the buffer is not large enough, it will be automatically resized.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <exception cref="NotFoundException">
        /// A NotFoundException is thrown if <paramref name="key"/> and
        /// <paramref name="data"/> are not in the database. 
        /// </exception>
        /// <exception cref="KeyEmptyException">
        /// A KeyEmptyException is thrown if the database is a
        /// <see cref="QueueDatabase"/> or <see cref="RecnoDatabase"/> 
        /// database and <paramref name="key"/> exists, but was never explicitly
        /// created by the application or was later deleted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetBothMultiple(
            DatabaseEntry key,
            DatabaseEntry data, int BufferSize, Transaction txn) {
            return GetBothMultiple(key, data, BufferSize, txn, null);
        }
        /// <summary>
        /// If a key/data pair in the database matches <paramref name="key"/>
        /// and <paramref name="data"/>, return the key and all duplicate data
        /// items.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="data">The data to search for</param>
        /// <param name="BufferSize">
        /// The initial size of the buffer to fill with duplicate data items. If
        /// the buffer is not large enough, it will be automatically resized.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
        /// <exception cref="NotFoundException">
        /// A NotFoundException is thrown if <paramref name="key"/> and
        /// <paramref name="data"/> are not in the database. 
        /// </exception>
        /// <exception cref="KeyEmptyException">
        /// A KeyEmptyException is thrown if the database is a
        /// <see cref="QueueDatabase"/> or <see cref="RecnoDatabase"/> 
        /// database and <paramref name="key"/> exists, but was never explicitly
        /// created by the application or was later deleted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public
            KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetBothMultiple(
            DatabaseEntry key, DatabaseEntry data,
            int BufferSize, Transaction txn, LockingInfo info) {
            KeyValuePair<DatabaseEntry, DatabaseEntry> kvp;
            int datasz = (int)data.Data.Length;

            for (; ; ) {
                byte[] udata = new byte[BufferSize];
                Array.Copy(data.Data, udata, datasz);
                data.UserData = udata;
                data.size = (uint)datasz;
                try {
                    kvp = Get(key, data, txn, info,
                        DbConstants.DB_MULTIPLE | DbConstants.DB_GET_BOTH);
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
        /// Retrieve a key and all duplicate data items from the database.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <exception cref="NotFoundException">
        /// A NotFoundException is thrown if <paramref name="key"/> is not in
        /// the database. 
        /// </exception>
        /// <exception cref="KeyEmptyException">
        /// A KeyEmptyException is thrown if the database is a
        /// <see cref="QueueDatabase"/> or <see cref="RecnoDatabase"/> 
        /// database and <paramref name="key"/> exists, but was never explicitly
        /// created by the application or was later deleted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            DatabaseEntry key) {
            return GetMultiple(key, (int)Pagesize, null, null);
        }
        /// <summary>
        /// Retrieve a key and all duplicate data items from the database.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="BufferSize">
        /// The initial size of the buffer to fill with duplicate data items. If
        /// the buffer is not large enough, it will be automatically resized.
        /// </param>
        /// <exception cref="NotFoundException">
        /// A NotFoundException is thrown if <paramref name="key"/> is not in
        /// the database. 
        /// </exception>
        /// <exception cref="KeyEmptyException">
        /// A KeyEmptyException is thrown if the database is a
        /// <see cref="QueueDatabase"/> or <see cref="RecnoDatabase"/> 
        /// database and <paramref name="key"/> exists, but was never explicitly
        /// created by the application or was later deleted.
        /// </exception>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            DatabaseEntry key, int BufferSize) {
            return GetMultiple(key, BufferSize, null, null);
        }
        /// <summary>
        /// Retrieve a key and all duplicate data items from the database.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="BufferSize">
        /// The initial size of the buffer to fill with duplicate data items. If
        /// the buffer is not large enough, it will be automatically resized.
        /// </param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            DatabaseEntry key, int BufferSize, Transaction txn) {
            return GetMultiple(key, BufferSize, txn, null);
        }
        /// <summary>
        /// Retrieve a key and all duplicate data items from the database.
        /// </summary>
        /// <param name="key">The key to search for</param>
        /// <param name="BufferSize">
        /// The initial size of the buffer to fill with duplicate data items. If
        /// the buffer is not large enough, it will be automatically resized.
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
        /// A <see cref="KeyValuePair{T,T}"/>
        /// whose Key parameter is <paramref name="key"/> and whose Value
        /// parameter is the retrieved data items.
        /// </returns>
        public KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> GetMultiple(
            DatabaseEntry key,
            int BufferSize, Transaction txn, LockingInfo info) {
            KeyValuePair<DatabaseEntry, DatabaseEntry> kvp;

            DatabaseEntry data = new DatabaseEntry();
            for (; ; ) {
                data.UserData = new byte[BufferSize];
                try {
                    kvp = Get(key, data, txn, info, DbConstants.DB_MULTIPLE);
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
        /// Create a specialized join cursor for use in performing equality or
        /// natural joins on secondary indices.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Once the cursors have been passed as part of <paramref name="lst"/>,
        /// they should not be accessed or modified until the newly created
        /// <see cref="JoinCursor"/>has been closed, or else inconsistent
        /// results may be returned.
        /// </para>
        /// <para>
        /// Joined values are retrieved by doing a sequential iteration over the
        /// first cursor in <paramref name="lst"/>, and a nested iteration over
        /// each secondary cursor in the order they are specified in the
        /// curslist parameter. This requires database traversals to search for
        /// the current datum in all the cursors after the first. For this
        /// reason, the best join performance normally results from sorting the
        /// cursors from the one that refers to the least number of data items
        /// to the one that refers to the most.
        /// </para>
        /// </remarks>
        /// <param name="lst">
        /// An array of SecondaryCursors. Each cursor must have been initialized
        /// to refer to the key on which the underlying database should be
        /// joined.
        /// </param>
        /// <param name="sortCursors">
        /// If true, sort the cursors from the one that refers to the least
        /// number of data items to the one that refers to the most.  If the
        /// data are structured so that cursors with many data items also share
        /// many common elements, higher performance will result from listing
        /// those cursors before cursors with fewer data items; that is, a sort
        /// order other than the default. A setting of false permits
        /// applications to perform join optimization prior to calling Join.
        /// </param>
        /// <returns>
        /// A specialized join cursor for use in performing equality or natural
        /// joins on secondary indices.
        /// </returns>
        public JoinCursor Join(SecondaryCursor[] lst, bool sortCursors) {
            int i;
            IntPtr[] cursList = new IntPtr[lst.Length + 1];
            for (i = 0; i < lst.Length; i++)
                cursList[i] = DBC.getCPtr(
                    SecondaryCursor.getDBC(lst[i])).Handle;
            cursList[i] = IntPtr.Zero;
            return new JoinCursor(db.join(cursList,
                sortCursors ? 0 : DbConstants.DB_JOIN_NOSORT));
        }

        /// <summary>
        /// Store the key/data pair in the database, replacing any previously
        /// existing key if duplicates are disallowed, or adding a duplicate
        /// data item if duplicates are allowed.
        /// </summary>
        /// <overloads>
        /// <para>
        /// If the database supports duplicates, add the new data value at the
        /// end of the duplicate set. If the database supports sorted
        /// duplicates, the new data value is inserted at the correct sorted
        /// location.
        /// </para>
        /// </overloads>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        public void Put(DatabaseEntry key, DatabaseEntry data) {
            Put(key, data, null);
        }
        /// <summary>
        /// Store the key/data pair in the database, replacing any previously
        /// existing key if duplicates are disallowed, or adding a duplicate
        /// data item if duplicates are allowed.
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
        public void Put(
            DatabaseEntry key, DatabaseEntry data, Transaction txn) {
            Put(key, data, txn, 0);
        }

        /// <summary>
        /// Store the key/data pair in the database, only if the key does not
        /// already appear in the database.
        /// </summary>
        /// <remarks>
        /// This enforcement of uniqueness of keys applies only to the primary
        /// key, the behavior of insertions into secondary databases is not
        /// affected. In particular, the insertion of a record that would result
        /// in the creation of a duplicate key in a secondary database that
        /// allows duplicates would not be prevented by the use of this flag.
        /// </remarks>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        public void PutNoOverwrite(DatabaseEntry key, DatabaseEntry data) {
            PutNoOverwrite(key, data, null);
        }
        /// <summary>
        /// Store the key/data pair in the database, only if the key does not
        /// already appear in the database.
        /// </summary>
        /// <remarks>
        /// This enforcement of uniqueness of keys applies only to the primary
        /// key, the behavior of insertions into secondary databases is not
        /// affected. In particular, the insertion of a record that would result
        /// in the creation of a duplicate key in a secondary database that
        /// allows duplicates would not be prevented by the use of this flag.
        /// </remarks>
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
        public void PutNoOverwrite(
            DatabaseEntry key, DatabaseEntry data, Transaction txn) {
            Put(key, data, txn, DbConstants.DB_NOOVERWRITE);
        }

        /// <summary>
        /// Protected wrapper for DB->put.  Used by subclasses for access method
        /// specific operations.
        /// </summary>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        /// <param name="txn">Transaction with which to protect the put</param>
        /// <param name="flags">Flags to pass to DB->put</param>
        protected void Put(DatabaseEntry key,
            DatabaseEntry data, Transaction txn, uint flags) {
            db.put(Transaction.getDB_TXN(txn), key, data, flags);
        }

        /// <summary>
        /// Write the key/data pairs from all databases in the file to 
        /// <see cref="Console.Out"/>. Key values are written for Btree, Hash
        /// and Queue databases, but not for Recno databases.
        /// </summary>
        /// <param name="file">
        /// The physical file in which the databases to be salvaged are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be salvaged.
        /// </param>
        public static void Salvage(string file, DatabaseConfig cfg) {
            Salvage(file, cfg, false, false, null);
        }
        /// <summary>
        /// Write the key/data pairs from all databases in the file to 
        /// <see cref="Console.Out"/>. Key values are written for Btree, Hash
        /// and Queue databases, but not for Recno databases.
        /// </summary>
        /// <param name="file">
        /// The physical file in which the databases to be salvaged are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be salvaged.
        /// </param>
        /// <param name="Printable">
        /// If true and characters in either the key or data items are printing
        /// characters (as defined by isprint(3)), use printing characters to
        /// represent them. This setting permits users to use standard text
        /// editors and tools to modify the contents of databases or selectively
        /// remove data from salvager output. 
        /// </param>
        public static void Salvage(
            string file, DatabaseConfig cfg, bool Printable) {
            Salvage(file, cfg, Printable, false, null);
        }
        /// <summary>
        /// Write the key/data pairs from all databases in the file to 
        /// <paramref name="OutputStream"/>. Key values are written for Btree,
        /// Hash and Queue databases, but not for Recno databases.
        /// </summary>
        /// <param name="file">
        /// The physical file in which the databases to be salvaged are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be salvaged.
        /// </param>
        /// <param name="OutputStream">
        /// The TextWriter to which the databases' key/data pairs are written.
        /// If null, <see cref="Console.Out"/> will be used.
        /// </param>
        public static void Salvage(
            string file, DatabaseConfig cfg, TextWriter OutputStream) {
            Salvage(file, cfg, false, false, OutputStream);
        }
        /// <summary>
        /// Write the key/data pairs from all databases in the file to 
        /// <paramref name="OutputStream"/>. Key values are written for Btree,
        /// Hash and Queue databases, but not for Recno databases.
        /// </summary>
        /// <param name="file">
        /// The physical file in which the databases to be salvaged are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be salvaged.
        /// </param>
        /// <param name="Printable">
        /// If true and characters in either the key or data items are printing
        /// characters (as defined by isprint(3)), use printing characters to
        /// represent them. This setting permits users to use standard text
        /// editors and tools to modify the contents of databases or selectively
        /// remove data from salvager output. 
        /// </param>
        /// <param name="OutputStream">
        /// The TextWriter to which the databases' key/data pairs are written.
        /// If null, <see cref="Console.Out"/> will be used.
        /// </param>
        public static void Salvage(string file,
            DatabaseConfig cfg, bool Printable, TextWriter OutputStream) {
            Salvage(file, cfg, Printable, false, OutputStream);
        }
        /// <summary>
        /// Write the key/data pairs from all databases in the file to 
        /// <see cref="Console.Out"/>. Key values are written for Btree, Hash
        /// and Queue databases, but not for Recno databases.
        /// </summary>
        /// <param name="file">
        /// The physical file in which the databases to be salvaged are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be salvaged.
        /// </param>
        /// <param name="Printable">
        /// If true and characters in either the key or data items are printing
        /// characters (as defined by isprint(3)), use printing characters to
        /// represent them. This setting permits users to use standard text
        /// editors and tools to modify the contents of databases or selectively
        /// remove data from salvager output. 
        /// </param>
        /// <param name="Aggressive">
        /// If true, output all the key/data pairs in the file that can be
        /// found.  Corruption will be assumed and key/data pairs that are
        /// corrupted or have been deleted may appear in the output (even if the
        /// file being salvaged is in no way corrupt), and the output will
        /// almost certainly require editing before being loaded into a
        /// database.
        /// </param>
        public static void Salvage(string file,
            DatabaseConfig cfg, bool Printable, bool Aggressive) {
            Salvage(file, cfg, Printable, Aggressive, null);
        }
        /// <summary>
        /// Write the key/data pairs from all databases in the file to 
        /// <paramref name="OutputStream"/>. Key values are written for Btree,
        /// Hash and Queue databases, but not for Recno databases.
        /// </summary>
        /// <param name="file">
        /// The physical file in which the databases to be salvaged are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be salvaged.
        /// </param>
        /// <param name="Printable">
        /// If true and characters in either the key or data items are printing
        /// characters (as defined by isprint(3)), use printing characters to
        /// represent them. This setting permits users to use standard text
        /// editors and tools to modify the contents of databases or selectively
        /// remove data from salvager output. 
        /// </param>
        /// <param name="Aggressive">
        /// If true, output all the key/data pairs in the file that can be
        /// found.  Corruption will be assumed and key/data pairs that are
        /// corrupted or have been deleted may appear in the output (even if the
        /// file being salvaged is in no way corrupt), and the output will
        /// almost certainly require editing before being loaded into a
        /// database.
        /// </param>
        /// <param name="OutputStream">
        /// The TextWriter to which the databases' key/data pairs are written.
        /// If null, <see cref="Console.Out"/> will be used.
        /// </param>
        public static void Salvage(string file, DatabaseConfig cfg,
            bool Printable, bool Aggressive, TextWriter OutputStream) {
            using (Database db = new Database(cfg.Env, 0)) {
                db.Config(cfg);
                if (OutputStream == null)
                    OutputStream = Console.Out;
                uint flags = DbConstants.DB_SALVAGE;
                flags |= Aggressive ? DbConstants.DB_AGGRESSIVE : 0;
                flags |= Printable ? DbConstants.DB_PRINTABLE : 0;
                writeToFileRef = new BDB_FileWriteDelegate(writeToFile);
                db.db.verify(file, null, OutputStream, writeToFileRef, flags);
            }
        }

        /// <summary>
        /// Upgrade all of the databases included in the file
        /// <paramref name="file"/>, if necessary. If no upgrade is necessary,
        /// Upgrade always returns successfully.
        /// </summary>
        /// <param name="file">
        /// The physical file containing the databases to be upgraded.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be upgraded.
        /// </param>
        public static void Upgrade(string file, DatabaseConfig cfg) {
            Upgrade(file, cfg, false);
        }
        /// <summary>
        /// Upgrade all of the databases included in the file
        /// <paramref name="file"/>, if necessary. If no upgrade is necessary,
        /// Upgrade always returns successfully.
        /// </summary>
        /// <overloads>
        /// Database upgrades are done in place and are destructive. For
        /// example, if pages need to be allocated and no disk space is
        /// available, the database may be left corrupted. Backups should be
        /// made before databases are upgraded. See Upgrading databases in the
        /// Programmer's Reference Guide for more information.
        /// </overloads>
        /// <remarks>
        /// <para>
        /// As part of the upgrade from the Berkeley DB 3.0 release to the 3.1
        /// release, the on-disk format of duplicate data items changed. To
        /// correctly upgrade the format requires applications to specify
        /// whether duplicate data items in the database are sorted or not.
        /// Specifying <paramref name="dupSortUpgraded"/> informs Upgrade that
        /// the duplicates are sorted; otherwise they are assumed to be
        /// unsorted. Incorrectly specifying the value of this flag may lead to
        /// database corruption.
        /// </para>
        /// <para>
        /// Further, because this method upgrades a physical file (including all
        /// the databases it contains), it is not possible to use Upgrade to
        /// upgrade files in which some of the databases it includes have sorted
        /// duplicate data items, and some of the databases it includes have
        /// unsorted duplicate data items. If the file does not have more than a
        /// single database, if the databases do not support duplicate data
        /// items, or if all of the databases that support duplicate data items
        /// support the same style of duplicates (either sorted or unsorted), 
        /// Upgrade will work correctly as long as
        /// <paramref name="dupSortUpgraded"/> is correctly specified.
        /// Otherwise, the file cannot be upgraded using Upgrade it must be
        /// upgraded manually by dumping and reloading the databases.
        /// </para>
        /// </remarks>
        /// <param name="file">
        /// The physical file containing the databases to be upgraded.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be upgraded.
        /// </param>
        /// <param name="dupSortUpgraded">
        /// If true, the duplicates in the upgraded database are sorted;
        /// otherwise they are assumed to be unsorted.  This setting is only 
        /// meaningful when upgrading databases from releases before the
        /// Berkeley DB 3.1 release.
        /// </param>
        public static void Upgrade(
            string file, DatabaseConfig cfg, bool dupSortUpgraded) {
            Database db = new Database(cfg.Env, 0);
            db.Config(cfg);
            db.db.upgrade(file, dupSortUpgraded ? DbConstants.DB_DUPSORT : 0);
        }

        /// <summary>
        /// Specifies the type of verification to perform
        /// </summary>
        public enum VerifyOperation {
            /// <summary>
            /// Perform database checks and check sort order
            /// </summary>
            DEFAULT,
            /// <summary>
            /// Perform the database checks for btree and duplicate sort order
            /// and for hashing
            /// </summary>
            ORDER_CHECK_ONLY,
            /// <summary>
            /// Skip the database checks for btree and duplicate sort order and
            /// for hashing. 
            /// </summary>
            NO_ORDER_CHECK
        };
        /// <summary>
        /// Verify the integrity of all databases in the file specified by 
        /// <paramref name="file"/>.
        /// </summary>
        /// <overloads>
        /// Verify does not perform any locking, even in Berkeley DB
        /// environments that are configured with a locking subsystem. As such,
        /// it should only be used on files that are not being modified by
        /// another thread of control.
        /// </overloads>
        /// <param name="file">
        /// The physical file in which the databases to be verified are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be verified.
        /// </param>
        public static void Verify(string file, DatabaseConfig cfg) {
            Verify(file, null, cfg, VerifyOperation.DEFAULT);
        }
        /// <summary>
        /// Verify the integrity of all databases in the file specified by 
        /// <paramref name="file"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Berkeley DB normally verifies that btree keys and duplicate items
        /// are correctly sorted, and hash keys are correctly hashed. If the
        /// file being verified contains multiple databases using differing
        /// sorting or hashing algorithms, some of them must necessarily fail
        /// database verification because only one sort order or hash function
        /// can be specified in <paramref name="cfg"/>. To verify files with
        /// multiple databases having differing sorting orders or hashing
        /// functions, first perform verification of the file as a whole by
        /// using <see cref="VerifyOperation.NO_ORDER_CHECK"/>, and then
        /// individually verify the sort order and hashing function for each
        /// database in the file using
        /// <see cref="VerifyOperation.ORDER_CHECK_ONLY"/>.
        /// </para>
        /// </remarks>
        /// <param name="file">
        /// The physical file in which the databases to be verified are found.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be verified.
        /// </param>
        /// <param name="op">The extent of verification</param>
        public static void Verify(
            string file, DatabaseConfig cfg, VerifyOperation op) {
            Verify(file, null, cfg, op);
        }
        /// <summary>
        /// Verify the integrity of the database specified by 
        /// <paramref name="file"/> and <paramref name="database"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Berkeley DB normally verifies that btree keys and duplicate items
        /// are correctly sorted, and hash keys are correctly hashed. If the
        /// file being verified contains multiple databases using differing
        /// sorting or hashing algorithms, some of them must necessarily fail
        /// database verification because only one sort order or hash function
        /// can be specified in <paramref name="cfg"/>. To verify files with
        /// multiple databases having differing sorting orders or hashing
        /// functions, first perform verification of the file as a whole by
        /// using <see cref="VerifyOperation.NO_ORDER_CHECK"/>, and then
        /// individually verify the sort order and hashing function for each
        /// database in the file using
        /// <see cref="VerifyOperation.ORDER_CHECK_ONLY"/>.
        /// </para>
        /// </remarks>
        /// <param name="file">
        /// The physical file in which the databases to be verified are found.
        /// </param>
        /// <param name="database">
        /// The database in <paramref name="file"/> on which the database checks
        /// for btree and duplicate sort order and for hashing are to be
        /// performed.  A non-null value for database is only allowed with
        /// <see cref="VerifyOperation.ORDER_CHECK_ONLY"/>.
        /// </param>
        /// <param name="cfg">
        /// Configuration parameters for the databases to be verified.
        /// </param>
        /// <param name="op">The extent of verification</param>
        public static void Verify(string file,
            string database, DatabaseConfig cfg, VerifyOperation op) {
            using (Database db = new Database(cfg.Env, 0)) {
                db.Config(cfg);
                uint flags;
                switch (op) {
                    case VerifyOperation.NO_ORDER_CHECK:
                        flags = DbConstants.DB_NOORDERCHK;
                        break;
                    case VerifyOperation.ORDER_CHECK_ONLY:
                        flags = DbConstants.DB_ORDERCHKONLY;
                        break;
                    case VerifyOperation.DEFAULT:
                    default:
                        flags = 0;
                        break;
                }
                db.db.verify(file, database, null, null, flags);
            }
        }
        #endregion Methods
    }
}
