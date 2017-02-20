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
    /// The base class from which all database classes inherit
    /// </summary>
    public class BaseDatabase : IDisposable {
        internal DB db;
        protected internal DatabaseEnvironment env;
        protected internal bool isOpen;
        private DBTCopyDelegate CopyDelegate;
        private EntryComparisonDelegate dupCompareHandler;
        private DatabaseFeedbackDelegate feedbackHandler;
        private BDB_CompareDelegate doDupCompareRef;
        private BDB_DbFeedbackDelegate doFeedbackRef;

        #region Constructors
        /// <summary>
        /// Protected constructor
        /// </summary>
        /// <param name="envp">
        /// The environment in which to create this database
        /// </param>
        /// <param name="flags">Flags to pass to the DB->create() method</param>
        protected BaseDatabase(DatabaseEnvironment envp, uint flags) {
            db = new DB(envp == null ? null : envp.dbenv, flags);
            db.api_internal = this;
            if (envp == null) {
                env = new DatabaseEnvironment(db.env());
            } else
                env = envp;
        }

        /// <summary>
        /// Create a new database object with the same underlying DB handle as
        /// <paramref name="clone"/>.  Used during Database.Open to get an
        /// object of the correct DBTYPE.
        /// </summary>
        /// <param name="clone">Database to clone</param>
        protected BaseDatabase(BaseDatabase clone) {
            db = clone.db;
            clone.db = null;
            db.api_internal = this;
            env = clone.env;
            clone.env = null;
        }

        internal void Config(DatabaseConfig cfg) {
            // The cache size cannot change.
            if (cfg.CacheSize != null)
                db.set_cachesize(cfg.CacheSize.Gigabytes,
                    cfg.CacheSize.Bytes, cfg.CacheSize.NCaches);
            if (cfg.encryptionIsSet)
                db.set_encrypt(
                    cfg.EncryptionPassword, (uint)cfg.EncryptAlgorithm);
            if (cfg.ErrorPrefix != null)
                ErrorPrefix = cfg.ErrorPrefix;
            if (cfg.ErrorFeedback != null)
                ErrorFeedback = cfg.ErrorFeedback;
            if (cfg.Feedback != null)
                Feedback = cfg.Feedback;

            db.set_flags(cfg.flags);
            if (cfg.ByteOrder != ByteOrder.MACHINE)
                db.set_lorder(cfg.ByteOrder.lorder);
            if (cfg.pagesizeIsSet)
                db.set_pagesize(cfg.PageSize);
            if (cfg.Priority != CachePriority.DEFAULT)
                db.set_priority(cfg.Priority.priority);
        }

        /// <summary>
        /// Protected factory method to create and open a new database object.
        /// </summary>
        /// <param name="Filename">The database's filename</param>
        /// <param name="DatabaseName">The subdatabase's name</param>
        /// <param name="cfg">The database's configuration</param>
        /// <param name="txn">
        /// The transaction in which to open the database
        /// </param>
        /// <returns>A new, open database object</returns>
        protected static BaseDatabase Open(string Filename,
            string DatabaseName, DatabaseConfig cfg, Transaction txn) {
            BaseDatabase ret = new BaseDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, DatabaseName, DBTYPE.DB_UNKNOWN, cfg.openFlags, 0);
            return ret;
        }
        #endregion Constructor

        #region Callbacks
        private static void doFeedback(IntPtr dbp, int opcode, int percent) {
            DB db = new DB(dbp, false);
            db.api_internal.Feedback((DatabaseFeedbackEvent)opcode, percent);
        }
        #endregion Callbacks

        #region Properties
        // Sorted alpha by property name
        /// <summary>
        /// If true, all database modification operations based on this object
        /// will be transactionally protected.
        /// </summary>
        public bool AutoCommit {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_AUTO_COMMIT) != 0;
            }
        }
        /// <summary>
        /// The size of the shared memory buffer pool -- that is, the cache.
        /// </summary>
        public CacheInfo CacheSize {
            get {
                uint gb = 0;
                uint b = 0;
                int n = 0;
                db.get_cachesize(ref gb, ref b, ref n);
                return new CacheInfo(gb, b, n);
            }
        }
        /// <summary>
        /// The CreatePolicy with which this database was opened.
        /// </summary>
        public CreatePolicy Creation {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                if ((flags & DbConstants.DB_EXCL) != 0)
                    return CreatePolicy.ALWAYS;
                else if ((flags & DbConstants.DB_CREATE) != 0)
                    return CreatePolicy.IF_NEEDED;
                else
                    return CreatePolicy.NEVER;
            }
        }
        /// <summary>
        /// The name of this database, if it has one.
        /// </summary>
        public string DatabaseName {
            get {
                string ret = "";
                string tmp = "";
                db.get_dbname(ref tmp, ref ret);
                return ret;
            }
        }
        /// <summary>
        /// If true, do checksum verification of pages read into the cache from
        /// the backing filestore.
        /// </summary>
        /// <remarks>
        /// Berkeley DB uses the SHA1 Secure Hash Algorithm if encryption is
        /// configured and a general hash algorithm if it is not.
        /// </remarks>
        public bool DoChecksum {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_CHKSUM) != 0;
            }
        }

        /// <summary>
        /// The algorithm used by the Berkeley DB library to perform encryption
        /// and decryption. 
        /// </summary>
        public EncryptionAlgorithm EncryptAlgorithm {
            get {
                uint flags = 0;
                db.get_encrypt_flags(ref flags);
                return (EncryptionAlgorithm)Enum.ToObject(
                    typeof(EncryptionAlgorithm), flags);
            }
        }
        /// <summary>
        /// If true, encrypt all data stored in the database.
        /// </summary>
        public bool Encrypted {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_ENCRYPT) != 0;
            }
        }
        /// <summary>
        /// The database byte order.
        /// </summary>
        public ByteOrder Endianness {
            get {
                int lorder = 0;
                db.get_lorder(ref lorder);
                return ByteOrder.FromConst(lorder);
            }
        }
        /// <summary>
        /// The mechanism for reporting detailed error messages to the
        /// application.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When an error occurs in the Berkeley DB library, a
        /// <see cref="DatabaseException"/>, or subclass of DatabaseException,
        /// is thrown. In some cases, however, the exception may be insufficient
        /// to completely describe the cause of the error, especially during
        /// initial application debugging.
        /// </para>
        /// <para>
        /// In some cases, when an error occurs, Berkeley DB will call the given
        /// delegate with additional error information. It is up to the delegate
        /// to display the error message in an appropriate manner.
        /// </para>
        /// <para>
        /// Setting ErrorFeedback to NULL unconfigures the callback interface.
        /// </para>
        /// <para>
        /// This error-logging enhancement does not slow performance or
        /// significantly increase application size, and may be run during
        /// normal operation as well as during application debugging.
        /// </para>
        /// <para>
        /// For databases opened inside of a DatabaseEnvironment, setting
        /// ErrorFeedback affects the entire environment and is equivalent to
        /// setting DatabaseEnvironment.ErrorFeedback.
        /// </para>
        /// <para>
        /// For databases not opened in an environment, setting ErrorFeedback
        /// configures operations performed using the specified object, not all
        /// operations performed on the underlying database. 
        /// </para>
        /// </remarks>
        public ErrorFeedbackDelegate ErrorFeedback {
            get { return env.ErrorFeedback; }
            set { env.ErrorFeedback = value; }
        }
        /// <summary>
        /// The prefix string that appears before error messages issued by
        /// Berkeley DB.
        /// </summary>
        /// <remarks>
        /// <para>
        /// For databases opened inside of a DatabaseEnvironment, setting
        /// ErrorPrefix affects the entire environment and is equivalent to
        /// setting <see cref="DatabaseEnvironment.ErrorPrefix"/>.
        /// </para>
        /// <para>
        /// Setting ErrorPrefix configures operations performed using the
        /// specified object, not all operations performed on the underlying
        /// database. 
        /// </para>
        /// </remarks>
        public string ErrorPrefix {
            get { return env.ErrorPrefix; }
            set { env.ErrorPrefix = value; }
        }
        /// <summary>
        /// Monitor progress within long running operations.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Some operations performed by the Berkeley DB library can take
        /// non-trivial amounts of time. The Feedback delegate can be used by
        /// applications to monitor progress within these operations. When an
        /// operation is likely to take a long time, Berkeley DB will call the
        /// specified delegate with progress information.
        /// </para>
        /// <para>
        /// It is up to the delegate to display this information in an
        /// appropriate manner. 
        /// </para>
        /// </remarks>
        public DatabaseFeedbackDelegate Feedback {
            get { return feedbackHandler; }
            set {
                if (value == null)
                    db.set_feedback(null);
                else if (feedbackHandler == null) {
                    if (doFeedbackRef == null)
                        doFeedbackRef = new BDB_DbFeedbackDelegate(doFeedback);
                    db.set_feedback(doFeedbackRef);
                }
                feedbackHandler = value;
            }
        }
        /// <summary>
        /// The filename of this database, if it has one.
        /// </summary>
        public string FileName {
            get {
                string ret = "";
                string tmp = "";
                db.get_dbname(ref ret, ref tmp);
                return ret;
            }
        }
        /// <summary>
        /// If true, the object is free-threaded; that is, concurrently usable
        /// by multiple threads in the address space. 
        /// </summary>
        public bool FreeThreaded {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_THREAD) != 0;
            }
        }
        /// <summary>
        /// If true, the object references a physical file supporting multiple
        /// databases.
        /// </summary>
        /// <remarks>
        /// If true, the object is a handle on a database whose key values are
        /// the names of the databases stored in the physical file and whose
        /// data values are opaque objects. No keys or data values may be
        /// modified or stored using the database handle. 
        /// </remarks>
        public bool HasMultiple { get { return (db.get_multiple() != 0); } }
        /// <summary>
        /// If true, the underlying database files were created on an
        /// architecture of the same byte order as the current one.  This
        /// information may be used to determine whether application data needs
        /// to be adjusted for this architecture or not. 
        /// </summary>
        public bool InHostOrder {
            get {
                int isswapped = 0;
                db.get_byteswapped(ref isswapped);
                return (isswapped == 0);
            }
        }
        /// <summary>
        /// <para>
        /// If true, this database is not mapped into process memory.
        /// </para>
        /// <para>
        /// See <see cref="DatabaseEnvironment.MMapSize"/> for further
        /// information. 
        /// </para>
        /// </summary>
        public bool NoMMap {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_NOMMAP) == 0;
            }
        }
        /// <summary>
        /// If true, Berkeley DB will not write log records for this database.
        /// </summary>
        public bool NonDurableTxns {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                return (flags & DbConstants.DB_TXN_NOT_DURABLE) != 0;
            }
        }
        /// <summary>
        /// The database's current page size.
        /// </summary>
        /// <remarks>  If <see cref="DatabaseConfig.PageSize"/> was not set by
        /// your application, then the default pagesize is selected based on the
        /// underlying filesystem I/O block size.
        /// </remarks>
        public uint Pagesize {
            get {
                uint pgsz = 0;
                db.get_pagesize(ref pgsz);
                return pgsz;
            }
        }
        /// <summary>
        /// The cache priority for pages referenced by this object.
        /// </summary>
        public CachePriority Priority {
            get {
                uint pri = 0;
                db.get_priority(ref pri);
                return CachePriority.fromUInt(pri);
            }
        }
        /// <summary>
        /// If true, this database has been opened for reading only. Any attempt
        /// to modify items in the database will fail, regardless of the actual
        /// permissions of any underlying files. 
        /// </summary>
        public bool ReadOnly {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_RDONLY) != 0;
            }
        }
        /// <summary>
        /// If true, this database supports transactional read operations with
        /// degree 1 isolation. Read operations on the database may request the
        /// return of modified but not yet committed data.
        /// </summary>
        public bool ReadUncommitted {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_READ_UNCOMMITTED) != 0;
            }
        }
        /// <summary>
        /// If true, this database has been opened in a transactional mode.
        /// </summary>
        public bool Transactional {
            get { return (db.get_transactional() != 0); }
        }
        /// <summary>
        /// If true, the underlying file was physically truncated upon open,
        /// discarding all previous databases it might have held.
        /// </summary>
        public bool Truncated {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_TRUNCATE) != 0;
            }
        }
        /// <summary>
        /// The type of the underlying access method (and file format). This
        /// value may be used to determine the type of the database after an
        /// <see cref="Database.Open"/>. 
        /// </summary>
        public DatabaseType Type {
            get {
                DBTYPE mytype = DBTYPE.DB_UNKNOWN;
                db.get_type(ref mytype);
                switch (mytype) {
                    case DBTYPE.DB_BTREE:
                        return DatabaseType.BTREE;
                    case DBTYPE.DB_HASH:
                        return DatabaseType.HASH;
                    case DBTYPE.DB_QUEUE:
                        return DatabaseType.QUEUE;
                    case DBTYPE.DB_RECNO:
                        return DatabaseType.RECNO;
                    default:
                        return DatabaseType.UNKNOWN;
                }
            }
        }
        /// <summary>
        /// If true, the database was opened with support for multiversion
        /// concurrency control.
        /// </summary>
        public bool UseMVCC {
            get {
                uint flags = 0;
                db.get_open_flags(ref flags);
                return (flags & DbConstants.DB_MULTIVERSION) != 0;
            }
        }
        #endregion Properties

        #region Methods
        //Sorted alpha by method name
        /// <summary>
        /// Flush any cached database information to disk, close any open
        /// <see cref="Cursor"/> objects, free any
        /// allocated resources, and close any underlying files.
        /// </summary>
        /// <overloads>
        /// <para>
        /// Although closing a database will close any open cursors, it is
        /// recommended that applications explicitly close all their Cursor
        /// objects before closing the database. The reason why is that when the
        /// cursor is explicitly closed, the memory allocated for it is
        /// reclaimed; however, this will not happen if you close a database
        /// while cursors are still opened.
        /// </para>
        /// <para>
        /// The same rule, for the same reasons, hold true for
        /// <see cref="Transaction"/> objects. Simply make sure you resolve
        /// all your transaction objects before closing your database handle.
        /// </para>
        /// <para>
        /// Because key/data pairs are cached in memory, applications should
        /// make a point to always either close database handles or sync their
        /// data to disk (using <see cref="Sync"/> before exiting, to
        /// ensure that any data cached in main memory are reflected in the
        /// underlying file system.
        /// </para>
        /// <para>
        /// When called on a database that is the primary database for a
        /// secondary index, the primary database should be closed only after
        /// all secondary indices referencing it have been closed.
        /// </para>
        /// <para>
        /// When multiple threads are using the object concurrently, only a
        /// single thread may call the Close method.
        /// </para>
        /// <para>
        /// The object may not be accessed again after Close is called,
        /// regardless of its outcome.
        /// </para>
        /// </overloads>
        public void Close() {
            Close(true);
        }
        /// <summary>
        /// Optionally flush any cached database information to disk, close any
        /// open <see cref="BaseDatabase.Cursor"/> objects, free
        /// any allocated resources, and close any underlying files.
        /// </summary>
        /// <param name="sync">
        /// If false, do not flush cached information to disk.
        /// </param>
        /// <remarks>
        /// <para>
        /// The sync parameter is a dangerous option. It should be set to false 
        /// only if the application is doing logging (with transactions) so that
        /// the database is recoverable after a system or application crash, or
        /// if the database is always generated from scratch after any system or
        /// application crash.
        /// </para>
        /// <para>
        /// It is important to understand that flushing cached information to
        /// disk only minimizes the window of opportunity for corrupted data.
        /// Although unlikely, it is possible for database corruption to happen
        /// if a system or application crash occurs while writing data to the
        /// database. To ensure that database corruption never occurs,
        /// applications must either use transactions and logging with automatic
        /// recovery or edit a copy of the database, and once all applications
        /// using the database have successfully called Close, atomically
        /// replace the original database with the updated copy.
        /// </para>
        /// <para>
        /// Note that this parameter only works when the database has been
        /// opened using an environment. 
        /// </para>
        /// </remarks>
        public void Close(bool sync) {
            db.close(sync ? 0 : DbConstants.DB_NOSYNC);
            isOpen = false;
        }

        /// <summary>
        /// Create a database cursor.
        /// </summary>
        /// <returns>A newly created cursor</returns>
        public Cursor Cursor() { return Cursor(new CursorConfig(), null); }
        /// <summary>
        /// Create a database cursor with the given configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public Cursor Cursor(CursorConfig cfg) { return Cursor(cfg, null); }
        /// <summary>
        /// Create a transactionally protected database cursor.
        /// </summary>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public Cursor Cursor(Transaction txn) {
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
        public Cursor Cursor(CursorConfig cfg, Transaction txn) {
            return new Cursor(db.cursor(
                Transaction.getDB_TXN(txn), cfg.flags), Type, Pagesize);
        }

        /// <summary>
        /// Remove key/data pairs from the database. The key/data pair
        /// associated with <paramref name="key"/> is discarded from the
        /// database. In the presence of duplicate key values, all records
        /// associated with the designated key will be discarded.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When called on a secondary database, remove the key/data pair from
        /// the primary database and all secondary indices.
        /// </para>
        /// <para>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected.
        /// </para>
        /// </remarks>
        /// <param name="key">
        /// Discard the key/data pair associated with <paramref name="key"/>.
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
        public void Delete(DatabaseEntry key) {
            Delete(key, null);
        }
        /// <summary>
        /// Remove key/data pairs from the database. The key/data pair
        /// associated with <paramref name="key"/> is discarded from the
        /// database. In the presence of duplicate key values, all records
        /// associated with the designated key will be discarded.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When called on a secondary database, remove the key/data pair from
        /// the primary database and all secondary indices.
        /// </para>
        /// <para>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </para>
        /// </remarks>
        /// <param name="key">
        /// Discard the key/data pair associated with <paramref name="key"/>.
        /// </param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
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
        public void Delete(DatabaseEntry key, Transaction txn) {
            db.del(Transaction.getDB_TXN(txn), key, 0);
        }

        /// <summary>
        /// Check whether <paramref name="key"/> appears in the database.
        /// </summary>
        /// <remarks>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected.
        /// </remarks>
        /// <param name="key">The key to search for.</param>
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
        /// True if <paramref name="key"/> appears in the database, false
        /// otherwise.
        /// </returns>
        public bool Exists(DatabaseEntry key) {
            return Exists(key, null, null);
        }
        /// <summary>
        /// Check whether <paramref name="key"/> appears in the database.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="key">The key to search for.</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
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
        /// True if <paramref name="key"/> appears in the database, false
        /// otherwise.
        /// </returns>
        public bool Exists(DatabaseEntry key, Transaction txn) {
            return Exists(key, txn, null);
        }
        /// <summary>
        /// Check whether <paramref name="key"/> appears in the database.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="key">The key to search for.</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
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
        /// True if <paramref name="key"/> appears in the database, false
        /// otherwise.
        /// </returns>
        public bool Exists(
            DatabaseEntry key, Transaction txn, LockingInfo info) {
            /* 
             * If the library call does not throw an exception the key exists.
             * If the exception is NotFound the key does not exist and we
             * should return false.  Any other exception should get passed
             * along.
             */
            try {
                db.exists(Transaction.getDB_TXN(txn),
                    key, (info == null) ? 0 : info.flags);
                return true;
            } catch (NotFoundException) {
                return false;
            }
        }

        /// <summary>
        /// Retrieve a key/data pair from the database.  In the presence of
        /// duplicate key values, Get will return the first data item for 
        /// <paramref name="key"/>.
        /// </summary>
        /// <remarks>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected.
        /// </remarks>
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
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public
            KeyValuePair<DatabaseEntry, DatabaseEntry> Get(DatabaseEntry key) {
            return Get(key, null, null);
        }
        /// <summary>
        /// Retrieve a key/data pair from the database.  In the presence of
        /// duplicate key values, Get will return the first data item for 
        /// <paramref name="key"/>.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="key">The key to search for</param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
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
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Get(
            DatabaseEntry key, Transaction txn) {
            return Get(key, txn, null);
        }
        /// <summary>
        /// Retrieve a key/data pair from the database.  In the presence of
        /// duplicate key values, Get will return the first data item for 
        /// <paramref name="key"/>.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="key">The key to search for</param>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="info">The locking behavior to use.</param>
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
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> Get(
            DatabaseEntry key, Transaction txn, LockingInfo info) {
            return Get(key, null, txn, info, 0);
        }

        /// <summary>
        /// Protected method to retrieve data from the underlying DB handle.
        /// </summary>
        /// <param name="key">
        /// The key to search for.  If null a new DatabaseEntry is created.
        /// </param>
        /// <param name="data">
        /// The data to search for.  If null a new DatabaseEntry is created.
        /// </param>
        /// <param name="txn">The txn for this operation.</param>
        /// <param name="info">Locking info for this operation.</param>
        /// <param name="flags">
        /// Flags value specifying which type of get to perform.  Passed
        /// directly to DB->get().
        /// </param>
        /// <returns>
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is the
        /// retrieved data.
        /// </returns>
        protected KeyValuePair<DatabaseEntry, DatabaseEntry> Get(
            DatabaseEntry key,
            DatabaseEntry data, Transaction txn, LockingInfo info, uint flags) {
            if (key == null)
                key = new DatabaseEntry();
            if (data == null)
                data = new DatabaseEntry();
            flags |= info == null ? 0 : info.flags;
            db.get(Transaction.getDB_TXN(txn), key, data, flags);
            return new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
        }


        /// <summary>
        /// Retrieve a key/data pair from the database which matches
        /// <paramref name="key"/> and <paramref name="data"/>.
        /// </summary>
        /// <remarks>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected.
        /// </remarks>
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
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is
        /// <paramref name="data"/>.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> GetBoth(
            DatabaseEntry key, DatabaseEntry data) {
            return GetBoth(key, data, null, null);
        }
        /// <summary>
        /// Retrieve a key/data pair from the database which matches
        /// <paramref name="key"/> and <paramref name="data"/>.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="key">The key to search for</param>
        /// <param name="data">The data to search for</param>
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
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is
        /// <paramref name="data"/>.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> GetBoth(
            DatabaseEntry key, DatabaseEntry data, Transaction txn) {
            return GetBoth(key, data, txn, null);
        }
        /// <summary>
        /// Retrieve a key/data pair from the database which matches
        /// <paramref name="key"/> and <paramref name="data"/>.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="key">The key to search for</param>
        /// <param name="data">The data to search for</param>
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
        /// A <see cref="KeyValuePair{T,T}"/> whose Key
        /// parameter is <paramref name="key"/> and whose Value parameter is
        /// <paramref name="data"/>.
        /// </returns>
        public KeyValuePair<DatabaseEntry, DatabaseEntry> GetBoth(
            DatabaseEntry key,
            DatabaseEntry data, Transaction txn, LockingInfo info) {
            return Get(key, data, txn, info, DbConstants.DB_GET_BOTH);
        }

        /// <summary>
        /// Display the database statistical information which does not require
        /// traversal of the database. 
        /// </summary>
        /// <remarks>
        /// Among other things, this method makes it possible for applications
        /// to request key and record counts without incurring the performance
        /// penalty of traversing the entire database. 
        /// </remarks>
        /// <overloads>
        /// The statistical information is described by the
        /// <see cref="BTreeStats"/>, <see cref="HashStats"/>,
        /// <see cref="QueueStats"/>, and <see cref="RecnoStats"/> classes. 
        /// </overloads>
        public void PrintFastStats() {
            PrintStats(false, true);
        }
        /// <summary>
        /// Display the database statistical information which does not require
        /// traversal of the database. 
        /// </summary>
        /// <remarks>
        /// Among other things, this method makes it possible for applications
        /// to request key and record counts without incurring the performance
        /// penalty of traversing the entire database. 
        /// </remarks>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        public void PrintFastStats(bool PrintAll) {
            PrintStats(PrintAll, true);
        }
        /// <summary>
        /// Display the database statistical information.
        /// </summary>
        /// <overloads>
        /// The statistical information is described by the
        /// <see cref="BTreeStats"/>, <see cref="HashStats"/>,
        /// <see cref="QueueStats"/>, and <see cref="RecnoStats"/> classes. 
        /// </overloads>
        public void PrintStats() {
            PrintStats(false, false);
        }
        /// <summary>
        /// Display the database statistical information.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        public void PrintStats(bool PrintAll) {
            PrintStats(PrintAll, false);
        }
        private void PrintStats(bool all, bool fast) {
            uint flags = 0;
            flags |= all ? DbConstants.DB_STAT_ALL : 0;
            flags |= fast ? DbConstants.DB_FAST_STAT : 0;
            db.stat_print(flags);
        }

        /// <summary>
        /// Remove the underlying file represented by
        /// <paramref name="Filename"/>, incidentally removing all of the
        /// databases it contained.
        /// </summary>
        /// <param name="Filename">The file to remove</param>
        public static void Remove(string Filename) {
            Remove(Filename, null, null);
        }
        /// <summary>
        /// Remove the underlying file represented by
        /// <paramref name="Filename"/>, incidentally removing all of the
        /// databases it contained.
        /// </summary>
        /// <param name="Filename">The file to remove</param>
        /// <param name="DbEnv">
        /// The DatabaseEnvironment the database belongs to
        /// </param>
        public static void Remove(string Filename, DatabaseEnvironment DbEnv) {
            Remove(Filename, null, DbEnv);
        }
        /// <summary>
        /// Remove the database specified by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <param name="Filename">The file to remove</param>
        /// <param name="DatabaseName">The database to remove</param>
        public static void Remove(string Filename, string DatabaseName) {
            Remove(Filename, DatabaseName, null);
        }
        /// <summary>
        /// Remove the database specified by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <overloads>
        /// <para>
        /// Applications should never remove databases with open DB handles, or
        /// in the case of removing a file, when any database in the file has an
        /// open handle. For example, some architectures do not permit the
        /// removal of files with open system handles. On these architectures,
        /// attempts to remove databases currently in use by any thread of
        /// control in the system may fail.
        /// </para>
        /// <para>
        /// Remove should not be called if the remove is intended to be
        /// transactionally safe;
        /// <see cref="DatabaseEnvironment.RemoveDB"/> should be
        /// used instead. 
        /// </para>
        /// </overloads>
        /// <param name="Filename">The file to remove</param>
        /// <param name="DatabaseName">The database to remove</param>
        /// <param name="DbEnv">
        /// The DatabaseEnvironment the database belongs to
        /// </param>
        public static void Remove(
            string Filename, string DatabaseName, DatabaseEnvironment DbEnv) {
            BaseDatabase db = new BaseDatabase(DbEnv, 0);
            db.db.remove(Filename, DatabaseName, 0);
        }

        /// <summary>
        /// Rename the underlying file represented by
        /// <paramref name="Filename"/>, incidentally renaming all of the
        /// databases it contained.
        /// </summary>
        /// <param name="Filename">The file to rename</param>
        /// <param name="NewName">The new filename</param>
        public static void Rename(string Filename, string NewName) {
            Rename(Filename, null, NewName, null);
        }
        /// <summary>
        /// Rename the underlying file represented by
        /// <paramref name="Filename"/>, incidentally renaming all of the
        /// databases it contained.
        /// </summary>
        /// <param name="Filename">The file to rename</param>
        /// <param name="NewName">The new filename</param>
        /// <param name="DbEnv">
        /// The DatabaseEnvironment the database belongs to
        /// </param>
        public static void Rename(
            string Filename, string NewName, DatabaseEnvironment DbEnv) {
            Rename(Filename, null, NewName, DbEnv);
        }
        /// <summary>
        /// Rename the database specified by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <param name="Filename">The file to rename</param>
        /// <param name="DatabaseName">The database to rename</param>
        /// <param name="NewName">The new database name</param>
        public static void Rename(
            string Filename, string DatabaseName, string NewName) {
            Rename(Filename, DatabaseName, NewName, null);
        }
        /// <summary>
        /// Rename the database specified by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <overloads>
        /// <para>
        /// Applications should not rename databases that are currently in use.
        /// If an underlying file is being renamed and logging is currently
        /// enabled in the database environment, no database in the file may be
        /// open when Rename is called. In particular, some architectures do not
        /// permit renaming files with open handles. On these architectures,
        /// attempts to rename databases that are currently in use by any thread
        /// of control in the system may fail. 
        /// </para>
        /// <para>
        /// Rename should not be called if the rename is intended to be
        /// transactionally safe;
        /// <see cref="DatabaseEnvironment.RenameDB"/> should be
        /// used instead. 
        /// </para>
        /// </overloads>
        /// <param name="Filename">The file to rename</param>
        /// <param name="DatabaseName">The database to rename</param>
        /// <param name="NewName">The new database name</param>
        /// <param name="DbEnv">
        /// The DatabaseEnvironment the database belongs to
        /// </param>
        public static void Rename(string Filename,
            string DatabaseName, string NewName, DatabaseEnvironment DbEnv) {
            BaseDatabase db = new BaseDatabase(DbEnv, 0);
            db.db.rename(Filename, DatabaseName, NewName, 0);
        }

        /// <summary>
        /// Flush any cached information to disk.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If the database is in memory only, Sync has no effect and will
        /// always succeed.
        /// </para>
        /// <para>
        /// It is important to understand that flushing cached information to
        /// disk only minimizes the window of opportunity for corrupted data.
        /// Although unlikely, it is possible for database corruption to happen
        /// if a system or application crash occurs while writing data to the
        /// database. To ensure that database corruption never occurs, 
        /// applications must either: use transactions and logging with
        /// automatic recovery or edit a copy of the database, and once all
        /// applications using the database have successfully called
        /// <see cref="BaseDatabase.Close"/>, atomically replace
        /// the original database with the updated copy.
        /// </para>
        /// </remarks>
        public void Sync() {
            db.sync(0);
        }

        /// <summary>
        /// Empty the database, discarding all records it contains.
        /// </summary>
        /// <remarks>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected.
        /// </remarks>
        /// <overloads>
        /// When called on a database configured with secondary indices, 
        /// Truncate will truncate the primary database and all secondary
        /// indices. A count of the records discarded from the primary database
        /// is returned. 
        /// </overloads>
        /// <returns>
        /// The number of records discarded from the database.
        ///</returns>
        public uint Truncate() {
            return Truncate(null);
        }
        /// <summary>
        /// Empty the database, discarding all records it contains.
        /// </summary>
        /// <remarks>
        /// If <paramref name="txn"/> is null and the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected.
        /// </remarks>
        /// <param name="txn">
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// The number of records discarded from the database.
        ///</returns>
        public uint Truncate(Transaction txn) {
            uint countp = 0;
            db.truncate(Transaction.getDB_TXN(txn), ref countp, 0);
            return countp;
        }

        #endregion Methods

        /// <summary>
        /// Release the resources held by this object, and close the database if
        /// it's still open.
        /// </summary>
        public void Dispose() {
            if (isOpen)
                this.Close();
            if (db != null)
                this.db.Dispose();

            GC.SuppressFinalize(this);
        }
    }
}
