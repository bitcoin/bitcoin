/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing configuration parameters for <see cref="Database"/>
    /// </summary>
    public class DatabaseConfig {
        /// <summary>
        /// The Berkeley DB environment within which to create a database.  If 
        /// null, the database will be created stand-alone; that is, it is not
        /// part of any Berkeley DB environment. 
        /// </summary>
        /// <remarks>
        /// The database access methods automatically make calls to the other
        /// subsystems in Berkeley DB, based on the enclosing environment. For
        /// example, if the environment has been configured to use locking, the
        /// access methods will automatically acquire the correct locks when
        /// reading and writing pages of the database.
        /// </remarks>
        public DatabaseEnvironment Env;

        /// <summary>
        /// The cache priority for pages referenced by the database.
        /// </summary>
        /// <remarks>
        /// The priority of a page biases the replacement algorithm to be more
        /// or less likely to discard a page when space is needed in the buffer
        /// pool. The bias is temporary, and pages will eventually be discarded
        /// if they are not referenced again. This priority is only advisory,
        /// and does not guarantee pages will be treated in a specific way.
        /// </remarks>
        public CachePriority Priority;

        /// <summary>
        /// The size of the shared memory buffer pool -- that is, the cache.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The cache should be the size of the normal working data set of the
        /// application, with some small amount of additional memory for unusual
        /// situations. (Note: the working set is not the same as the number of
        /// pages accessed simultaneously, and is usually much larger.)
        /// </para>
        /// <para>
        /// The default cache size is 256KB, and may not be specified as less
        /// than 20KB. Any cache size less than 500MB is automatically increased
        /// by 25% to account for buffer pool overhead; cache sizes larger than
        /// 500MB are used as specified. The maximum size of a single cache is
        /// 4GB on 32-bit systems and 10TB on 64-bit systems. (All sizes are in
        /// powers-of-two, that is, 256KB is 2^18 not 256,000.) For information
        /// on tuning the Berkeley DB cache size, see Selecting a cache size in
        /// the Programmer's Reference Guide.
        /// </para>
        /// </remarks>
        public CacheInfo CacheSize;

        /// <summary>
        /// The byte order for integers in the stored database metadata.  The
        /// host byte order of the machine where the Berkeley DB library was
        /// compiled is the default value.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The access methods provide no guarantees about the byte ordering of
        /// the application data stored in the database, and applications are
        /// responsible for maintaining any necessary ordering.
        /// </para>
        /// <para>
        /// If creating additional databases in a single physical file, this
        /// parameter will be ignored and the byte order of the existing
        /// databases will be used.
        /// </para>
        /// </remarks>
        public ByteOrder ByteOrder = ByteOrder.MACHINE;

        internal bool pagesizeIsSet;
        private uint pgsz;
        /// <summary>
        /// The size of the pages used to hold items in the database, in bytes.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The minimum page size is 512 bytes, the maximum page size is 64K
        /// bytes, and the page size must be a power-of-two. If the page size is
        /// not explicitly set, one is selected based on the underlying
        /// filesystem I/O block size. The automatically selected size has a
        /// lower limit of 512 bytes and an upper limit of 16K bytes.
        /// </para>
        /// <para>
        /// For information on tuning the Berkeley DB page size, see Selecting a
        /// page size in the Programmer's Reference Guide.
        /// </para>
        /// <para>
        /// If creating additional databases in a single physical file, this
        /// parameter will be ignored and the page size of the existing
        /// databases will be used.
        /// </para>
        /// </remarks>
        public uint PageSize {
            get { return pgsz; }
            set {
                pagesizeIsSet = true;
                pgsz = value;
            }
        }

        internal bool encryptionIsSet;
        private String encryptPwd;
        private EncryptionAlgorithm encryptAlg;
        /// <summary>
        /// Set the password and algorithm used by the Berkeley DB library to
        /// perform encryption and decryption. 
        /// </summary>
        /// <param name="password">
        /// The password used to perform encryption and decryption.
        /// </param>
        /// <param name="alg">
        /// The algorithm used to perform encryption and decryption.
        /// </param>
        public void SetEncryption(String password, EncryptionAlgorithm alg) {
            encryptionIsSet = true;
            encryptPwd = password;
            encryptAlg = alg;
        }
        /// <summary>
        /// The password used to perform encryption and decryption.
        /// </summary>
        public string EncryptionPassword { get { return encryptPwd; } }
        /// <summary>
        /// The algorithm used to perform encryption and decryption.
        /// </summary>
        public EncryptionAlgorithm EncryptAlgorithm {
            get { return encryptAlg; }
        }

        /// <summary>
        /// The prefix string that appears before error messages issued by
        /// Berkeley DB.
        /// </summary>
        public String ErrorPrefix;
        /// <summary>
        /// The mechanism for reporting error messages to the application.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In some cases, when an error occurs, Berkeley DB will call
        /// ErrorFeedback with additional error information. It is up to the
        /// delegate function to display the error message in an appropriate
        /// manner.
        /// </para>
        /// <para>
        /// This error-logging enhancement does not slow performance or
        /// significantly increase application size, and may be run during
        /// normal operation as well as during application debugging.
        /// </para>
        /// <para>
        /// For databases opened inside of Berkeley DB environments, setting
        /// ErrorFeedback affects the entire environment and is equivalent to 
        /// setting <see cref="DatabaseEnvironment.ErrorFeedback"/>.
        /// </para>
        /// </remarks>
        public ErrorFeedbackDelegate ErrorFeedback;

        /// <summary>
        /// 
        /// </summary>
        public DatabaseFeedbackDelegate Feedback;

        /// <summary>
        /// If true, do checksum verification of pages read into the cache from
        /// the backing filestore.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Berkeley DB uses the SHA1 Secure Hash Algorithm if encryption is
        /// configured and a general hash algorithm if it is not.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public bool DoChecksum;

        /// <summary>
        /// If true, Berkeley DB will not write log records for this database.
        /// </summary>
        /// <remarks>
        /// If Berkeley DB does not write log records, updates of this database
        /// will exhibit the ACI (atomicity, consistency, and isolation)
        /// properties, but not D (durability); that is, database integrity will
        /// be maintained, but if the application or system fails, integrity
        /// will not persist. The database file must be verified and/or restored
        /// from backup after a failure. In order to ensure integrity after
        /// application shut down, the database must be synced when closed, or
        /// all database changes must be flushed from the database environment
        /// cache using either
        /// <see cref="DatabaseEnvironment.Checkpoint"/> or
        /// <see cref="DatabaseEnvironment.SyncMemPool"/>. All database objects
        /// for a single physical file must set NonDurableTxns, including
        /// database objects for different databases in a physical file.
        /// </remarks>
        public bool NonDurableTxns;

        internal uint flags {
            get {
                uint ret = 0;
                ret |= DoChecksum ? Internal.DbConstants.DB_CHKSUM : 0;
                ret |= encryptionIsSet ? Internal.DbConstants.DB_ENCRYPT : 0;
                ret |= NonDurableTxns ? Internal.DbConstants.DB_TXN_NOT_DURABLE : 0;
                return ret;
            }
        }

        /// <summary>
        /// Enclose the open call within a transaction. If the call succeeds,
        /// the open operation will be recoverable and all subsequent database
        /// modification operations based on this handle will be transactionally
        /// protected. If the call fails, no database will have been created. 
        /// </summary>
        public bool AutoCommit;
        /// <summary>
        /// Cause the database object to be free-threaded; that is, concurrently
        /// usable by multiple threads in the address space.
        /// </summary>
        public bool FreeThreaded;
        /// <summary>
        /// Do not map this database into process memory.
        /// </summary>
        public bool NoMMap;
        /// <summary>
        /// Open the database for reading only. Any attempt to modify items in
        /// the database will fail, regardless of the actual permissions of any
        /// underlying files. 
        /// </summary>
        public bool ReadOnly;
        /// <summary>
        /// Support transactional read operations with degree 1 isolation.
        /// </summary>
        /// <remarks>
        /// Read operations on the database may request the return of modified
        /// but not yet committed data. This flag must be specified on all
        /// database objects used to perform dirty reads or database updates,
        /// otherwise requests for dirty reads may not be honored and the read
        /// may block.
        /// </remarks>
        public bool ReadUncommitted;
        /// <summary>
        /// Physically truncate the underlying file, discarding all previous databases it might have held.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Underlying filesystem primitives are used to implement this flag.
        /// For this reason, it is applicable only to the file and cannot be
        /// used to discard databases within a file.
        /// </para>
        /// <para>
        /// This setting cannot be lock or transaction-protected, and it is an
        /// error to specify it in a locking or transaction-protected
        /// environment.
        /// </para>
        /// </remarks>
        public bool Truncate;
        /// <summary>
        /// Open the database with support for multiversion concurrency control.
        /// </summary>
        /// <remarks>
        /// This will cause updates to the database to follow a copy-on-write
        /// protocol, which is required to support snapshot isolation. This
        /// settting requires that the database be transactionally protected
        /// during its open and is not supported by the queue format.
        /// </remarks>
        public bool UseMVCC;
        internal uint openFlags {
            get {
                uint ret = 0;
                ret |= AutoCommit ? Internal.DbConstants.DB_AUTO_COMMIT : 0;
                ret |= FreeThreaded ? Internal.DbConstants.DB_THREAD : 0;
                ret |= NoMMap ? Internal.DbConstants.DB_NOMMAP : 0;
                ret |= ReadOnly ? Internal.DbConstants.DB_RDONLY : 0;
                ret |= ReadUncommitted ? Internal.DbConstants.DB_READ_UNCOMMITTED : 0;
                ret |= Truncate ? Internal.DbConstants.DB_TRUNCATE : 0;
                ret |= UseMVCC ? Internal.DbConstants.DB_MULTIVERSION : 0;
                return ret;
            }
        }

        /// <summary>
        /// Instantiate a new DatabaseConfig object
        /// </summary>
        public DatabaseConfig() {
            Env = null;
            Priority = CachePriority.DEFAULT;
            pagesizeIsSet = false;
            encryptionIsSet = false;
            ErrorPrefix = null;
            Feedback = null;
            DoChecksum = false;
            NonDurableTxns = false;
            AutoCommit = false;
            FreeThreaded = false;
            NoMMap = false;
            ReadOnly = false;
            ReadUncommitted = false;
            Truncate = false;
            UseMVCC = false;
        }
    }
}