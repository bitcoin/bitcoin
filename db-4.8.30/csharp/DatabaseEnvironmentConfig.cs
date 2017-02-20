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
    /// A class representing configuration parameters for
    /// <see cref="DatabaseEnvironment"/>
    /// </summary>
    public class DatabaseEnvironmentConfig {
        /// <summary>
        /// Create a new object, with default settings
        /// </summary>
        public DatabaseEnvironmentConfig() {
            DataDirs = new List<string>();
        }

        /// <summary>
        /// Configuration for the locking subsystem
        /// </summary>
        public LockingConfig LockSystemCfg;
        /// <summary>
        /// Configuration for the logging subsystem
        /// </summary>
        public LogConfig LogSystemCfg;
        /// <summary>
        /// Configuration for the memory pool subsystem
        /// </summary>
        public MPoolConfig MPoolSystemCfg;
        /// <summary>
        /// Configuration for the mutex subsystem
        /// </summary>
        public MutexConfig MutexSystemCfg;
        /// <summary>
        /// Configuration for the replication subsystem
        /// </summary>
        public ReplicationConfig RepSystemCfg;

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
        /// </remarks>
        public ErrorFeedbackDelegate ErrorFeedback;
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
        public EnvironmentFeedbackDelegate Feedback;
        /// <summary>
        /// A delegate which is called to notify the process of specific
        /// Berkeley DB events. 
        /// </summary>
        public EventNotifyDelegate EventNotify;
        /// <summary>
        /// A delegate that returns a unique identifier pair for the current 
        /// thread of control.
        /// </summary>
        /// <remarks>
        /// This delegate supports <see cref="DatabaseEnvironment.FailCheck"/>.
        /// For more information, see Architecting Data Store and Concurrent
        /// Data Store applications, and Architecting Transactional Data Store
        /// applications, both in the Berkeley DB Programmer's Reference Guide.
        /// </remarks>
        public SetThreadIDDelegate SetThreadID;
        /// <summary>
        /// A delegate that formats a process ID and thread ID identifier pair. 
        /// </summary>
        public SetThreadNameDelegate ThreadName;
        /// <summary>
        /// A delegate that returns if a thread of control (either a true thread
        /// or a process) is still running.
        /// </summary>
        public ThreadIsAliveDelegate ThreadIsAlive;

        /// <summary>
        /// Paths of directories to be used as the location of the access method
        /// database files.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Paths specified to <see cref="Database.Open"/> will be searched
        /// relative to this path. Paths set using this method are additive, and
        /// specifying more than one will result in each specified directory
        /// being searched for database files.
        /// </para>
        /// <para>
        /// If no database directories are specified, database files must be
        /// named either by absolute paths or relative to the environment home
        /// directory. See Berkeley DB File Naming in the Programmer's Reference
        /// Guide for more information.
        /// </para>
        /// </remarks>
        public List<string> DataDirs;
        /// <summary>
        /// The path of a directory to be used as the location to create the
        /// access method database files. When <see cref="BTreeDatabase.Open"/>,
        /// <see cref="HashDatabase.Open"/>, <see cref="QueueDatabase.Open"/> or
        /// <see cref="RecnoDatabase.Open"/> is used to create a file it will be
        /// created relative to this path.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This path must also exist in <see cref="DataDirs"/>.
        /// </para>
        /// <para>
        /// If no database directory is specified, database files must be named
        /// either by absolute paths or relative to the environment home 
        /// directory. See Berkeley DB File Naming in the Programmer's Reference
        /// Guide for more information.
        /// </para>
        /// </remarks>
        public string CreationDir;

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
        /// <remarks>
        /// <para>
        /// For databases opened inside of a DatabaseEnvironment, setting
        /// ErrorPrefix affects the entire environment and is equivalent to
        /// setting <see cref="DatabaseEnvironment.ErrorPrefix"/>.
        /// </para>
        /// </remarks>
        public string ErrorPrefix;
        /// <summary>
        /// The permissions for any intermediate directories created by Berkeley
        /// DB.
        /// </summary>
        /// <remarks>
        /// <para>
        /// By default, Berkeley DB does not create intermediate directories
        /// needed for recovery, that is, if the file /a/b/c/mydatabase is being
        /// recovered, and the directory path b/c does not exist, recovery will
        /// fail. This default behavior is because Berkeley DB does not know
        /// what permissions are appropriate for intermediate directory
        /// creation, and creating the directory might result in a security
        /// problem.
        /// </para>
        /// <para>
        /// Directory permissions are interpreted as a string of nine
        /// characters, using the character set r (read), w (write), x (execute
        /// or search), and - (none). The first character is the read
        /// permissions for the directory owner (set to either r or -). The
        /// second character is the write permissions for the directory owner
        /// (set to either w or -). The third character is the execute
        /// permissions for the directory owner (set to either x or -).
        /// </para>
        /// <para>
        /// Similarly, the second set of three characters are the read, write
        /// and execute/search permissions for the directory group, and the
        /// third set of three characters are the read, write and execute/search
        /// permissions for all others. For example, the string rwx------ would
        /// configure read, write and execute/search access for the owner only.
        /// The string rwxrwx--- would configure read, write and execute/search
        /// access for both the owner and the group. The string rwxr----- would
        /// configure read, write and execute/search access for the directory
        /// owner and read-only access for the directory group.
        /// </para>
        /// </remarks>
        public string IntermediateDirMode;
        private bool IsRPCClient { get { return false; } }

        internal bool lckTimeoutIsSet;
        private uint lckTimeout;
        /// <summary>
        /// A value, in microseconds, representing lock timeouts.
        /// </summary>
        /// <remarks>
        /// <para>
        /// All timeouts are checked whenever a thread of control blocks on a
        /// lock or when deadlock detection is performed. As timeouts are only
        /// checked when the lock request first blocks or when deadlock
        /// detection is performed, the accuracy of the timeout depends on how
        /// often deadlock detection is performed.
        /// </para>
        /// <para>
        /// Timeout values specified for the database environment may be
        /// overridden on a per-transaction basis, see
        /// <see cref="Transaction.SetLockTimeout"/>.
        /// </para>
        /// </remarks>
        public uint LockTimeout {
            get { return lckTimeout; }
            set {
                lckTimeoutIsSet = true;
                lckTimeout = value;
            }
        }

        internal bool maxTxnsIsSet;
        private uint maxTxns;
        /// <summary>
        /// The number of active transactions supported by the environment. This
        /// value bounds the size of the memory allocated for transactions.
        /// Child transactions are counted as active until they either commit or
        /// abort.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Transactions that update multiversion databases are not freed until
        /// the last page version that the transaction created is flushed from
        /// cache. This means that applications using multi-version concurrency
        /// control may need a transaction for each page in cache, in the
        /// extreme case.
        /// </para>
        /// <para>
        /// When all of the memory available in the database environment for
        /// transactions is in use, calls to 
        /// <see cref="DatabaseEnvironment.BeginTransaction"/> will fail (until
        /// some active transactions complete). If MaxTransactions is never set,
        /// the database environment is configured to support at least 100
        /// active transactions.
        /// </para>
        /// </remarks>
        public uint MaxTransactions {
            get { return maxTxns; }
            set {
                maxTxnsIsSet = true;
                maxTxns = value;
            }
        }

        /// <summary>
        /// The path of a directory to be used as the location of temporary
        /// files.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The files created to back in-memory access method databases will be
        /// created relative to this path. These temporary files can be quite
        /// large, depending on the size of the database.
        /// </para>
        /// <para>
        /// If no directories are specified, the following alternatives are
        /// checked in the specified order. The first existing directory path is
        /// used for all temporary files.
        /// </para>
        /// <list type="number">
        /// <item>The value of the environment variable TMPDIR.</item>
        /// <item>The value of the environment variable TEMP.</item>
        /// <item>The value of the environment variable TMP.</item>
        /// <item>The value of the environment variable TempFolder.</item>
        /// <item>The value returned by the GetTempPath interface.</item>
        /// <item>The directory /var/tmp.</item>
        /// <item>The directory /usr/tmp.</item>
        /// <item>The directory /temp.</item>
        /// <item>The directory /tmp.</item>
        /// <item>The directory C:/temp.</item>
        /// <item>The directory C:/tmp.</item>
        /// </list>
        /// <para>
        /// Environment variables are only checked if
        /// <see cref="UseEnvironmentVars"/> is true.
        /// </para>
        /// </remarks>
        public string TempDir;

        internal bool threadCntIsSet;
        private uint threadCnt;
        /// <summary>
        /// An approximate number of threads in the database environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// ThreadCount must set if <see cref="DatabaseEnvironment.FailCheck"/>
        /// will be used. ThreadCount does not set the maximum number of threads
        /// but is used to determine memory sizing and the thread control block
        /// reclamation policy.
        /// </para>
        /// <para>
        /// If a process has not configured <see cref="ThreadIsAlive"/>, and
        /// then attempts to join a database environment configured for failure
        /// checking with <see cref="DatabaseEnvironment.FailCheck"/>,
        /// <see cref="SetThreadID"/>, <see cref="ThreadIsAlive"/> and
        /// ThreadCount, the program may be unable to allocate a thread control
        /// block and fail to join the environment. This is true of the
        /// standalone Berkeley DB utility programs. To avoid problems when
        /// using the standalone Berkeley DB utility programs with environments
        /// configured for failure checking, incorporate the utility's
        /// functionality directly in the application, or call 
        /// <see cref="DatabaseEnvironment.FailCheck"/> before running the
        /// utility.
        /// </para>
        /// </remarks>
        public uint ThreadCount {
            get { return threadCnt; }
            set {
                threadCntIsSet = true;
                threadCnt = value;
            }
        }

        internal bool txnTimeoutIsSet;
        private uint _txnTimeout;
        /// <summary>
        /// A value, in microseconds, representing transaction timeouts.
        /// </summary>
        /// <remarks>
        /// <para>
        /// All timeouts are checked whenever a thread of control blocks on a
        /// lock or when deadlock detection is performed. As timeouts are only
        /// checked when the lock request first blocks or when deadlock
        /// detection is performed, the accuracy of the timeout depends on how
        /// often deadlock detection is performed.
        /// </para>
        /// <para>
        /// Timeout values specified for the database environment may be
        /// overridden on a per-transaction basis, see
        /// <see cref="Transaction.SetTxnTimeout"/>.
        /// </para>
        /// </remarks>
        public uint TxnTimeout {
            get { return _txnTimeout; }
            set {
                txnTimeoutIsSet = true;
                _txnTimeout = value;
            }
        }

        internal bool txnTimestampIsSet;
        private DateTime _txnTimestamp;
        /// <summary>
        /// Recover to the time specified by timestamp rather than to the most
        /// current possible date.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Once a database environment has been upgraded to a new version of
        /// Berkeley DB involving a log format change (see Upgrading Berkeley DB
        /// installations in the Programmer's Reference Guide), it is no longer
        /// possible to recover to a specific time before that upgrade.
        /// </para>
        /// </remarks>
        public DateTime TxnTimestamp {
            get { return _txnTimestamp; }
            set {
                txnTimestampIsSet = true;
                _txnTimestamp = value;
            }
        }

        /// <summary>
        /// Specific additional informational and debugging messages in the
        /// Berkeley DB message output.
        /// </summary>
        public VerboseMessages Verbosity = new VerboseMessages();

        /* Fields for set_flags() */
        /// <summary>
        /// If true, database operations for which no explicit transaction
        /// handle was specified, and which modify databases in the database
        /// environment, will be automatically enclosed within a transaction.
        /// </summary>
        public bool AutoCommit;
        /// <summary>
        /// If true, Berkeley DB Concurrent Data Store applications will perform
        /// locking on an environment-wide basis rather than on a per-database
        /// basis. 
        /// </summary>
        public bool CDB_ALLDB;
        /// <summary>
        /// If true, Berkeley DB will flush database writes to the backing disk
        /// before returning from the write system call, rather than flushing
        /// database writes explicitly in a separate system call, as necessary.
        /// </summary>
        /// <remarks>
        /// This is only available on some systems (for example, systems
        /// supporting the IEEE/ANSI Std 1003.1 (POSIX) standard O_DSYNC flag,
        /// or systems supporting the Windows FILE_FLAG_WRITE_THROUGH flag).
        /// This flag may result in inaccurate file modification times and other
        /// file-level information for Berkeley DB database files. This flag
        /// will almost certainly result in a performance decrease on most
        /// systems. This flag is only applicable to certain filesysystems (for
        /// example, the Veritas VxFS filesystem), where the filesystem's
        /// support for trickling writes back to stable storage behaves badly
        /// (or more likely, has been misconfigured).
        /// </remarks>
        public bool ForceFlush;
        /// <summary>
        /// If true, Berkeley DB will page-fault shared regions into memory when
        /// initially creating or joining a Berkeley DB environment. In
        /// addition, Berkeley DB will write the shared regions when creating an
        /// environment, forcing the underlying virtual memory and filesystems
        /// to instantiate both the necessary memory and the necessary disk
        /// space. This can also avoid out-of-disk space failures later on.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In some applications, the expense of page-faulting the underlying
        /// shared memory regions can affect performance. (For example, if the
        /// page-fault occurs while holding a lock, other lock requests can
        /// convoy, and overall throughput may decrease.)
        /// </para>
        /// </remarks>
        public bool InitRegions;
        /// <summary>
        /// If true, turn off system buffering of Berkeley DB database files to
        /// avoid double caching. 
        /// </summary>
        public bool NoBuffer;
        /// <summary>
        /// If true, Berkeley DB will grant all requested mutual exclusion
        /// mutexes and database locks without regard for their actual
        /// availability. This functionality should never be used for purposes
        /// other than debugging. 
        /// </summary>
        public bool NoLocking;
        /// <summary>
        /// If true, Berkeley DB will copy read-only database files into the
        /// local cache instead of potentially mapping them into process memory
        /// (see <see cref="MPoolConfig.MMapSize"/> for further information).
        /// </summary>
        public bool NoMMap;
        /// <summary>
        /// If true, Berkeley DB will ignore any panic state in the database
        /// environment. (Database environments in a panic state normally refuse
        /// all attempts to call Berkeley DB functions, throwing
        /// <see cref="RunRecoveryException"/>. This functionality should never
        /// be used for purposes other than debugging.
        /// </summary>
        public bool NoPanic;
        /// <summary>
        /// If true, overwrite files stored in encrypted formats before deleting
        /// them.
        /// </summary>
        /// <remarks>
        /// Berkeley DB overwrites files using alternating 0xff, 0x00 and 0xff
        /// byte patterns. For file overwriting to be effective, the underlying
        /// file must be stored on a fixed-block filesystem. Systems with
        /// journaling or logging filesystems will require operating system
        /// support and probably modification of the Berkeley DB sources.
        /// </remarks>
        public bool Overwrite;
        /// <summary>
        /// If true, database calls timing out based on lock or transaction
        /// timeout values will throw <see cref="LockNotGrantedException"/>
        /// instead of <see cref="DeadlockException"/>. This allows applications
        /// to distinguish between operations which have deadlocked and
        /// operations which have exceeded their time limits.
        /// </summary>
        public bool TimeNotGranted;
        /// <summary>
        /// If true, Berkeley DB will not write or synchronously flush the log
        /// on transaction commit.
        /// </summary>
        /// <remarks>
        /// This means that transactions exhibit the ACI (atomicity,
        /// consistency, and isolation) properties, but not D (durability); that
        /// is, database integrity will be maintained, but if the application or
        /// system fails, it is possible some number of the most recently
        /// committed transactions may be undone during recovery. The number of
        /// transactions at risk is governed by how many log updates can fit
        /// into the log buffer, how often the operating system flushes dirty
        /// buffers to disk, and how often the log is checkpointed.
        /// </remarks>
        public bool TxnNoSync;
        /// <summary>
        /// If true and a lock is unavailable for any Berkeley DB operation
        /// performed in the context of a transaction, cause the operation to
        /// throw <see cref="DeadlockException"/> (or
        /// <see cref="LockNotGrantedException"/> if
        /// <see cref="TimeNotGranted"/> is set.
        /// </summary>
        public bool TxnNoWait;
        /// <summary>
        /// If true, all transactions in the environment will be started as if
        /// <see cref="TransactionConfig.Snapshot"/> were passed to
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>, and all
        /// non-transactional cursors will be opened as if
        /// <see cref="CursorConfig.SnapshotIsolation"/> were passed to
        /// <see cref="BaseDatabase.Cursor"/>.
        /// </summary>
        public bool TxnSnapshot;
        /// <summary>
        /// If true, Berkeley DB will write, but will not synchronously flush,
        /// the log on transaction commit.
        /// </summary>
        /// <remarks>
        /// This means that transactions exhibit the ACI (atomicity,
        /// consistency, and isolation) properties, but not D (durability); that
        /// is, database integrity will be maintained, but if the system fails,
        /// it is possible some number of the most recently committed
        /// transactions may be undone during recovery. The number of
        /// transactions at risk is governed by how often the system flushes
        /// dirty buffers to disk and how often the log is checkpointed.
        /// </remarks>
        public bool TxnWriteNoSync;
        /// <summary>
        /// If true, all databases in the environment will be opened as if
        /// <see cref="DatabaseConfig.UseMVCC"/> is passed to
        /// <see cref="Database.Open"/>. This flag will be ignored for queue
        /// databases for which MVCC is not supported. 
        /// </summary>
        public bool UseMVCC;
        /// <summary>
        /// If true, Berkeley DB will yield the processor immediately after each
        /// page or mutex acquisition. This functionality should never be used
        /// for purposes other than stress testing. 
        /// </summary>
        public bool YieldCPU;
        internal uint flags {
            get {
                uint ret = 0;
                ret |= AutoCommit ? DbConstants.DB_AUTO_COMMIT : 0;
                ret |= CDB_ALLDB ? DbConstants.DB_CDB_ALLDB : 0;
                ret |= ForceFlush ? DbConstants.DB_DSYNC_DB : 0;
                ret |= InitRegions ? DbConstants.DB_REGION_INIT : 0;
                ret |= NoBuffer ? DbConstants.DB_DIRECT_DB : 0;
                ret |= NoLocking ? DbConstants.DB_NOLOCKING : 0;
                ret |= NoMMap ? DbConstants.DB_NOMMAP : 0;
                ret |= NoPanic ? DbConstants.DB_NOPANIC : 0;
                ret |= Overwrite ? DbConstants.DB_OVERWRITE : 0;
                ret |= TimeNotGranted ? DbConstants.DB_TIME_NOTGRANTED : 0;
                ret |= TxnNoSync ? DbConstants.DB_TXN_NOSYNC : 0;
                ret |= TxnNoWait ? DbConstants.DB_TXN_NOWAIT : 0;
                ret |= TxnSnapshot ? DbConstants.DB_TXN_SNAPSHOT : 0;
                ret |= TxnWriteNoSync ? DbConstants.DB_TXN_WRITE_NOSYNC : 0;
                ret |= UseMVCC ? DbConstants.DB_MULTIVERSION : 0;
                ret |= YieldCPU ? DbConstants.DB_YIELDCPU : 0;
                return ret;
            }
        }

        /* Fields for open() flags */
        /// <summary>
        /// If true, Berkeley DB subsystems will create any underlying files, as
        /// necessary.
        /// </summary>
        public bool Create;
        /// <summary>
        /// If true, the created <see cref="DatabaseEnvironment"/> object will
        /// be free-threaded; that is, concurrently usable by multiple threads
        /// in the address space.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Required to be true if the created <see cref="DatabaseEnvironment"/>
        /// object will be concurrently used by more than one thread in the
        /// process, or if any <see cref="Database"/> objects opened in the
        /// scope of the <see cref="DatabaseEnvironment"/> object will be
        /// concurrently used by more than one thread in the process.
        /// </para>
        /// <para>Required to be true when using the Replication Manager.</para>
        /// </remarks>
        public bool FreeThreaded;
        /// <summary>
        /// If true, lock shared Berkeley DB environment files and memory-mapped
        /// databases into memory.
        /// </summary>
        public bool Lockdown;
        /// <summary>
        /// If true, allocate region memory from the heap instead of from memory
        /// backed by the filesystem or system shared memory. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// This setting implies the environment will only be accessed by a
        /// single process (although that process may be multithreaded). This
        /// flag has two effects on the Berkeley DB environment. First, all
        /// underlying data structures are allocated from per-process memory
        /// instead of from shared memory that is accessible to more than a
        /// single process. Second, mutexes are only configured to work between
        /// threads.
        /// </para>
        /// <para>
        /// This setting should be false if more than a single process is
        /// accessing the environment because it is likely to cause database
        /// corruption and unpredictable behavior. For example, if both a server
        /// application and Berkeley DB utilities (for example, db_archive,
        /// db_checkpoint or db_stat) are expected to access the environment,
        /// this setting should be false.
        /// </para>
        /// </remarks>
        public bool Private;
        /// <summary>
        /// If true, check to see if recovery needs to be performed before
        /// opening the database environment. (For this check to be accurate,
        /// all processes using the environment must specify it when opening the
        /// environment.)
        /// </summary>
        /// <remarks>
        /// If recovery needs to be performed for any reason (including the
        /// initial use of this setting), and <see cref="RunRecovery"/>is also
        /// specified, recovery will be performed and the open will proceed
        /// normally. If recovery needs to be performed and
        /// <see cref="RunRecovery"/> is not specified,
        /// <see cref="RunRecoveryException"/> will be thrown. If recovery does
        /// not need to be performed, <see cref="RunRecovery"/> will be ignored.
        /// See Architecting Transactional Data Store applications in the 
        /// Programmer's Reference Guide for more information.
        /// </remarks>
        public bool Register;
        /// <summary>
        /// If true, catastrophic recovery will be run on this environment
        /// before opening it for normal use.
        /// </summary>
        /// <remarks>
        /// If true, the <see cref="Create"/> and <see cref="UseTxns"/> must
        /// also be set, because the regions will be removed and re-created,
        /// and transactions are required for application recovery.
        /// </remarks>
        public bool RunFatalRecovery;
        /// <summary>
        /// If true, normal recovery will be run on this environment before
        /// opening it for normal use.
        /// </summary>
        /// <remarks>
        /// If true, the <see cref="Create"/> and <see cref="UseTxns"/> must
        /// also be set, because the regions will be removed and re-created,
        /// and transactions are required for application recovery.
        /// </remarks>
        public bool RunRecovery;
        /// <summary>
        /// If true, allocate region memory from system shared memory instead of
        /// from heap memory or memory backed by the filesystem. 
        /// </summary>
        /// <remarks>
        /// See Shared Memory Regions in the Programmer's Reference Guide for
        /// more information.
        /// </remarks>
        public bool SystemMemory;
        /// <summary>
        /// If true, the Berkeley DB process' environment may be permitted to
        /// specify information to be used when naming files.
        /// </summary>
        /// <remarks>
        /// <para>
        /// See Berkeley DB File Naming in the Programmer's Reference Guide for 
        /// more information.
        /// </para>
        /// <para>
        /// Because permitting users to specify which files are used can create
        /// security problems, environment information will be used in file 
        /// naming for all users only if UseEnvironmentVars is true.
        /// </para>
        /// </remarks>
        public bool UseEnvironmentVars;
        private bool USE_ENVIRON_ROOT;
        /// <summary>
        /// If true, initialize locking for the Berkeley DB Concurrent Data
        /// Store product.
        /// </summary>
        /// <remarks>
        /// In this mode, Berkeley DB provides multiple reader/single writer
        /// access. The only other subsystem that should be specified with
        /// UseCDB flag is <see cref="UseMPool"/>.
        /// </remarks>
        public bool UseCDB;
        /// <summary>
        /// If true, initialize the locking subsystem.
        /// </summary>
        /// <remarks>
        /// This subsystem should be used when multiple processes or threads are
        /// going to be reading and writing a Berkeley DB database, so that they
        /// do not interfere with each other. If all threads are accessing the
        /// database(s) read-only, locking is unnecessary. When UseLocking is
        /// specified, it is usually necessary to run a deadlock detector, as
        /// well. See <see cref="DatabaseEnvironment.DetectDeadlocks"/> for more
        /// information.
        /// </remarks>
        public bool UseLocking;
        /// <summary>
        /// If true, initialize the logging subsystem.
        /// </summary>
        /// <remarks>
        /// This subsystem should be used when recovery from application or
        /// system failure is necessary. If the log region is being created and
        /// log files are already present, the log files are reviewed;
        /// subsequent log writes are appended to the end of the log, rather
        /// than overwriting current log entries.
        /// </remarks>
        public bool UseLogging;
        /// <summary>
        /// If true, initialize the shared memory buffer pool subsystem.
        /// </summary>
        /// <remarks>
        /// This subsystem should be used whenever an application is using any
        /// Berkeley DB access method.
        /// </remarks>
        public bool UseMPool;
        /// <summary>
        /// If true, initialize the replication subsystem.
        /// </summary>
        /// <remarks>
        /// This subsystem should be used whenever an application plans on using
        /// replication. UseReplication requires <see cref="UseTxns"/> and
        /// <see cref="UseLocking"/> also be set.
        /// </remarks>
        public bool UseReplication;
        /// <summary>
        /// If true, initialize the transaction subsystem.
        /// </summary>
        /// <remarks>
        /// This subsystem should be used when recovery and atomicity of
        /// multiple operations are important. UseTxns implies
        /// <see cref="UseLogging"/>.
        /// </remarks>
        public bool UseTxns;
        internal uint openFlags {
            get {
                uint ret = 0;
                ret |= Create ? DbConstants.DB_CREATE : 0;
                ret |= FreeThreaded ? DbConstants.DB_THREAD : 0;
                ret |= Lockdown ? DbConstants.DB_LOCKDOWN : 0;
                ret |= Private ? DbConstants.DB_PRIVATE : 0;
                ret |= RunFatalRecovery ? DbConstants.DB_RECOVER_FATAL : 0;
                ret |= RunRecovery ? DbConstants.DB_RECOVER : 0;
                ret |= SystemMemory ? DbConstants.DB_SYSTEM_MEM : 0;
                ret |= UseEnvironmentVars ? DbConstants.DB_USE_ENVIRON : 0;
                ret |= USE_ENVIRON_ROOT ? DbConstants.DB_USE_ENVIRON_ROOT : 0;
                ret |= UseCDB ? DbConstants.DB_INIT_CDB : 0;
                ret |= UseLocking ? DbConstants.DB_INIT_LOCK : 0;
                ret |= UseLogging ? DbConstants.DB_INIT_LOG : 0;
                ret |= UseMPool ? DbConstants.DB_INIT_MPOOL : 0;
                ret |= UseReplication ? DbConstants.DB_INIT_REP : 0;
                ret |= UseTxns ? DbConstants.DB_INIT_TXN : 0;
                return ret;
            }
        }
    }
}