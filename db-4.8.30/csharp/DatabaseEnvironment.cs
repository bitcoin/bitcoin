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
    /// A class representing a Berkeley DB database environment - a collection
    /// including support for some or all of caching, locking, logging and
    /// transaction subsystems, as well as databases and log files.
    /// </summary>
    public class DatabaseEnvironment {
        internal DB_ENV dbenv;
        private ErrorFeedbackDelegate errFeedbackHandler;
        private EnvironmentFeedbackDelegate feedbackHandler;
        private ThreadIsAliveDelegate isAliveHandler;
        private EventNotifyDelegate notifyHandler;
        private ReplicationTransportDelegate transportHandler;
        private SetThreadIDDelegate threadIDHandler;
        private SetThreadNameDelegate threadNameHandler;
        private string _pfx;
        private DBTCopyDelegate CopyDelegate;
        private BDB_ErrcallDelegate doErrFeedbackRef;
        private BDB_EnvFeedbackDelegate doFeedbackRef;
        private BDB_EventNotifyDelegate doNotifyRef;
        private BDB_IsAliveDelegate doIsAliveRef;
        private BDB_RepTransportDelegate doRepTransportRef;
        private BDB_ThreadIDDelegate doThreadIDRef;
        private BDB_ThreadNameDelegate doThreadNameRef;

        #region Callbacks
        private static void doNotify(IntPtr env, uint eventcode, byte[] event_info) {
            DB_ENV dbenv = new DB_ENV(env, false);
            
            dbenv.api2_internal.notifyHandler(
                (NotificationEvent)eventcode, event_info);
        }
        private static void doErrFeedback(IntPtr env, string pfx, string msg) {
            DB_ENV dbenv = new DB_ENV(env, false);
            dbenv.api2_internal.errFeedbackHandler(
                dbenv.api2_internal._pfx, msg);
        }
        private static void doFeedback(IntPtr env, int opcode, int percent) {
            DB_ENV dbenv = new DB_ENV(env, false);
            dbenv.api2_internal.feedbackHandler(
                (EnvironmentFeedbackEvent)opcode, percent);
        }
        private static int doIsAlive(IntPtr env, int pid, uint tid, uint flags) {
            DB_ENV dbenv = new DB_ENV(env, false);
            DbThreadID id = new DbThreadID(pid, tid);
            bool procOnly = (flags == DbConstants.DB_MUTEX_PROCESS_ONLY);
            return dbenv.api2_internal.isAliveHandler(id, procOnly) ? 1 : 0;
        }
        private static int doRepTransport(IntPtr envp,
            IntPtr controlp, IntPtr recp, IntPtr lsnp, int envid, uint flags) {
            DB_ENV dbenv = new DB_ENV(envp, false);
            DBT control = new DBT(controlp, false);
            DBT rec = new DBT(recp, false);
            DB_LSN tmplsn = new DB_LSN(lsnp, false);
            LSN dblsn = new LSN(tmplsn.file, tmplsn.offset);
            return dbenv.api2_internal.transportHandler(
                DatabaseEntry.fromDBT(control),
                DatabaseEntry.fromDBT(rec), dblsn, envid, flags);
        }
        private static void doThreadID(IntPtr env, IntPtr pid, IntPtr tid) {
            DB_ENV dbenv = new DB_ENV(env, false);
            DbThreadID id = dbenv.api2_internal.threadIDHandler();

            /* 
             * Sometimes the library doesn't care about either pid or tid 
             * (usually tid) and will pass NULL instead of a valid pointer.
             */
            if (pid != IntPtr.Zero)
                Marshal.WriteInt32(pid, id.processID);
            if (tid != IntPtr.Zero)
                Marshal.WriteInt32(tid, (int)id.threadID);
        }
        private static string doThreadName(IntPtr env,
            int pid, uint tid, ref string buf) {
            DB_ENV dbenv = new DB_ENV(env, false);
            DbThreadID id = new DbThreadID(pid, tid);
            string ret = 
                dbenv.api2_internal.threadNameHandler(id);
            try {
                buf = ret;
            } catch (NullReferenceException) {
                /* 
                 * The library may give us a NULL pointer in buf and there's no
                 * good way to test for that.  Just ignore the exception if
                 * we're not able to set buf.
                 */
            }
            return ret;
        }
        
        #endregion Callbacks
        private DatabaseEnvironment(uint flags) {
            dbenv = new DB_ENV(flags);
            initialize();
        }

        /* Called by Databases with private environments. */
        internal DatabaseEnvironment(DB_ENV dbenvp) {
            dbenv = dbenvp;
            initialize();
        }

        private void initialize() {
            dbenv.api2_internal = this;
            CopyDelegate = new DBTCopyDelegate(DatabaseEntry.dbt_usercopy);
            dbenv.set_usercopy(CopyDelegate);
        }

        private void Config(DatabaseEnvironmentConfig cfg) {
            //Alpha by dbenv function call
            foreach (string dirname in cfg.DataDirs)
                dbenv.add_data_dir(dirname);
            if (cfg.CreationDir != null)
                dbenv.set_create_dir(cfg.CreationDir);
            if (cfg.encryptionIsSet)
                dbenv.set_encrypt(
                    cfg.EncryptionPassword, (uint)cfg.EncryptAlgorithm);
            if (cfg.ErrorFeedback != null)
                ErrorFeedback = cfg.ErrorFeedback;
            ErrorPrefix = cfg.ErrorPrefix;
            if (cfg.EventNotify != null)
                EventNotify = cfg.EventNotify;
            if (cfg.Feedback != null)
                Feedback = cfg.Feedback;
            if (cfg.IntermediateDirMode != null)
                IntermediateDirMode = cfg.IntermediateDirMode;
            if (cfg.ThreadIsAlive != null)
                ThreadIsAlive = cfg.ThreadIsAlive;
            if (cfg.threadCntIsSet)
                ThreadCount = cfg.ThreadCount;
            if (cfg.SetThreadID != null)
                SetThreadID = cfg.SetThreadID;
            if (cfg.ThreadName != null)
                threadNameHandler = cfg.ThreadName;
            if (cfg.lckTimeoutIsSet)
                LockTimeout = cfg.LockTimeout;
            if (cfg.txnTimeoutIsSet)
                TxnTimeout = cfg.TxnTimeout;
            if (cfg.TempDir != null)
                TempDir = cfg.TempDir;
            if (cfg.maxTxnsIsSet)
                MaxTransactions = cfg.MaxTransactions;
            if (cfg.txnTimestampIsSet)
                TxnTimestamp = cfg.TxnTimestamp;
            if (cfg.Verbosity != null)
                Verbosity = cfg.Verbosity;
            if (cfg.flags != 0)
                dbenv.set_flags(cfg.flags, 1);

            if (cfg.LockSystemCfg != null) {
                if (cfg.LockSystemCfg.Conflicts != null)
                    LockConflictMatrix = cfg.LockSystemCfg.Conflicts;
                if (cfg.LockSystemCfg.DeadlockResolution != null)
                    DeadlockResolution = cfg.LockSystemCfg.DeadlockResolution;
                if (cfg.LockSystemCfg.maxLockersIsSet)
                    MaxLockers = cfg.LockSystemCfg.MaxLockers;
                if (cfg.LockSystemCfg.maxLocksIsSet)
                    MaxLocks = cfg.LockSystemCfg.MaxLocks;
                if (cfg.LockSystemCfg.maxObjectsIsSet)
                    MaxObjects = cfg.LockSystemCfg.MaxObjects;
                if (cfg.LockSystemCfg.partitionsIsSet)
                    LockPartitions = cfg.LockSystemCfg.Partitions;
            }

            if (cfg.LogSystemCfg != null) {
                if (cfg.LogSystemCfg.bsizeIsSet)
                    LogBufferSize = cfg.LogSystemCfg.BufferSize;
                if (cfg.LogSystemCfg.Dir != null)
                    LogDir = cfg.LogSystemCfg.Dir;
                if (cfg.LogSystemCfg.modeIsSet)
                    LogFileMode = cfg.LogSystemCfg.FileMode;
                if (cfg.LogSystemCfg.maxSizeIsSet)
                    MaxLogFileSize = cfg.LogSystemCfg.MaxFileSize;
                if (cfg.LogSystemCfg.regionSizeIsSet)
                    LogRegionSize = cfg.LogSystemCfg.RegionSize;
                if (cfg.LogSystemCfg.ConfigFlags != 0)
                    dbenv.log_set_config(cfg.LogSystemCfg.ConfigFlags, 1);
            }

            if (cfg.MPoolSystemCfg != null) {
                if (cfg.MPoolSystemCfg.CacheSize != null)
                    CacheSize = cfg.MPoolSystemCfg.CacheSize;
                if (cfg.MPoolSystemCfg.MaxCacheSize != null)
                    MaxCacheSize = cfg.MPoolSystemCfg.MaxCacheSize;
                if (cfg.MPoolSystemCfg.maxOpenFDIsSet)
                    MaxOpenFiles = cfg.MPoolSystemCfg.MaxOpenFiles;
                if (cfg.MPoolSystemCfg.maxSeqWriteIsSet)
                    SetMaxSequentialWrites(
                        cfg.MPoolSystemCfg.MaxSequentialWrites,
                        cfg.MPoolSystemCfg.SequentialWritePause);
                if (cfg.MPoolSystemCfg.mmapSizeSet)
                    MMapSize = cfg.MPoolSystemCfg.MMapSize;
            }

            if (cfg.MutexSystemCfg != null) {
                if (cfg.MutexSystemCfg.alignmentIsSet)
                    MutexAlignment = cfg.MutexSystemCfg.Alignment;
                /*
                 * Setting max after increment ensures that the value of max
                 * will win if both max and increment are set.  This is the
                 * behavior we document in MutexConfig.
                 */
                if (cfg.MutexSystemCfg.incrementIsSet)
                    MutexIncrement = cfg.MutexSystemCfg.Increment;
                if (cfg.MutexSystemCfg.maxIsSet)
                    MaxMutexes = cfg.MutexSystemCfg.MaxMutexes;
                if (cfg.MutexSystemCfg.numTASIsSet)
                    NumTestAndSetSpins = cfg.MutexSystemCfg.NumTestAndSetSpins;
            }

            if (cfg.RepSystemCfg != null) {
                if (cfg.RepSystemCfg.ackTimeoutIsSet)
                    RepAckTimeout = cfg.RepSystemCfg.AckTimeout;
                if (cfg.RepSystemCfg.BulkTransfer)
                    RepBulkTransfer = true;
                if (cfg.RepSystemCfg.checkpointDelayIsSet)
                    RepCheckpointDelay = cfg.RepSystemCfg.CheckpointDelay;
                if (cfg.RepSystemCfg.clockskewIsSet)
                    RepSetClockskew(cfg.RepSystemCfg.ClockskewFast,
                        cfg.RepSystemCfg.ClockskewSlow);
                if (cfg.RepSystemCfg.connectionRetryIsSet)
                    RepConnectionRetry = cfg.RepSystemCfg.ConnectionRetry;
                if (cfg.RepSystemCfg.DelayClientSync)
                    RepDelayClientSync = true;
                if (cfg.RepSystemCfg.electionRetryIsSet)
                    RepElectionRetry = cfg.RepSystemCfg.ElectionRetry;
                if (cfg.RepSystemCfg.electionTimeoutIsSet)
                    RepElectionTimeout = cfg.RepSystemCfg.ElectionTimeout;
                if (cfg.RepSystemCfg.fullElectionTimeoutIsSet)
                    RepFullElectionTimeout =
                        cfg.RepSystemCfg.FullElectionTimeout;
                if (cfg.RepSystemCfg.heartbeatMonitorIsSet)
                    RepHeartbeatMonitor = cfg.RepSystemCfg.HeartbeatMonitor;
                if (cfg.RepSystemCfg.heartbeatSendIsSet)
                    RepHeartbeatSend = cfg.RepSystemCfg.HeartbeatSend;
                if (cfg.RepSystemCfg.leaseTimeoutIsSet)
                    RepLeaseTimeout = cfg.RepSystemCfg.LeaseTimeout;
                if (cfg.RepSystemCfg.NoAutoInit)
                    RepNoAutoInit = true;
                if (cfg.RepSystemCfg.NoBlocking)
                    RepNoBlocking = true;
                if (cfg.RepSystemCfg.nsitesIsSet)
                    RepNSites = cfg.RepSystemCfg.NSites;
                if (cfg.RepSystemCfg.remoteAddrs.Count > 0)
                    foreach (ReplicationHostAddress addr
                        in cfg.RepSystemCfg.remoteAddrs.Keys)
                        RepMgrAddRemoteSite(addr,
                            cfg.RepSystemCfg.remoteAddrs[addr]);
                if (cfg.RepSystemCfg.priorityIsSet)
                    RepPriority = cfg.RepSystemCfg.Priority;
                if (cfg.RepSystemCfg.RepMgrAckPolicy != null)
                    RepMgrAckPolicy = cfg.RepSystemCfg.RepMgrAckPolicy;
                if (cfg.RepSystemCfg.retransmissionRequestIsSet)
                    RepSetRetransmissionRequest(
                        cfg.RepSystemCfg.RetransmissionRequestMin,
                        cfg.RepSystemCfg.RetransmissionRequestMax);
                if (cfg.RepSystemCfg.RepMgrLocalSite != null)
                    RepMgrLocalSite = cfg.RepSystemCfg.RepMgrLocalSite;
                if (cfg.RepSystemCfg.Strict2Site)
                    RepStrict2Site = true;
                if (cfg.RepSystemCfg.transmitLimitIsSet)
                    RepSetTransmitLimit(
                        cfg.RepSystemCfg.TransmitLimitGBytes,
                        cfg.RepSystemCfg.TransmitLimitBytes);
                if (cfg.RepSystemCfg.UseMasterLeases)
                    RepUseMasterLeases = true;
            }

        }

        #region Properties
        /// <summary>
        /// If true, database operations for which no explicit transaction
        /// handle was specified, and which modify databases in the database
        /// environment, will be automatically enclosed within a transaction.
        /// </summary>
        public bool AutoCommit {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_AUTO_COMMIT) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_AUTO_COMMIT, value ? 1 : 0);
            }
        }
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
        public CacheInfo CacheSize {
            get {
                uint gb = 0;
                uint b = 0;
                int n = 0;
                dbenv.get_cachesize(ref gb, ref b, ref n);
                return new CacheInfo(gb, b, n);
            }
            set {
                if (value != null)
                dbenv.set_cachesize(
                    value.Gigabytes, value.Bytes, value.NCaches);
            }
        }
        /// <summary>
        /// If true, Berkeley DB Concurrent Data Store applications will perform
        /// locking on an environment-wide basis rather than on a per-database
        /// basis. 
        /// </summary>
        public bool CDB_ALLDB {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_CDB_ALLDB) != 0;
            }
        }
        /// <summary>
        /// If true, Berkeley DB subsystems will create any underlying files, as
        /// necessary.
        /// </summary>
        public bool Create { 
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_CREATE) != 0;
            } 
        }
        /// <summary>
        /// The array of directories where database files are stored.
        /// </summary>
        public List<string> DataDirs { get { return dbenv.get_data_dirs(); } }

        /// <summary>
        /// The deadlock detector configuration, specifying what lock request(s)
        /// should be rejected. As transactions acquire locks on behalf of a
        /// single locker ID, rejecting a lock request associated with a
        /// transaction normally requires the transaction be aborted.
        /// </summary>
        public DeadlockPolicy DeadlockResolution {
            get {
                uint mode = 0;
                dbenv.get_lk_detect(ref mode);
                return DeadlockPolicy.fromPolicy(mode);
            }
            set {
                if (value != null)
                    dbenv.set_lk_detect(value.policy);
                else
                    dbenv.set_lk_detect(DeadlockPolicy.DEFAULT.policy);
            }
        }
        /// <summary>
        /// The algorithm used by the Berkeley DB library to perform encryption
        /// and decryption. 
        /// </summary>
        public EncryptionAlgorithm EncryptAlgorithm {
            get {
                uint flags = 0;
                dbenv.get_encrypt_flags(ref flags);
                return (EncryptionAlgorithm)Enum.ToObject(
                    typeof(EncryptionAlgorithm), flags);
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
        /// </remarks>
        public ErrorFeedbackDelegate ErrorFeedback {
            get { return errFeedbackHandler; }
            set {
                if (value == null)
                    dbenv.set_errcall(null);
                else if (errFeedbackHandler == null) {
                    if (doErrFeedbackRef == null)
                        doErrFeedbackRef = new BDB_ErrcallDelegate(doErrFeedback);
                    dbenv.set_errcall(doErrFeedbackRef);
                }
                errFeedbackHandler = value;
            }
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
            get { return _pfx; }
            set { _pfx = value; }
        }

        /// <summary>
        /// A delegate which is called to notify the process of specific
        /// Berkeley DB events. 
        /// </summary>
        public EventNotifyDelegate EventNotify {
            get { return notifyHandler; }
            set {
                if (value == null)
                    dbenv.set_event_notify(null);
                else if (notifyHandler == null) {
                    if (doNotifyRef == null)
                        doNotifyRef = new BDB_EventNotifyDelegate(doNotify);
                    dbenv.set_event_notify(doNotifyRef);
                }

                notifyHandler = value;
            }
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
        public EnvironmentFeedbackDelegate Feedback {
            get { return feedbackHandler; }
            set {
                if (value == null)
                    dbenv.set_feedback(null);
                else if (feedbackHandler == null) {
                    if (doFeedbackRef == null)
                        doFeedbackRef = new BDB_EnvFeedbackDelegate(doFeedback);
                    dbenv.set_feedback(doFeedbackRef);
                }
                feedbackHandler = value;
            }
        }
        /// <summary>
        /// If true, flush database writes to the backing disk before returning
        /// from the write system call, rather than flushing database writes
        /// explicitly in a separate system call, as necessary.
        /// </summary>
        /// <remarks>
        /// This flag may result in inaccurate file modification times and other
        /// file-level information for Berkeley DB database files. This flag
        /// will almost certainly result in a performance decrease on most
        /// systems.
        /// </remarks>
        public bool ForceFlush {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_DSYNC_DB) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_DSYNC_DB, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, the object is free-threaded; that is, concurrently usable
        /// by multiple threads in the address space.
        /// </summary>
        public bool FreeThreaded { 
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_THREAD) != 0;
            }
        }
        /// <summary>
        /// The database environment home directory.
        /// </summary>
        public string Home {
            get {
                string dir = "";
                dbenv.get_home(ref dir);
                return dir;
            }
        }
        /// <summary>
        /// If true, Berkeley DB will page-fault shared regions into memory when
        /// initially creating or joining a Berkeley DB environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// In some applications, the expense of page-faulting the underlying
        /// shared memory regions can affect performance. (For example, if the
        /// page-fault occurs while holding a lock, other lock requests can
        /// convoy, and overall throughput may decrease.)
        /// </para>
        /// <para>
        /// In addition to page-faulting, Berkeley DB will write the shared
        /// regions when creating an environment, forcing the underlying virtual
        /// memory and filesystems to instantiate both the necessary memory and
        /// the necessary disk space. This can also avoid out-of-disk space
        /// failures later on.
        /// </para>
        /// </remarks>
        public bool InitRegions {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_REGION_INIT) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_REGION_INIT, value ? 1 : 0);
            }
        }
        /// <summary>
        /// The intermediate directory permissions. 
        /// </summary>
        public string IntermediateDirMode {
            get {
                string ret = "";
                dbenv.get_intermediate_dir_mode(ref ret);
                return ret;
            }
            private set {
                dbenv.set_intermediate_dir_mode(value);
            }
        }
        
        /// <summary>
        /// The current lock conflicts array.
        /// </summary>
        public byte[,] LockConflictMatrix {
            get {
                int sz = 0;
                dbenv.get_lk_conflicts_nmodes(ref sz);
                byte [,] ret = new byte[sz, sz];
                dbenv.get_lk_conflicts(ret);
                return ret;
            }
            private set {
                // Matrix dimensions checked in LockingConfig.
                dbenv.set_lk_conflicts(value, (int)Math.Sqrt(value.Length));
            }
        }
        /// <summary>
        /// If true, lock shared Berkeley DB environment files and memory-mapped
        /// databases into memory.
        /// </summary>
        public bool Lockdown { 
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_LOCKDOWN) != 0;
            }
        }
        /// <summary>
        /// The number of lock table partitions used in the Berkeley DB
        /// environment.
        /// </summary>
        public uint LockPartitions {
            get {
                uint ret = 0;
                dbenv.get_lk_partitions(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lk_partitions(value);
            }
        }
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
            get {
                uint timeout = 0;
                dbenv.get_timeout(ref timeout, DbConstants.DB_SET_LOCK_TIMEOUT);
                return timeout;
            }
            set {
                dbenv.set_timeout(value, DbConstants.DB_SET_LOCK_TIMEOUT);
            }
        }
        /// <summary>
        /// The size of the in-memory log buffer, in bytes
        /// </summary>
        public uint LogBufferSize {
            get {
                uint ret = 0;
                dbenv.get_lg_bsize(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lg_bsize(value);
            }
        }
        /// <summary>
        /// The path of a directory to be used as the location of logging files.
        /// Log files created by the Log Manager subsystem will be created in
        /// this directory. 
        /// </summary>
        public string LogDir {
            get {
                string ret = "";
                dbenv.get_lg_dir(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lg_dir(value);
            }
        }
        /// <summary>
        /// The absolute file mode for created log files. This property is only
        /// useful for the rare Berkeley DB application that does not control
        /// its umask value.
        /// </summary>
        /// <remarks>
        /// Normally, if Berkeley DB applications set their umask appropriately,
        /// all processes in the application suite will have read permission on
        /// the log files created by any process in the application suite.
        /// However, if the Berkeley DB application is a library, a process
        /// using the library might set its umask to a value preventing other
        /// processes in the application suite from reading the log files it
        /// creates. In this rare case, this property can be used to set the
        /// mode of created log files to an absolute value.
        /// </remarks>
        public int LogFileMode {
            get {
                int ret = 0;
                dbenv.get_lg_filemode(ref ret);
                return ret;
            }
            set {
                dbenv.set_lg_filemode(value);
            }
        }
        /// <summary>
        /// If true, system buffering is turned off for Berkeley DB log files to
        /// avoid double caching. 
        /// </summary>
        public bool LogNoBuffer {
            get {
                int onoff = 0;
                dbenv.log_get_config(DbConstants.DB_LOG_DIRECT, ref onoff);
                return (onoff != 0);
            }
            set {
                dbenv.log_set_config(DbConstants.DB_LOG_DIRECT, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, Berkeley DB will flush log writes to the backing disk
        /// before returning from the write system call, rather than flushing
        /// log writes explicitly in a separate system call, as necessary.
        /// </summary>
        public bool LogForceSync {
            get{
                int onoff = 0;
                dbenv.log_get_config(DbConstants.DB_LOG_DSYNC, ref onoff);
                return (onoff != 0);
            }
            set {
                dbenv.log_set_config(DbConstants.DB_LOG_DSYNC, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, Berkeley DB will automatically remove log files that are no
        /// longer needed.
        /// </summary>
        public bool LogAutoRemove {
            get {
                int onoff = 0;
                dbenv.log_get_config(DbConstants.DB_LOG_AUTO_REMOVE, ref onoff);
                return (onoff != 0);
            }
            set {
                dbenv.log_set_config(
                    DbConstants.DB_LOG_AUTO_REMOVE, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, transaction logs are maintained in memory rather than on
        /// disk. This means that transactions exhibit the ACI (atomicity,
        /// consistency, and isolation) properties, but not D (durability).
        /// </summary>
        public bool LogInMemory {
            get {
                int onoff = 0;
                dbenv.log_get_config(DbConstants.DB_LOG_IN_MEMORY, ref onoff);
                return (onoff != 0);
            }
        }
        /// <summary>
        /// If true, all pages of a log file are zeroed when that log file is
        /// created.
        /// </summary>
        public bool LogZeroOnCreate {
            get {
                int onoff = 0;
                dbenv.log_get_config(DbConstants.DB_LOG_ZERO, ref onoff);
                return (onoff != 0);
            }
        }
        /// <summary>
        /// The size of the underlying logging area of the Berkeley DB
        /// environment, in bytes.
        /// </summary>
        public uint LogRegionSize {
            get {
                uint ret = 0;
                dbenv.get_lg_regionmax(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lg_regionmax(value);
            }
        }
        /// <summary>
        /// The maximum cache size
        /// </summary>
        public CacheInfo MaxCacheSize {
            get {
                uint gb = 0;
                uint b = 0;
                dbenv.get_cache_max(ref gb, ref b);
                return new CacheInfo(gb, b, 0);
            }
            private set {
                dbenv.set_cache_max(value.Gigabytes, value.Bytes);
            }
        }
        /// <summary>
        /// The maximum size of a single file in the log, in bytes. Because
        /// <see cref="LSN.Offset">LSN Offsets</see> are unsigned four-byte
        /// values, the size may not be larger than the maximum unsigned
        /// four-byte value.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When the logging subsystem is configured for on-disk logging, the
        /// default size of a log file is 10MB.
        /// </para>
        /// <para>
        /// When the logging subsystem is configured for in-memory logging, the
        /// default size of a log file is 256KB. In addition, the configured log
        /// buffer size must be larger than the log file size. (The logging
        /// subsystem divides memory configured for in-memory log records into
        /// "files", as database environments configured for in-memory log
        /// records may exchange log records with other members of a replication
        /// group, and those members may be configured to store log records
        /// on-disk.) When choosing log buffer and file sizes for in-memory
        /// logs, applications should ensure the in-memory log buffer size is
        /// large enough that no transaction will ever span the entire buffer,
        /// and avoid a state where the in-memory buffer is full and no space
        /// can be freed because a transaction that started in the first log
        /// "file" is still active.
        /// </para>
        /// <para>
        /// See Log File Limits in the Programmer's Reference Guide for more
        /// information.
        /// </para>
        /// <para>
        /// If no size is specified by the application, the size last specified
        /// for the database region will be used, or if no database region
        /// previously existed, the default will be used.
        /// </para>
        /// </remarks>
        public uint MaxLogFileSize {
            get {
                uint ret = 0;
                dbenv.get_lg_max(ref ret);
                return ret;
            }
            set {
                dbenv.set_lg_max(value);
            }
        }
        /// <summary>
        /// The maximum number of locking entities supported by the Berkeley DB
        /// environment.
        /// </summary>
        public uint MaxLockers {
            get {
                uint ret = 0;
                dbenv.get_lk_max_lockers(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lk_max_lockers(value);
            }
        }
        /// <summary>
        /// The maximum number of locks supported by the Berkeley DB
        /// environment.
        /// </summary>
        public uint MaxLocks {
            get {
                uint ret = 0;
                dbenv.get_lk_max_locks(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lk_max_locks(value);
            }
        }
        /// <summary>
        /// The total number of mutexes allocated
        /// </summary>
        public uint MaxMutexes {
            get {
                uint ret = 0;
                dbenv.mutex_get_max(ref ret);
                return ret;
            }
            private set {
                dbenv.mutex_set_max(value);
            }
        }
        /// <summary>
        /// The maximum number of locked objects
        /// </summary>
        public uint MaxObjects {
            get {
                uint ret = 0;
                dbenv.get_lk_max_objects(ref ret);
                return ret;
            }
            private set {
                dbenv.set_lk_max_objects(value);
            }
        }
        /// <summary>
        /// The number of file descriptors the library will open concurrently
        /// when flushing dirty pages from the cache.
        /// </summary>
        public int MaxOpenFiles {
            get {
                int ret = 0;
                dbenv.get_mp_max_openfd(ref ret);
                return ret;
            }
            set {
                dbenv.set_mp_max_openfd(value);
            }
        }
        /// <summary>
        /// The number of sequential write operations scheduled by the library
        /// when flushing dirty pages from the cache. 
        /// </summary>
        public int MaxSequentialWrites {
            get {
                int ret = 0;
                uint tmp = 0;
                dbenv.get_mp_max_write(ref ret, ref tmp);
                return ret;
            }
        }
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
        /// transactions is in use, calls to <see cref="BeginTransaction"/> will
        /// fail (until some active transactions complete). If MaxTransactions
        /// is never set, the database environment is configured to support at
        /// least 100 active transactions.
        /// </para>
        /// </remarks>
        public uint MaxTransactions {
            get {
                uint ret = 0;
                dbenv.get_tx_max(ref ret);
                return ret;
            }
            set {
                dbenv.set_tx_max(value);
            }
        }
        /// <summary>
        /// The maximum file size, in bytes, for a file to be mapped into the
        /// process address space. If no value is specified, it defaults to
        /// 10MB. 
        /// </summary>
        /// <remarks>
        /// Files that are opened read-only in the cache (and that satisfy a few
        /// other criteria) are, by default, mapped into the process address
        /// space instead of being copied into the local cache. This can result
        /// in better-than-usual performance because available virtual memory is
        /// normally much larger than the local cache, and page faults are
        /// faster than page copying on many systems. However, it can cause
        /// resource starvation in the presence of limited virtual memory, and
        /// it can result in immense process sizes in the presence of large
        /// databases.
        /// </remarks>
        public uint MMapSize {
            get {
                uint ret = 0;
                dbenv.get_mp_mmapsize(ref ret);
                return ret;
            }
            set {
                dbenv.set_mp_mmapsize(value);
            }
        }
        /// <summary>
        /// The mutex alignment, in bytes.
        /// </summary>
        public uint MutexAlignment {
            get {
                uint ret = 0;
                dbenv.mutex_get_align(ref ret);
                return ret;
            }
            private set {
                dbenv.mutex_set_align(value);
            }
        }
        /// <summary>
        /// The number of additional mutexes allocated.
        /// </summary>
        public uint MutexIncrement {
            get {
                uint ret = 0;
                dbenv.mutex_get_increment(ref ret);
                return ret;
            }
            private set {
                dbenv.mutex_set_increment(value);
            }
        }
        /// <summary>
        /// If true, turn off system buffering of Berkeley DB database files to
        /// avoid double caching. 
        /// </summary>
        public bool NoBuffer {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_DIRECT_DB) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_DIRECT_DB, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, Berkeley DB will grant all requested mutual exclusion
        /// mutexes and database locks without regard for their actual
        /// availability. This functionality should never be used for purposes
        /// other than debugging. 
        /// </summary>
        public bool NoLocking {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_NOLOCKING) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_NOLOCKING, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, Berkeley DB will copy read-only database files into the
        /// local cache instead of potentially mapping them into process memory.
        /// </summary>
        /// <seealso cref="MMapSize"/>
        public bool NoMMap {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_NOMMAP) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_NOMMAP, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, Berkeley DB will ignore any panic state in the database
        /// environment. (Database environments in a panic state normally refuse
        /// all attempts to call Berkeley DB functions, throwing 
        /// <see cref="RunRecoveryException"/>.) This functionality should never
        /// be used for purposes other than debugging.
        /// </summary>
        public bool NoPanic {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_NOPANIC) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_NOPANIC, value ? 1 : 0);
            }
        }
        /// <summary>
        /// The number of times that test-and-set mutexes should spin without
        /// blocking. The value defaults to 1 on uniprocessor systems and to 50
        /// times the number of processors on multiprocessor systems. 
        /// </summary>
        public uint NumTestAndSetSpins {
            get {
                uint ret = 0;
                dbenv.mutex_get_tas_spins(ref ret);
                return ret;
            }
            set {
                dbenv.mutex_set_tas_spins(value);
            }
        }
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
        public bool Overwrite {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_OVERWRITE) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_OVERWRITE, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, allocate region memory from the heap instead of from memory
        /// backed by the filesystem or system shared memory. 
        /// </summary>
        public bool Private { 
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_PRIVATE) != 0;
            }
        }
        /// <summary>
        /// If true, Berkeley DB will have checked to see if recovery needed to
        /// be performed before opening the database environment.
        /// </summary>
        public bool Register { 
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_REGISTER) != 0;
            }
        }
        /// <summary>
        /// The amount of time the replication manager's transport function
        /// waits to collect enough acknowledgments from replication group
        /// clients, before giving up and returning a failure indication. The
        /// default wait time is 1 second.
        /// </summary>
        public uint RepAckTimeout {
            get { return getRepTimeout(DbConstants.DB_REP_ACK_TIMEOUT); }
            set { 
                dbenv.rep_set_timeout(
                    DbConstants.DB_REP_ACK_TIMEOUT, value); 
            }
        }
        /// <summary>
        /// If true, the replication master sends groups of records to the
        /// clients in a single network transfer
        /// </summary>
        public bool RepBulkTransfer {
            get { return getRepConfig(DbConstants.DB_REP_CONF_BULK); }
            set { 
                dbenv.rep_set_config(
                    DbConstants.DB_REP_CONF_BULK, value ? 1 : 0);
            }
        }
        /// <summary>
        /// The amount of time a master site will delay between completing a
        /// checkpoint and writing a checkpoint record into the log.
        /// </summary>
        /// <remarks>
        /// This delay allows clients to complete their own checkpoints before
        /// the master requires completion of them. The default is 30 seconds.
        /// If all databases in the environment, and the environment's
        /// transaction log, are configured to reside in memory (never preserved
        /// to disk), then, although checkpoints are still necessary, the delay
        /// is not useful and should be set to 0.
        /// </remarks>
        public uint RepCheckpointDelay {
            get { return getRepTimeout(DbConstants.DB_REP_CHECKPOINT_DELAY); }
            set { 
                dbenv.rep_set_timeout(
                    DbConstants.DB_REP_CHECKPOINT_DELAY, value);
            }
        }
        /// <summary>
        /// The value, relative to <see cref="RepClockskewSlow"/>, of the
        /// fastest clock in the group of sites.
        /// </summary>
        public uint RepClockskewFast {
            get {
                uint fast = 0;
                uint slow = 0;
                dbenv.rep_get_clockskew(ref fast, ref slow);
                return fast;
            }
        }
        /// <summary>
        /// The value of the slowest clock in the group of sites.
        /// </summary>
        public uint RepClockskewSlow {
            get {
                uint fast = 0;
                uint slow = 0;
                dbenv.rep_get_clockskew(ref fast, ref slow);
                return slow;
            }
        }
        /// <summary>
        /// Set the clock skew ratio among replication group members based on
        /// the fastest and slowest measurements among the group for use with
        /// master leases.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Calling this method is optional, the default values for clock skew
        /// assume no skew. The user must also configure leases via
        /// <see cref="RepUseMasterLeases"/>. Additionally, the user must also
        /// set the master lease timeout via <see cref="RepLeaseTimeout"/> and
        /// the number of sites in the replication group via
        /// <see cref="RepNSites"/>. These settings may be configured in any
        /// order. For a description of the clock skew values, see Clock skew 
        /// in the Berkeley DB Programmer's Reference Guide. For a description
        /// of master leases, see Master leases in the Berkeley DB Programmer's
        /// Reference Guide.
        /// </para>
        /// <para>
        /// These arguments can be used to express either raw measurements of a
        /// clock timing experiment or a percentage across machines. For
        /// instance a group of sites have a 2% variance, then
        /// <paramref name="fast"/> should be set to 102, and
        /// <paramref name="slow"/> should be set to 100. Or, for a 0.03%
        /// difference, you can use 10003 and 10000 respectively.
        /// </para>
        /// </remarks>
        /// <param name="fast">
        /// The value, relative to <paramref name="slow"/>, of the fastest clock
        /// in the group of sites.
        /// </param>
        /// <param name="slow">
        /// The value of the slowest clock in the group of sites.
        /// </param>
        public void RepSetClockskew(uint fast, uint slow) {
            dbenv.rep_set_clockskew(fast, slow);
        }
        /// <summary>
        /// The amount of time the replication manager will wait before trying
        /// to re-establish a connection to another site after a communication
        /// failure. The default wait time is 30 seconds.
        /// </summary>
        public uint RepConnectionRetry {
            get { return getRepTimeout(DbConstants.DB_REP_CONNECTION_RETRY); }
            set { 
                dbenv.rep_set_timeout(
                    DbConstants.DB_REP_CONNECTION_RETRY, value);
            }
        }
        /// <summary>
        /// If true, the client should delay synchronizing to a newly declared
        /// master (defaults to false). Clients configured in this way will remain
        /// unsynchronized until the application calls <see cref="RepSync"/>. 
        /// </summary>
        public bool RepDelayClientSync {
            get { return getRepConfig(DbConstants.DB_REP_CONF_DELAYCLIENT); }
            set { 
                dbenv.rep_set_config(
                    DbConstants.DB_REP_CONF_DELAYCLIENT, value ? 1 : 0);
            }
        }
        /// <summary>
        /// Configure the amount of time the replication manager will wait
        /// before retrying a failed election. The default wait time is 10
        /// seconds. 
        /// </summary>
        public uint RepElectionRetry {
            get { return getRepTimeout(DbConstants.DB_REP_ELECTION_RETRY); }
            set { 
                dbenv.rep_set_timeout(DbConstants.DB_REP_ELECTION_RETRY, value);
            }
        }
        /// <summary>
        /// The timeout period for an election. The default timeout is 2
        /// seconds.
        /// </summary>
        public uint RepElectionTimeout {
            get { return getRepTimeout(DbConstants.DB_REP_ELECTION_TIMEOUT); }
            set { 
                dbenv.rep_set_timeout(
                    DbConstants.DB_REP_ELECTION_TIMEOUT, value);
            }
        }
        /// <summary>
        /// An optional configuration timeout period to wait for full election
        /// participation the first time the replication group finds a master.
        /// By default this option is turned off and normal election timeouts
        /// are used. (See the Elections section in the Berkeley DB Reference
        /// Guide for more information.) 
        /// </summary>
        public uint RepFullElectionTimeout {
            get { 
                return getRepTimeout(
                    DbConstants.DB_REP_FULL_ELECTION_TIMEOUT);
            }
            set { 
                dbenv.rep_set_timeout(
                    DbConstants.DB_REP_FULL_ELECTION_TIMEOUT, value);
            }
        }
        /// <summary>
        /// The amount of time the replication manager, running at a client
        /// site, waits for some message activity on the connection from the
        /// master (heartbeats or other messages) before concluding that the
        /// connection has been lost. When 0 (the default), no monitoring is
        /// performed.
        /// </summary>
        public uint RepHeartbeatMonitor {
            get { return getRepTimeout(DbConstants.DB_REP_HEARTBEAT_MONITOR); }
            set { 
                dbenv.rep_set_timeout(
                    DbConstants.DB_REP_HEARTBEAT_MONITOR, value);
            }
        }
        /// <summary>
        /// The frequency at which the replication manager, running at a master
        /// site, broadcasts a heartbeat message in an otherwise idle system.
        /// When 0 (the default), no heartbeat messages will be sent. 
        /// </summary>
        public uint RepHeartbeatSend {
            get { return getRepTimeout(DbConstants.DB_REP_HEARTBEAT_SEND); }
            set { 
                dbenv.rep_set_timeout(DbConstants.DB_REP_HEARTBEAT_SEND, value);
            }
        }
        /// <summary>
        /// The amount of time a client grants its master lease to a master.
        /// When using master leases all sites in a replication group must use
        /// the same lease timeout value. There is no default value. If leases
        /// are desired, this method must be called prior to calling
        /// <see cref="RepStartClient"/> or <see cref="RepStartMaster"/>.
        /// </summary>
        public uint RepLeaseTimeout {
            get { return getRepTimeout(DbConstants.DB_REP_LEASE_TIMEOUT); }
            set { 
                dbenv.rep_set_timeout(DbConstants.DB_REP_LEASE_TIMEOUT, value);
            }
        }
        /// <summary>
        /// Specify how master and client sites will handle acknowledgment of
        /// replication messages which are necessary for "permanent" records.
        /// The current implementation requires all sites in a replication group
        /// configure the same acknowledgement policy. 
        /// </summary>
        /// <seealso cref="RepAckTimeout"/>
        public AckPolicy RepMgrAckPolicy {
            get {
                int policy = 0;
                dbenv.repmgr_get_ack_policy(ref policy);
                return AckPolicy.fromInt(policy);
            }
            set { dbenv.repmgr_set_ack_policy(value.Policy); }
        }
        /// <summary>
        /// The host information for the local system. 
        /// </summary>
        public ReplicationHostAddress RepMgrLocalSite {
            set { dbenv.repmgr_set_local_site(value.Host, value.Port, 0); }
        }
        /// <summary>
        /// The status of the sites currently known by the replication manager. 
        /// </summary>
        public RepMgrSite[] RepMgrRemoteSites {
            get { return dbenv.repmgr_site_list(); }
        }
        /// <summary>
        /// If true, the replication master will not automatically re-initialize
        /// outdated clients (defaults to false). 
        /// </summary>
        public bool RepNoAutoInit {
            get { return getRepConfig(DbConstants.DB_REP_CONF_NOAUTOINIT); }
            set {
                dbenv.rep_set_config(
                    DbConstants.DB_REP_CONF_NOAUTOINIT, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, Berkeley DB method calls that would normally block while
        /// clients are in recovery will return errors immediately (defaults to
        /// false).
        /// </summary>
        public bool RepNoBlocking {
            get { return getRepConfig(DbConstants.DB_REP_CONF_NOWAIT); }
            set { 
                dbenv.rep_set_config(
                    DbConstants.DB_REP_CONF_NOWAIT, value ? 1 : 0);
            }
        }
        /// <summary>
        /// The total number of sites in the replication group.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This setting is typically used by applications which use the
        /// Berkeley DB library "replication manager" support. (However, see
        /// also <see cref="RepHoldElection"/>, the description of the nsites
        /// parameter.)
        /// </para>
        /// </remarks>
        public uint RepNSites {
            get {
                uint ret = 0;
                dbenv.rep_get_nsites(ref ret);
                return ret;
            }
            set { dbenv.rep_set_nsites(value); }
        }
        /// <summary>
        /// The database environment's priority in replication group elections.
        /// A special value of 0 indicates that this environment cannot be a
        /// replication group master. If not configured, then a default value
        /// of 100 is used.
        /// </summary>
        public uint RepPriority {
            get {
                uint ret = 0;
                dbenv.rep_get_priority(ref ret);
                return ret;
            }
            set { dbenv.rep_set_priority(value); }
        }
        /// <summary>
        /// The minimum number of microseconds a client waits before requesting
        /// retransmission.
        /// </summary>
        public uint RepRetransmissionRequestMin {
            get {
                uint min = 0;
                uint max = 0;
                dbenv.rep_get_request(ref min, ref max);
                return min;
            }
        }
        /// <summary>
        /// The maximum number of microseconds a client waits before requesting
        /// retransmission.
        /// </summary>
        public uint RepRetransmissionRequestMax {
            get {
                uint min = 0;
                uint max = 0;
                dbenv.rep_get_request(ref min, ref max);
                return max;
            }
        }
        /// <summary>
        /// Set a threshold for the minimum and maximum time that a client waits
        /// before requesting retransmission of a missing message.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If the client detects a gap in the sequence of incoming log records
        /// or database pages, Berkeley DB will wait for at least
        /// <paramref name="min"/> microseconds before requesting retransmission
        /// of the missing record. Berkeley DB will double that amount before
        /// requesting the same missing record again, and so on, up to a
        /// maximum threshold of <paramref name="max"/> microseconds.
        /// </para>
        /// <para>
        /// These values are thresholds only. Since Berkeley DB has no thread
        /// available in the library as a timer, the threshold is only checked
        /// when a thread enters the Berkeley DB library to process an incoming
        /// replication message. Any amount of time may have passed since the
        /// last message arrived and Berkeley DB only checks whether the amount
        /// of time since a request was made is beyond the threshold value or
        /// not.
        /// </para>
        /// <para>
        /// By default the minimum is 40000 and the maximum is 1280000 (1.28
        /// seconds). These defaults are fairly arbitrary and the application
        /// likely needs to adjust these. The values should be based on expected
        /// load and performance characteristics of the master and client host
        /// platforms and transport infrastructure as well as round-trip message
        /// time.
        /// </para></remarks>
        /// <param name="min">
        /// The minimum number of microseconds a client waits before requesting
        /// retransmission.
        /// </param>
        /// <param name="max">
        /// The maximum number of microseconds a client waits before requesting
        /// retransmission.
        /// </param>
        public void RepSetRetransmissionRequest(uint min, uint max) {
            dbenv.rep_set_request(min, max);
        }
        /// <summary>
        /// Replication Manager observes the strict "majority" rule in managing
        /// elections, even in a group with only 2 sites. This means the client
        /// in a 2-site group will be unable to take over as master if the
        /// original master fails or becomes disconnected. (See the Elections
        /// section in the Berkeley DB Reference Guide for more information.)
        /// Both sites in the replication group should have the same value for
        /// this parameter.
        /// </summary>
        public bool RepStrict2Site {
            get { 
                return getRepConfig(DbConstants.DB_REPMGR_CONF_2SITE_STRICT);
            }
            set {
                dbenv.rep_set_config(
                    DbConstants.DB_REPMGR_CONF_2SITE_STRICT, value ? 1 : 0);
            }
        }
        /// <summary>
        /// The gigabytes component of the byte-count limit on the amount of
        /// data that will be transmitted from a site in response to a single
        /// message processed by <see cref="RepProcessMessage"/>.
        /// </summary>
        public uint RepTransmitLimitGBytes {
            get {
                uint gb = 0;
                uint b = 0;
                dbenv.rep_get_limit(ref gb, ref b);
                return gb;
            }
        }
        /// <summary>
        /// The bytes component of the byte-count limit on the amount of data
        /// that will be transmitted from a site in response to a single
        /// message processed by <see cref="RepProcessMessage"/>.
        /// </summary>
        public uint RepTransmitLimitBytes {
            get {
                uint gb = 0;
                uint b = 0;
                dbenv.rep_get_limit(ref gb, ref b);
                return b;
            }
        }
        /// <summary>
        /// Set a byte-count limit on the amount of data that will be
        /// transmitted from a site in response to a single message processed by
        /// <see cref="RepProcessMessage"/>. The limit is not a hard limit, and
        /// the record that exceeds the limit is the last record to be sent. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Record transmission throttling is turned on by default with a limit
        /// of 10MB.
        /// </para>
        /// <para>
        /// If both <paramref name="GBytes"/> and <paramref name="Bytes"/> are
        /// zero, then the transmission limit is turned off.
        /// </para>
        /// </remarks>
        /// <param name="GBytes">
        /// The number of gigabytes which, when added to
        /// <paramref name="Bytes"/>, specifies the maximum number of bytes that
        /// will be sent in a single call to <see cref="RepProcessMessage"/>.
        /// </param>
        /// <param name="Bytes">
        /// The number of bytes which, when added to 
        /// <paramref name="GBytes"/>, specifies the maximum number of bytes
        /// that will be sent in a single call to
        /// <see cref="RepProcessMessage"/>.
        /// </param>
        public void RepSetTransmitLimit(uint GBytes, uint Bytes) {
            dbenv.rep_set_limit(GBytes, Bytes);
        }
        /// <summary>
        /// The delegate used to transmit data using the replication
        /// application's communication infrastructure.
        /// </summary>
        public ReplicationTransportDelegate RepTransport { 
            get { return transportHandler; }
        }
        /// <summary>
        /// Initialize the communication infrastructure for a database
        /// environment participating in a replicated application.
        /// </summary>
        /// <remarks>
        /// RepSetTransport is not called by most replication applications. It
        /// should only be called by applications implementing their own network
        /// transport layer, explicitly holding replication group elections and
        /// handling replication messages outside of the replication manager
        /// framework.
        /// </remarks>
        /// <param name="envid">
        /// The local environment's ID. It must be a non-negative integer and
        /// uniquely identify this Berkeley DB database environment (see
        /// Replication environment IDs in the Programmer's Reference Guide for
        /// more information).
        /// </param>
        /// <param name="transport">
        /// The delegate used to transmit data using the replication
        /// application's communication infrastructure.
        /// </param>
        public void RepSetTransport(int envid,
            ReplicationTransportDelegate transport) {
            if (transport == null)
                dbenv.rep_set_transport(envid, null);
            else if (transportHandler == null) {
                if (doRepTransportRef == null)
                    doRepTransportRef = new BDB_RepTransportDelegate(doRepTransport);
                dbenv.rep_set_transport(envid, doRepTransportRef);
            }                    
            transportHandler = transport;
        }
        /// <summary>
        /// If true, master leases will be used for this site (defaults to
        /// false). 
        /// </summary>
        /// <remarks>
        /// Configuring this option may result in a 
        /// <see cref="LeaseExpiredException"/> when attempting to read entries
        /// from a database after the site's master lease has expired.
        /// </remarks>
        public bool RepUseMasterLeases {
            get { return getRepConfig(DbConstants.DB_REP_CONF_LEASE); }
            set {
                dbenv.rep_set_config(
                    DbConstants.DB_REP_CONF_LEASE, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, catastrophic recovery was run on this environment before
        /// opening it for normal use.
        /// </summary>
        public bool RunFatalRecovery {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_RECOVER_FATAL) != 0;
            }
        }
        /// <summary>
        /// If true, normal recovery was run on this environment before opening
        /// it for normal use.
        /// </summary>
        public bool RunRecovery {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_RECOVER) != 0;
            }
        }
        /// <summary>
        /// The number of microseconds the thread of control will pause before
        /// scheduling further write operations.
        /// </summary>
        public uint SequentialWritePause {
            get {
                int tmp = 0;
                uint ret = 0;
                dbenv.get_mp_max_write(ref tmp, ref ret);
                return ret;
            }
        }
        /// <summary>
        /// A delegate that returns a unique identifier pair for the current 
        /// thread of control.
        /// </summary>
        /// <remarks>
        /// This delegate supports <see cref="FailCheck"/>. For more
        /// information, see Architecting Data Store and Concurrent Data Store
        /// applications, and Architecting Transactional Data Store
        /// applications, both in the Berkeley DB Programmer's Reference Guide.
        /// </remarks>
        public SetThreadIDDelegate SetThreadID {
            get { return threadIDHandler; }
            set {
                if (value == null)
                    dbenv.set_thread_id(null);
                else if (threadIDHandler == null) {
                    if (doThreadIDRef == null)
                        doThreadIDRef = new BDB_ThreadIDDelegate(doThreadID);
                    dbenv.set_thread_id(doThreadIDRef);
                }
                threadIDHandler = value;
            }
        }
        /// <summary>
        /// A delegate that formats a process ID and thread ID identifier pair. 
        /// </summary>
        public SetThreadNameDelegate SetThreadName {
            get { return threadNameHandler; }
            set {
                if (value == null)
                    dbenv.set_thread_id_string(null);
                else if (threadNameHandler == null) {
                    if (doThreadNameRef == null)
                        doThreadNameRef =
                            new BDB_ThreadNameDelegate(doThreadName);
                    dbenv.set_thread_id_string(doThreadNameRef);
                }
                threadNameHandler = value;
            }
        }
        /// <summary>
        /// If true, allocate region memory from system shared memory instead of
        /// from heap memory or memory backed by the filesystem. 
        /// </summary>
        public bool SystemMemory {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_SYSTEM_MEM) != 0;
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
        public string TempDir {
            get {
                string ret = "";
                dbenv.get_tmp_dir(ref ret);
                return ret;
            }
            set { dbenv.set_tmp_dir(value); }
        }
        /// <summary>
        /// An approximate number of threads in the database environment.
        /// </summary>
        public uint ThreadCount {
            get {
                uint ret = 0;
                dbenv.get_thread_count(ref ret);
                return ret;
            }
            private set { dbenv.set_thread_count(value); }
        }
        /// <summary>
        /// A delegate that returns if a thread of control (either a true thread
        /// or a process) is still running.
        /// </summary>
        public ThreadIsAliveDelegate ThreadIsAlive {
            get { return isAliveHandler; }
            set {
                if (value == null)
                    dbenv.set_isalive(null);
                else if (isAliveHandler == null) {
                    if (doIsAliveRef == null)
                        doIsAliveRef = new BDB_IsAliveDelegate(doIsAlive);
                    dbenv.set_isalive(doIsAliveRef);
                }
                isAliveHandler = value;
            }
        }
        /// <summary>
        /// If true, database calls timing out based on lock or transaction
        /// timeout values will throw <see cref="LockNotGrantedException"/>
        /// instead of <see cref="DeadlockException"/>.
        /// </summary>
        /// <remarks>
        /// If true, this allows applications to distinguish between operations
        /// which have deadlocked and operations which have exceeded their time
        /// limits.
        /// </remarks>
        public bool TimeNotGranted {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_TIME_NOTGRANTED) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_TIME_NOTGRANTED, value ? 1 : 0);
            }
        }
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
        public bool TxnNoSync {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_TXN_NOSYNC) != 0;
            }
            set { dbenv.set_flags(DbConstants.DB_TXN_NOSYNC, value ? 1 : 0); }
        }
        /// <summary>
        /// If true and a lock is unavailable for any Berkeley DB operation
        /// performed in the context of a transaction, cause the operation to 
        /// throw <see cref="DeadlockException"/> (or
        /// <see cref="LockNotGrantedException"/> if configured with
        /// <see cref="TimeNotGranted"/>).
        /// </summary>
        public bool TxnNoWait {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_TXN_NOWAIT) != 0;
            }
            set { dbenv.set_flags(DbConstants.DB_TXN_NOWAIT, value ? 1 : 0); }
        }
        /// <summary>
        /// If true, all transactions in the environment will be started as if
        /// <see cref="TransactionConfig.Snapshot"/> was passed to
        /// <see cref="BeginTransaction"/>, and all non-transactional cursors
        /// will be opened as if <see cref="CursorConfig.SnapshotIsolation"/>
        /// was passed to <see cref="BaseDatabase.Cursor"/>.
        /// </summary>
        public bool TxnSnapshot {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_TXN_SNAPSHOT) != 0;
            }
            set { dbenv.set_flags(DbConstants.DB_TXN_SNAPSHOT, value ? 1 : 0); }
        }
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
            get {
                uint timeout = 0;
                dbenv.get_timeout(ref timeout, DbConstants.DB_SET_TXN_TIMEOUT);
                return timeout;
            }
            set {
                dbenv.set_timeout(value, DbConstants.DB_SET_TXN_TIMEOUT);
            }
        }
        /// <summary>
        /// The recovery timestamp
        /// </summary>
        public DateTime TxnTimestamp {
            get {
                long secs = 0;
                dbenv.get_tx_timestamp(ref secs);
                DateTime epoch = new DateTime(1970, 1, 1);
                DateTime ret = epoch.AddSeconds(secs);
                return ret;
            }
            private set {
                if (value != null) {
                    TimeSpan ts = value - new DateTime(1970, 1, 1);
                    long secs = (long)ts.TotalSeconds;
                    dbenv.set_tx_timestamp(ref secs);
                }
            }
        }
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
        public bool TxnWriteNoSync {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_TXN_WRITE_NOSYNC) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_TXN_WRITE_NOSYNC, value ? 1 : 0);
            }
        }
        /// <summary>
        /// If true, all databases in the environment will be opened as if
        /// <see cref="DatabaseConfig.UseMVCC"/> was set.
        /// </summary>
        /// <remarks>
        /// This flag will be ignored for queue databases for which MVCC is not
        /// supported.
        /// </remarks>
        public bool UseMVCC {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_MULTIVERSION) != 0;
            }
            set { dbenv.set_flags(DbConstants.DB_MULTIVERSION, value ? 1 : 0); }
        }
        /// <summary>
        /// If true, locking for the Berkeley DB Concurrent Data Store product
        /// was initialized.
        /// </summary>
        public bool UsingCDB {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_INIT_CDB) != 0;
            }
        }
        /// <summary>
        /// If true, the locking subsystem was initialized.
        /// </summary>
        public bool UsingLocking {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_INIT_LOCK) != 0;
            }
        }
        /// <summary>
        /// If true, the logging subsystem was initialized.
        /// </summary>
        public bool UsingLogging {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_INIT_LOG) != 0;
            }
        }
        /// <summary>
        /// If true, the shared memory buffer pool subsystem was initialized.
        /// </summary>
        public bool UsingMPool {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_INIT_MPOOL) != 0;
            }
        }
        /// <summary>
        /// If true, the replication subsystem was initialized.
        /// </summary>
        public bool UsingReplication {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_INIT_REP) != 0;
            }
        }
        /// <summary>
        /// If true, the transaction subsystem was initialized.
        /// </summary>
        public bool UsingTxns {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_INIT_TXN) != 0;
            }
        }
        /// <summary>
        /// Specific additional informational and debugging messages in the
        /// Berkeley DB message output.
        /// </summary>
        public VerboseMessages Verbosity {
            get {
                uint flags = 0;
                dbenv.get_verbose(ref flags);
                return VerboseMessages.FromFlags(flags);
            }
            set {
                if (value.MessagesOff != 0)
                    dbenv.set_verbose(value.MessagesOff, 0);
                if (value.MessagesOn != 0)
                    dbenv.set_verbose(value.MessagesOn, 1);
            }
        }
        /// <summary>
        /// If true, Berkeley DB will yield the processor immediately after each
        /// page or mutex acquisition.
        /// </summary>
        /// <remarks>
        /// This functionality should never be used for purposes other than
        /// stress testing.
        /// </remarks>
        public bool YieldCPU {
            get {
                uint flags = 0;
                dbenv.get_flags(ref flags);
                return (flags & DbConstants.DB_YIELDCPU) != 0;
            }
            set {
                dbenv.set_flags(DbConstants.DB_YIELDCPU, value ? 1 : 0);
            }
        }
        #endregion Properties

        /// <summary>
        /// Instantiate a new DatabaseEnvironment object and open the Berkeley
        /// DB environment represented by <paramref name="home"/>.
        /// </summary>
        /// <param name="home">
        /// The database environment's home directory.  For more information on
        /// home, and filename resolution in general, see Berkeley DB File
        /// Naming in the Programmer's Reference Guide.
        /// </param>
        /// <param name="cfg">The environment's configuration</param>
        /// <returns>A new, open DatabaseEnvironment object</returns>
        public static DatabaseEnvironment Open(
            String home, DatabaseEnvironmentConfig cfg) {
            DatabaseEnvironment env = new DatabaseEnvironment(0);
            env.Config(cfg);
            env.dbenv.open(home, cfg.openFlags, 0);
            return env;
        }

        /// <summary>
        /// Destroy a Berkeley DB environment if it is not currently in use.
        /// </summary>
        /// <overloads>
        /// <para>
        /// The environment regions, including any backing files, are removed.
        /// Any log or database files and the environment directory are not
        /// removed.
        /// </para>
        /// <para>
        /// If there are processes that have called <see cref="Open"/> without
        /// calling <see cref="Close"/> (that is, there are processes currently
        /// using the environment), Remove will fail without further action.
        /// </para>
        /// <para>
        /// Calling Remove should not be necessary for most applications because
        /// the Berkeley DB environment is cleaned up as part of normal database
        /// recovery procedures. However, applications may want to call Remove
        /// as part of application shut down to free up system resources. For
        /// example, if <see cref="DatabaseEnvironmentConfig.SystemMemory"/> was
        /// specified to <see cref="Open"/>, it may be useful to call Remove in
        /// order to release system shared memory segments that have been
        /// allocated. Or, on architectures in which mutexes require allocation
        /// of underlying system resources, it may be useful to call Remove in
        /// order to release those resources. Alternatively, if recovery is not
        /// required because no database state is maintained across failures,
        /// and no system resources need to be released, it is possible to clean
        /// up an environment by simply removing all the Berkeley DB files in
        /// the database environment's directories.
        /// </para>
        /// <para>
        /// In multithreaded applications, only a single thread may call Remove.
        /// </para>
        /// </overloads>
        /// <param name="db_home">
        /// The database environment to be removed.
        /// </param>
        public static void Remove(string db_home) {
            Remove(db_home, false, false, false);
        }
        /// <summary>
        /// Destroy a Berkeley DB environment if it is not currently in use.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Generally, <paramref name="force"/> is specified only when
        /// applications were unable to shut down cleanly, and there is a risk
        /// that an application may have died holding a Berkeley DB lock.)
        /// </para>
        /// <para>
        /// The result of attempting to forcibly destroy the environment when it
        /// is in use is unspecified. Processes using an environment often
        /// maintain open file descriptors for shared regions within it. On UNIX
        /// systems, the environment removal will usually succeed, and processes
        /// that have already joined the region will continue to run in that
        /// region without change. However, processes attempting to join the
        /// environment will either fail or create new regions. On other systems
        /// in which the unlink(2) system call will fail if any process has an
        /// open file descriptor for the file (for example Windows/NT), the
        /// region removal will fail.
        /// </para>
        /// </remarks>
        /// <param name="db_home">
        /// The database environment to be removed.
        /// </param>
        /// <param name="force">
        /// If true, the environment is removed, regardless of any processes
        /// that may still using it, and no locks are acquired during this
        /// process.
        /// </param>
        public static void Remove(string db_home, bool force) {
            Remove(db_home, force, false, false);
        }
        private static void Remove(string db_home,
            bool force, bool USE_ENVIRON, bool USE_ENVIRON_ROOT) {
            DatabaseEnvironment env = new DatabaseEnvironment(0);
            uint flags = 0;

            flags |= force ? DbConstants.DB_FORCE : 0;
            flags |= USE_ENVIRON ? DbConstants.DB_USE_ENVIRON : 0;
            flags |= USE_ENVIRON_ROOT ? DbConstants.DB_USE_ENVIRON_ROOT : 0;
            env.dbenv.remove(db_home, flags);
        }

        /// <summary>
        /// Hold an election for the master of a replication group.
        /// </summary>
        public void RepHoldElection() {
            RepHoldElection(0, 0);
        }
        /// <summary>
        /// Hold an election for the master of a replication group.
        /// </summary>
        /// <param name="nsites">
        /// The number of replication sites expected to participate in the
        /// election. Once the current site has election information from that
        /// many sites, it will short-circuit the election and immediately cast
        /// its vote for a new master. This parameter must be no less than
        /// <paramref name="nvotes"/>, or 0 if the election should use
        /// <see cref="RepNSites"/>. If an application is using master leases,
        /// then the value must be 0 and <see cref="RepNSites"/> must be used.
        /// </param>
        public void RepHoldElection(uint nsites) {
            RepHoldElection(nsites, 0);
        }
        /// <summary>
        /// Hold an election for the master of a replication group.
        /// </summary>
        /// <overloads>
        /// <para>
        /// RepHoldElection is not called by most replication applications. It
        /// should only be called by applications implementing their own network
        /// transport layer, explicitly holding replication group elections and
        /// handling replication messages outside of the replication manager
        /// framework.
        /// </para>
        /// <para>
        /// If the election is successful, Berkeley DB will notify the
        /// application of the results of the election by means of either the 
        /// <see cref="NotificationEvent.REP_ELECTED"/> or 
        /// <see cref="NotificationEvent.REP_NEWMASTER"/> events (see 
        /// <see cref="EventNotify"/>for more information). The application is
        /// responsible for adjusting its relationship to the other database
        /// environments in the replication group, including directing all
        /// database updates to the newly selected master, in accordance with
        /// the results of the election.
        /// </para>
        /// <para>
        /// The thread of control that calls RepHoldElection must not be the
        /// thread of control that processes incoming messages; processing the
        /// incoming messages is necessary to successfully complete an election.
        /// </para>
        /// <para>
        /// Before calling this method, the <see cref="RepTransport"/> delegate 
        /// must already have been configured to send replication messages.
        /// </para>
        /// </overloads>
        /// <param name="nsites">
        /// The number of replication sites expected to participate in the
        /// election. Once the current site has election information from that
        /// many sites, it will short-circuit the election and immediately cast
        /// its vote for a new master. This parameter must be no less than
        /// <paramref name="nvotes"/>, or 0 if the election should use
        /// <see cref="RepNSites"/>. If an application is using master leases,
        /// then the value must be 0 and <see cref="RepNSites"/> must be used.
        /// </param>
        /// <param name="nvotes">
        /// The minimum number of replication sites from which the current site
        /// must have election information, before the current site will cast a
        /// vote for a new master. This parameter must be no greater than
        /// <paramref name="nsites"/>, or 0 if the election should use the value
        /// ((<paramref name="nsites"/> / 2) + 1).
        /// </param>
        public void RepHoldElection(uint nsites, uint nvotes) {
            dbenv.rep_elect(nsites, nvotes, 0);
        }

        /// <summary>
        /// Add a new replication site to the replication manager's list of
        /// known sites. It is not necessary for all sites in a replication
        /// group to know about all other sites in the group. 
        /// </summary>
        /// <param name="Host">The remote site's address</param>
        /// <returns>The environment ID assigned to the remote site</returns>
        public int RepMgrAddRemoteSite(ReplicationHostAddress Host) {
            return RepMgrAddRemoteSite(Host, false);
        }

        /// <summary>
        /// Add a new replication site to the replication manager's list of
        /// known sites. It is not necessary for all sites in a replication
        /// group to know about all other sites in the group. 
        /// </summary>
        /// <remarks>
        /// Currently, the replication manager framework only supports a single
        /// client peer, and the last specified peer is used.
        /// </remarks>
        /// <param name="Host">The remote site's address</param>
        /// <param name="isPeer">
        /// If true, configure client-to-client synchronization with the
        /// specified remote site.
        /// </param>
        /// <returns>The environment ID assigned to the remote site</returns>
        public int RepMgrAddRemoteSite(
            ReplicationHostAddress Host, bool isPeer) {
            int eidp = 0;
            dbenv.repmgr_add_remote_site(Host.Host,
                Host.Port, ref eidp, isPeer ? DbConstants.DB_REPMGR_PEER : 0);
            return eidp;
        }
        
        /// <summary>
        /// Start the replication manager as a client site, and do not call for
        /// an election.
        /// </summary>
        /// <overloads>
        /// <para>
        /// There are two ways to build Berkeley DB replication applications:
        /// the most common approach is to use the Berkeley DB library
        /// "replication manager" support, where the Berkeley DB library manages
        /// the replication group, including network transport, all replication
        /// message processing and acknowledgment, and group elections.
        /// Applications using the replication manager support generally make
        /// the following calls:
        /// </para>
        /// <list type="number">
        /// <item>
        /// Configure the local site in the replication group,
        /// <see cref="RepMgrLocalSite"/>.
        /// </item>
        /// <item>
        /// Call <see cref="RepMgrAddRemoteSite"/> to configure the remote
        /// site(s) in the replication group.
        /// </item>
        /// <item>Configure the message acknowledgment policy
        /// (<see cref="RepMgrAckPolicy"/>) which provides the replication group's
        /// transactional needs.
        /// </item>
        /// <item>
        /// Configure the local site's election priority,
        /// <see cref="RepPriority"/>.
        /// </item>
        /// <item>
        /// Call <see cref="RepMgrStartClient"/> or
        /// <see cref="RepMgrStartMaster"/> to start the replication
        /// application.
        /// </item>
        /// </list>
        /// <para>
        /// For more information on building replication manager applications,
        /// please see the Replication Getting Started Guide included in the
        /// Berkeley DB documentation.
        /// </para>
        /// <para>
        /// Applications with special needs (for example, applications using
        /// network protocols not supported by the Berkeley DB replication
        /// manager), must perform additional configuration and call other
        /// Berkeley DB replication methods. For more information on building
        /// advanced replication applications, please see the Base Replication
        /// API section in the Berkeley DB Programmer's Reference Guide for more
        /// information.
        /// </para>
        /// <para>
        /// Starting the replication manager consists of opening the TCP/IP
        /// listening socket to accept incoming connections, and starting all
        /// necessary background threads. When multiple processes share a
        /// database environment, only one process can open the listening
        /// socket; <see cref="RepMgrStartClient"/> (and
        /// <see cref="RepMgrStartMaster"/>) automatically open the socket in
        /// the first process to call it, and skips this step in the later calls
        /// from other processes.
        /// </para>
        /// </overloads>
        /// <param name="nthreads">
        /// Specify the number of threads of control created and dedicated to
        /// processing replication messages. In addition to these message
        /// processing threads, the replication manager creates and manages a
        /// few of its own threads of control.
        /// </param>
        public void RepMgrStartClient(int nthreads) {
            RepMgrStartClient(nthreads, false);
        }
        /// <summary>
        /// Start the replication manager as a client site, and optionally call
        /// for an election.
        /// </summary>
        /// <param name="nthreads">
        /// Specify the number of threads of control created and dedicated to
        /// processing replication messages. In addition to these message
        /// processing threads, the replication manager creates and manages a
        /// few of its own threads of control.
        /// </param>
        /// <param name="holdElection">
        /// If true, start as a client, and call for an election if no master is
        /// found.
        /// </param>
        public void RepMgrStartClient(int nthreads, bool holdElection) {
            dbenv.repmgr_start(nthreads,
                holdElection ?
                DbConstants.DB_REP_ELECTION : DbConstants.DB_REP_CLIENT);
        }
        /// <summary>
        /// Start the replication manager as a master site, and do not call for
        /// an election.
        /// </summary>
        /// <remarks>
        /// <para>
        /// There are two ways to build Berkeley DB replication applications:
        /// the most common approach is to use the Berkeley DB library
        /// "replication manager" support, where the Berkeley DB library manages
        /// the replication group, including network transport, all replication
        /// message processing and acknowledgment, and group elections.
        /// Applications using the replication manager support generally make
        /// the following calls:
        /// </para>
        /// <list type="number">
        /// <item>
        /// Configure the local site in the replication group,
        /// <see cref="RepMgrLocalSite"/>.
        /// </item>
        /// <item>
        /// Call <see cref="RepMgrAddRemoteSite"/> to configure the remote
        /// site(s) in the replication group.
        /// </item>
        /// <item>Configure the message acknowledgment policy
        /// (<see cref="RepMgrAckPolicy"/>) which provides the replication group's
        /// transactional needs.
        /// </item>
        /// <item>
        /// Configure the local site's election priority,
        /// <see cref="RepPriority"/>.
        /// </item>
        /// <item>
        /// Call <see cref="RepMgrStartClient"/> or
        /// <see cref="RepMgrStartMaster"/> to start the replication
        /// application.
        /// </item>
        /// </list>
        /// <para>
        /// For more information on building replication manager applications,
        /// please see the Replication Getting Started Guide included in the
        /// Berkeley DB documentation.
        /// </para>
        /// <para>
        /// Applications with special needs (for example, applications using
        /// network protocols not supported by the Berkeley DB replication
        /// manager), must perform additional configuration and call other
        /// Berkeley DB replication methods. For more information on building
        /// advanced replication applications, please see the Base Replication
        /// API section in the Berkeley DB Programmer's Reference Guide for more
        /// information.
        /// </para>
        /// <para>
        /// Starting the replication manager consists of opening the TCP/IP
        /// listening socket to accept incoming connections, and starting all
        /// necessary background threads. When multiple processes share a
        /// database environment, only one process can open the listening
        /// socket; <see cref="RepMgrStartMaster"/> (and
        /// <see cref="RepMgrStartClient"/>) automatically open the socket in
        /// the first process to call it, and skips this step in the later calls
        /// from other processes.
        /// </para>
        /// </remarks>
        /// <param name="nthreads">
        /// Specify the number of threads of control created and dedicated to
        /// processing replication messages. In addition to these message
        /// processing threads, the replication manager creates and manages a
        /// few of its own threads of control.
        /// </param>
        public void RepMgrStartMaster(int nthreads) {
            dbenv.repmgr_start(nthreads, DbConstants.DB_REP_MASTER);
        }
        
        /// <summary>
        /// Process an incoming replication message sent by a member of the
        /// replication group to the local database environment. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// RepProcessMessage is not called by most replication applications. It
        /// should only be called by applications implementing their own network
        /// transport layer, explicitly holding replication group elections and
        /// handling replication messages outside of the replication manager
        /// framework.
        /// </para>
        /// <para>
        /// For implementation reasons, all incoming replication messages must
        /// be processed using the same <see cref="DatabaseEnvironment"/>
        /// object. It is not required that a single thread of control process
        /// all messages, only that all threads of control processing messages
        /// use the same object.
        /// </para>
        /// <para>
        /// Before calling this method, the <see cref="RepTransport"/> delegate 
        /// must already have been configured to send replication messages.
        /// </para>
        /// </remarks>
        /// <param name="control">
        /// A copy of the control parameter specified by Berkeley DB on the
        /// sending environment.
        /// </param>
        /// <param name="rec">
        /// A copy of the rec parameter specified by Berkeley DB on the sending
        /// environment.
        /// </param>
        /// <param name="envid">
        /// The local identifier that corresponds to the environment that sent
        /// the message to be processed (see Replication environment IDs in the
        /// Programmer's Reference Guide for more information)..
        /// </param>
        /// <returns>The result of processing a message</returns>
        public RepProcMsgResult RepProcessMessage(
            DatabaseEntry control, DatabaseEntry rec, int envid) {
            DB_LSN dblsn = new DB_LSN();
            int ret = dbenv.rep_process_message(control,
                rec, envid, dblsn);
            LSN lsnp = new LSN(dblsn.file, dblsn.offset);
            RepProcMsgResult result = new RepProcMsgResult(ret, lsnp);
            if (result.Result == RepProcMsgResult.ProcMsgResult.ERROR)
                DatabaseException.ThrowException(ret);
            return result;
        }
        
        /// <summary>
        /// Configure the database environment as a client in a group of
        /// replicated database environments.
        /// </summary>
        public void RepStartClient() {
            RepStartClient(null);
        }
        /// <summary>
        /// Configure the database environment as a client in a group of
        /// replicated database environments.
        /// </summary>
        /// <overloads>
        /// <para>
        /// RepStartClient is not called by most replication applications. It
        /// should only be called by applications implementing their own network
        /// transport layer, explicitly holding replication group elections and
        /// handling replication messages outside of the replication manager
        /// framework.
        /// </para>
        /// <para>
        /// Replication master environments are the only database environments
        /// where replicated databases may be modified. Replication client
        /// environments are read-only as long as they are clients. Replication
        /// client environments may be upgraded to be replication master
        /// environments in the case that the current master fails or there is
        /// no master present. If master leases are in use, this method cannot
        /// be used to appoint a master, and should only be used to configure a
        /// database environment as a master as the result of an election.
        /// </para>
        /// <para>
        /// Before calling this method, the <see cref="RepTransport"/> delegate 
        /// must already have been configured to send replication messages.
        /// </para>
        /// </overloads>
        /// <param name="cdata">
        /// An opaque data item that is sent over the communication
        /// infrastructure when the client comes online (see Connecting to a new
        /// site in the Programmer's Reference Guide for more information). If
        /// no such information is useful, cdata should be null.
        /// </param>
        public void RepStartClient(DatabaseEntry cdata) {
            dbenv.rep_start(
                cdata, DbConstants.DB_REP_CLIENT);
        }
        /// <summary>
        /// Configure the database environment as a master in a group of
        /// replicated database environments.
        /// </summary>
        public void RepStartMaster() {
            RepStartMaster(null);
        }
        /// <summary>
        /// Configure the database environment as a master in a group of
        /// replicated database environments.
        /// </summary>
        /// <overloads>
        /// <para>
        /// RepStartMaster is not called by most replication applications. It
        /// should only be called by applications implementing their own network
        /// transport layer, explicitly holding replication group elections and
        /// handling replication messages outside of the replication manager
        /// framework.
        /// </para>
        /// <para>
        /// Replication master environments are the only database environments
        /// where replicated databases may be modified. Replication client
        /// environments are read-only as long as they are clients. Replication
        /// client environments may be upgraded to be replication master
        /// environments in the case that the current master fails or there is
        /// no master present. If master leases are in use, this method cannot
        /// be used to appoint a master, and should only be used to configure a
        /// database environment as a master as the result of an election.
        /// </para>
        /// <para>
        /// Before calling this method, the <see cref="RepTransport"/> delegate 
        /// must already have been configured to send replication messages.
        /// </para>
        /// </overloads>
        /// <param name="cdata">
        /// An opaque data item that is sent over the communication
        /// infrastructure when the client comes online (see Connecting to a new
        /// site in the Programmer's Reference Guide for more information). If
        /// no such information is useful, cdata should be null.
        /// </param>
        public void RepStartMaster(DatabaseEntry cdata) {
            dbenv.rep_start(
                cdata, DbConstants.DB_REP_MASTER);
        }

        /// <summary>
        /// Force master synchronization to begin for this client.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This method is the other half of setting
        /// <see cref="RepDelayClientSync"/>.
        /// </para>
        /// <para>
        /// If an application has configured delayed master synchronization, the
        /// application must synchronize explicitly (otherwise the client will
        /// remain out-of-date and will ignore all database changes forwarded
        /// from the replication group master). RepSync may be called any time
        /// after the client application learns that the new master has been
        /// established (by receiving
        /// <see cref="NotificationEvent.REP_NEWMASTER"/>).
        /// </para>
        /// <para>
        /// Before calling this method, the <see cref="RepTransport"/> delegate 
        /// must already have been configured to send replication messages.
        /// </para>
        /// </remarks>
        public void RepSync() {
            dbenv.rep_sync(0);
        }

        /// <summary>
        /// The names of all of the log files that are no longer in use (for
        /// example, that are no longer involved in active transactions), and
        /// that may safely be archived for catastrophic recovery and then
        /// removed from the system.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The Berkeley DB interfaces to the database environment logging
        /// subsystem (for example, <see cref="Transaction.Abort"/>) may
        /// allocate log cursors and have open file descriptors for log files
        /// as well. On operating systems where filesystem related system calls
        /// (for example, rename and unlink on Windows/NT) can fail if a process
        /// has an open file descriptor for the affected file, attempting to
        /// move or remove the log files listed by ArchivableLogFiles may fail.
        /// All Berkeley DB internal use of log cursors operates on active log
        /// files only and furthermore, is short-lived in nature. So, an
        /// application seeing such a failure should be restructured to retry
        /// the operation until it succeeds. (Although this is not likely to be
        /// necessary; it is hard to imagine a reason to move or rename a log
        /// file in which transactions are being logged or aborted.)
        /// </para>
        /// <para>
        /// See the db_archive utility for more information on database archival
        /// procedures.
        /// </para>
        /// </remarks>
        /// <param name="AbsolutePaths">
        /// If true, all pathnames are returned as absolute pathnames, instead 
        /// of relative to the database home directory.
        /// </param>
        /// <returns>
        /// The names of all of the log files that are no longer in use
        /// </returns>
        public List<string> ArchivableLogFiles(bool AbsolutePaths) {
            uint flags = 0;
            flags |= AbsolutePaths ? DbConstants.DB_ARCH_ABS : 0;
            return dbenv.log_archive(flags);
        }
        /// <summary>
        /// The database files that need to be archived in order to recover the
        /// database from catastrophic failure. If any of the database files
        /// have not been accessed during the lifetime of the current log files,
        /// they will not included in this list. It is also possible that some
        /// of the files referred to by the log have since been deleted from the
        /// system. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// See the db_archive utility for more information on database archival
        /// procedures.
        /// </para>
        /// </remarks>
        /// <param name="AbsolutePaths">
        /// If true, all pathnames are returned as absolute pathnames, instead 
        /// of relative to the database home directory.
        /// </param>
        /// <returns>
        /// The database files that need to be archived in order to recover the
        /// database from catastrophic failure.
        /// </returns>
        public List<string> ArchivableDatabaseFiles(bool AbsolutePaths) {
            uint flags = DbConstants.DB_ARCH_DATA;
            flags |= AbsolutePaths ? DbConstants.DB_ARCH_ABS : 0;
            return dbenv.log_archive(flags);
        }
        /// <summary>
        /// The names of all of the log files
        /// </summary>
        /// <remarks>
        /// <para>
        /// The Berkeley DB interfaces to the database environment logging
        /// subsystem (for example, <see cref="Transaction.Abort"/>) may
        /// allocate log cursors and have open file descriptors for log files
        /// as well. On operating systems where filesystem related system calls
        /// (for example, rename and unlink on Windows/NT) can fail if a process
        /// has an open file descriptor for the affected file, attempting to
        /// move or remove the log files listed by LogFiles may fail. All
        /// Berkeley DB internal use of log cursors operates on active log files
        /// only and furthermore, is short-lived in nature. So, an application
        /// seeing such a failure should be restructured to retry the operation
        /// until it succeeds. (Although this is not likely to be necessary; it
        /// is hard to imagine a reason to move or rename a log file in which
        /// transactions are being logged or aborted.)
        /// </para>
        /// <para>
        /// See the db_archive utility for more information on database archival
        /// procedures.
        /// </para>
        /// </remarks>
        /// <param name="AbsolutePaths">
        /// If true, all pathnames are returned as absolute pathnames, instead 
        /// of relative to the database home directory.
        /// </param>
        /// <returns>
        /// All the log filenames, regardless of whether or not they are in use.
        /// </returns>
        public List<string> LogFiles(bool AbsolutePaths) {
            uint flags = DbConstants.DB_ARCH_LOG;
            flags |= AbsolutePaths ? DbConstants.DB_ARCH_ABS : 0;
            return dbenv.log_archive(flags);
        }
        /// <summary>
        /// Remove log files that are no longer needed. Automatic log file
        /// removal is likely to make catastrophic recovery impossible. 
        /// </summary>
        public void RemoveUnusedLogFiles() {
            dbenv.log_archive(DbConstants.DB_ARCH_REMOVE);
        }

        /// <summary>
        /// Allocate a locker ID in an environment configured for Berkeley DB
        /// Concurrent Data Store applications.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Calling <see cref="Transaction.Commit"/> will discard the allocated
        /// locker ID.
        /// </para>
        /// <para>
        /// See Berkeley DB Concurrent Data Store applications in the
        /// Programmer's Reference Guide for more information about when this is
        /// required.
        /// </para>
        /// </remarks>
        /// <returns>
        /// A Transaction object that uniquely identifies the locker ID
        /// </returns>
        public Transaction BeginCDSGroup() {
            return new Transaction(dbenv.cdsgroup_begin());
        }

        /// <summary>
        /// Create a new transaction in the environment, with the default 
        /// configuration.
        /// </summary>
        /// <returns>A new transaction object</returns>
        public Transaction BeginTransaction() {
            return BeginTransaction(new TransactionConfig(), null);
        }
        /// <summary>
        /// Create a new transaction in the environment.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the transaction
        /// </param>
        /// <returns>A new transaction object</returns>
        public Transaction BeginTransaction(TransactionConfig cfg) {
            return BeginTransaction(cfg, null);
        }
        /// <summary>
        /// Create a new transaction in the environment.
        /// </summary>
        /// <remarks>
        /// In the presence of distributed transactions and two-phase commit,
        /// only the parental transaction, that is a transaction without a
        /// parent specified, should be passed as an parameter to 
        /// <see cref="Transaction.Prepare"/>.
        /// </remarks>
        /// <param name="cfg">
        /// The configuration properties for the transaction
        /// </param>
        /// <param name="parent">
        /// If the non-null, the new transaction will be a nested transaction,
        /// with <paramref name="parent"/> as the new transaction's parent.
        /// Transactions may be nested to any level.
        /// </param>
        /// <returns>A new transaction object</returns>
        public Transaction BeginTransaction(
            TransactionConfig cfg, Transaction parent) {
            DB_TXN dbtxn = dbenv.txn_begin(
                Transaction.getDB_TXN(parent), cfg.flags);
            Transaction txn = new Transaction(dbtxn);
            if (cfg.lockTimeoutIsSet)
                txn.SetLockTimeout(cfg.LockTimeout);
            if (cfg.nameIsSet)
                txn.Name = cfg.Name;
            if (cfg.txnTimeoutIsSet)
                txn.SetTxnTimeout(cfg.TxnTimeout);

            return txn;
        }

        /// <summary>
        /// Flush the underlying memory pool, write a checkpoint record to the
        /// log, and then flush the log, even if there has been no activity
        /// since the last checkpoint.
        /// </summary>
        public void Checkpoint() {
            dbenv.txn_checkpoint(0, 0, DbConstants.DB_FORCE);
        }
        /// <summary>
        /// If there has been any logging activity in the database environment
        /// since the last checkpoint, flush the underlying memory pool, write a
        /// checkpoint record to the log, and then flush the log.
        /// </summary>
        /// <param name="kbytesWritten">
        /// A checkpoint will be done if more than kbytesWritten kilobytes of
        /// log data have been written since the last checkpoint.
        /// </param>
        /// <param name="minutesElapsed">
        /// A checkpoint will be done if more than minutesElapsed minutes have
        /// passed since the last checkpoint.
        /// </param>
        public void Checkpoint(uint kbytesWritten, uint minutesElapsed) {
            dbenv.txn_checkpoint(kbytesWritten, minutesElapsed, 0);
        }

        /// <summary>
        /// Close the Berkeley DB environment, freeing any allocated resources
        /// and closing any underlying subsystems. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// The object should not be closed while any other handle that refers
        /// to it is not yet closed; for example, database environment handles
        /// must not be closed while database objects remain open, or
        /// transactions in the environment have not yet been committed or
        /// aborted.
        /// </para>
        /// <para>
        /// Where the environment was configured with
        /// <see cref="DatabaseEnvironmentConfig.UseTxns"/>, calling Close
        /// aborts any unresolved transactions. Applications should not depend
        /// on this behavior for transactions involving Berkeley DB databases;
        /// all such transactions should be explicitly resolved. The problem
        /// with depending on this semantic is that aborting an unresolved
        /// transaction involving database operations requires a database
        /// handle. Because the database handles should have been closed before
        /// calling Close, it will not be possible to abort the transaction, and
        /// recovery will have to be run on the Berkeley DB environment before
        /// further operations are done.
        /// </para>
        /// <para>
        /// In multithreaded applications, only a single thread may call Close.
        /// </para>
        /// </remarks>
        public void Close() {
            dbenv.close(0);
        }

        /// <summary>
        /// Run one iteration of the deadlock detector. The deadlock detector
        /// traverses the lock table and marks one of the participating lock
        /// requesters for rejection in each deadlock it finds.
        /// </summary>
        /// <param name="atype">Specify which lock request(s) to reject</param>
        /// <returns>The number of lock requests that were rejected.</returns>
        public uint DetectDeadlocks(DeadlockPolicy atype) {
            uint rejectCount = 0;
            if (atype == null)
                atype = DeadlockPolicy.DEFAULT;
            dbenv.lock_detect(0, atype.policy, ref rejectCount);
            return rejectCount;
        }

        /// <summary>
        /// Check for threads of control (either a true thread or a process)
        /// that have exited while manipulating Berkeley DB library data
        /// structures, while holding a logical database lock, or with an
        /// unresolved transaction (that is, a transaction that was never
        /// aborted or committed).
        /// </summary>
        /// <remarks>
        /// <para>
        /// For more information, see Architecting Data Store and Concurrent
        /// Data Store applications, and Architecting Transactional Data Store
        /// applications, both in the Berkeley DB Programmer's Reference Guide.
        /// </para>
        /// <para>
        /// FailCheck is based on the <see cref="SetThreadID"/> and
        /// <see cref="ThreadIsAlive"/> delegates. Applications calling
        /// FailCheck must have already set <see cref="ThreadIsAlive"/>, and
        /// must have configured <see cref="ThreadCount"/>.
        /// </para>
        /// <para>
        /// If FailCheck determines a thread of control exited while holding
        /// database read locks, it will release those locks. If FailCheck
        /// determines a thread of control exited with an unresolved
        /// transaction, the transaction will be aborted. In either of these
        /// cases, FailCheck will return successfully and the application may
        /// continue to use the database environment.
        /// </para>
        /// <para>
        /// In either of these cases, FailCheck will also report the process and
        /// thread IDs associated with any released locks or aborted
        /// transactions. The information is printed to a specified output
        /// channel (see <see cref="MessageFile"/> for more information), or
        /// passed to an application delegate (see <see cref="MessageCall"/> for
        /// more information).
        /// </para>
        /// <para>
        /// If FailCheck determines a thread of control has exited such that
        /// database environment recovery is required, it will throw
        /// <see cref="RunRecoveryException"/>. In this case, the application
        /// should not continue to use the database environment. For a further
        /// description as to the actions the application should take when this
        /// failure occurs, see Handling failure in Data Store and Concurrent
        /// Data Store applications, and Handling failure in Transactional Data
        /// Store applications, both in the Berkeley DB Programmer's Reference
        /// Guide.
        /// </para>
        /// </remarks>
        public void FailCheck() {
            dbenv.failchk(0);
        }

        /// <summary>
        /// Map an LSN object to a log filename
        /// </summary>
        /// <param name="logSeqNum">
        /// The DB_LSN structure for which a filename is wanted.
        /// </param>
        /// <returns>
        /// The name of the file containing the record named by
        /// <paramref name="logSeqNum"/>.
        /// </returns>
        public string LogFile(LSN logSeqNum) {
            return dbenv.log_file(LSN.getDB_LSN(logSeqNum));
        }

        /// <summary>
        /// Write all log records to disk.
        /// </summary>
        public void LogFlush() {
            LogFlush(null);
        }
        /// <summary>
        /// Write log records to disk.
        /// </summary>
        /// <param name="logSeqNum">
        /// All log records with LSN values less than or equal to
        /// <paramref name="logSeqNum"/> are written to disk. If null, all
        /// records in the log are flushed.
        /// </param>
        public void LogFlush(LSN logSeqNum) {
            dbenv.log_flush(LSN.getDB_LSN(logSeqNum));
        }

        /// <summary>
        /// Append a record to the log
        /// </summary>
        /// <param name="dbt">The record to write to the log.</param>
        /// <param name="flush">
        /// If true, the log is forced to disk after this record is written,
        /// guaranteeing that all records with LSN values less than or equal to
        /// the one being "put" are on disk before LogWrite returns.
        /// </param>
        /// <returns>The LSN of the written record</returns>
        public LSN LogWrite(DatabaseEntry dbt, bool flush) {
            DB_LSN lsn = new DB_LSN();

            dbenv.log_put(lsn,
                dbt, flush ? DbConstants.DB_FLUSH : 0);
            return new LSN(lsn.file, lsn.offset);
        }

        /// <summary>
        /// Set the panic state for the database environment. (Database
        /// environments in a panic state normally refuse all attempts to call
        /// Berkeley DB functions, throwing <see cref="RunRecoveryException"/>.)
        /// </summary>
        public void Panic() {
            dbenv.set_flags(DbConstants.DB_PANIC_ENVIRONMENT, 1);
        }

        /// <summary>
        /// Restore transactions that were prepared, but not yet resolved at the
        /// time of the system shut down or crash, to their state prior to the
        /// shut down or crash, including any locks previously held.
        /// </summary>
        /// <remarks>
        /// Calls to Recover from different threads of control should not be
        /// intermixed in the same environment.
        /// </remarks>
        /// <param name="count">
        /// The maximum number of <see cref="PreparedTransaction"/> objects
        /// to return.
        /// </param>
        /// <param name="resume">
        /// If true, continue returning a list of prepared, but not yet resolved
        /// transactions, starting where the last call to Recover left off.  If 
        /// false, begins a new pass over all prepared, but not yet completed
        /// transactions, regardless of whether they have already been returned
        /// in previous calls to Recover.
        /// </param>
        /// <returns>A list of the prepared transactions</returns>
        public PreparedTransaction[] Recover(uint count, bool resume) {
            uint flags = 0;
            flags |= resume ? DbConstants.DB_NEXT : DbConstants.DB_FIRST;
            
            return dbenv.txn_recover(count, flags);
        }

        /// <summary>
        /// Remove the underlying file represented by <paramref name="file"/>,
        /// incidentally removing all of the databases it contained.
        /// </summary>
        /// <param name="file">The physical file to be removed.</param>
        /// <param name="autoCommit">
        /// If true, enclose RemoveDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        public void RemoveDB(string file, bool autoCommit) {
            RemoveDB(file, null, autoCommit, null);
        }
        /// <summary>
        /// Remove the database specified by <paramref name="file"/> and
        /// <paramref name="database"/>.  If no database is specified, the
        /// underlying file represented by <paramref name="file"/> is removed,
        /// incidentally removing all of the databases it contained.
        /// </summary>
        /// <param name="file">
        /// The physical file which contains the database(s) to be removed.
        /// </param>
        /// <param name="database">The database to be removed.</param>
        /// <param name="autoCommit">
        /// If true, enclose RemoveDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        public void RemoveDB(string file, string database, bool autoCommit) {
            RemoveDB(file, database, autoCommit, null);
        }
        /// <summary>
        /// Remove the underlying file represented by <paramref name="file"/>,
        /// incidentally removing all of the databases it contained.
        /// </summary>
        /// <param name="file">The physical file to be removed.</param>
        /// <param name="autoCommit">
        /// If true, enclose RemoveDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.  If
        /// null, but <paramref name="autoCommit"/> or <see cref="AutoCommit"/>
        /// is true, the operation will be implicitly transaction protected. 
        /// </param>
        public void RemoveDB(string file, bool autoCommit, Transaction txn) {
            RemoveDB(file, null, autoCommit, txn);
        }
        /// <summary>
        /// Remove the database specified by <paramref name="file"/> and
        /// <paramref name="database"/>.  If no database is specified, the
        /// underlying file represented by <paramref name="file"/> is removed,
        /// incidentally removing all of the databases it contained.
        /// </summary>
        /// <param name="file">
        /// The physical file which contains the database(s) to be removed.
        /// </param>
        /// <param name="database">The database to be removed.</param>
        /// <param name="autoCommit">
        /// If true, enclose RemoveDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.  If
        /// null, but <paramref name="autoCommit"/> or <see cref="AutoCommit"/>
        /// is true, the operation will be implicitly transaction protected. 
        /// </param>
        public void RemoveDB(
            string file, string database, bool autoCommit, Transaction txn) {
            dbenv.dbremove(Transaction.getDB_TXN(txn),
                file, database, autoCommit ? DbConstants.DB_AUTO_COMMIT : 0);
        }

        /// <summary>
        /// Rename the underlying file represented by <paramref name="file"/>
        /// using the value supplied to <paramref name="newname"/>, incidentally
        /// renaming all of the databases it contained.
        /// </summary>
        /// <param name="file">The physical file to be renamed.</param>
        /// <param name="newname">The new name of the database or file.</param>
        /// <param name="autoCommit">
        /// If true, enclose RenameDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        public void RenameDB(string file, string newname, bool autoCommit) {
            RenameDB(file, null, newname, autoCommit, null);
        }
        /// <summary>
        /// Rename the database specified by <paramref name="file"/> and
        /// <paramref name="database"/> to <paramref name="newname"/>. If no
        /// database is specified, the underlying file represented by
        /// <paramref name="file"/> is renamed using the value supplied to
        /// <paramref name="newname"/>, incidentally renaming all of the
        /// databases it contained.
        /// </summary>
        /// <param name="file">
        /// The physical file which contains the database(s) to be renamed.
        /// </param>
        /// <param name="database">The database to be renamed.</param>
        /// <param name="newname">The new name of the database or file.</param>
        /// <param name="autoCommit">
        /// If true, enclose RenameDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        public void RenameDB(
            string file, string database, string newname, bool autoCommit) {
            RenameDB(file, database, newname, autoCommit, null);
        }
        /// <summary>
        /// Rename the underlying file represented by <paramref name="file"/>
        /// using the value supplied to <paramref name="newname"/>, incidentally
        /// renaming all of the databases it contained.
        /// </summary>
        /// <param name="file">The physical file to be renamed.</param>
        /// <param name="newname">The new name of the database or file.</param>
        /// <param name="autoCommit">
        /// If true, enclose RenameDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.  If
        /// null, but <paramref name="autoCommit"/> or <see cref="AutoCommit"/>
        /// is true, the operation will be implicitly transaction protected. 
        /// </param>
        public void RenameDB(string file,
            string newname, bool autoCommit, Transaction txn) {
            RenameDB(file, null, newname, autoCommit, txn);
        }
        /// <summary>
        /// Rename the database specified by <paramref name="file"/> and
        /// <paramref name="database"/> to <paramref name="newname"/>. If no
        /// database is specified, the underlying file represented by
        /// <paramref name="file"/> is renamed using the value supplied to
        /// <paramref name="newname"/>, incidentally renaming all of the
        /// databases it contained.
        /// </summary>
        /// <param name="file">
        /// The physical file which contains the database(s) to be renamed.
        /// </param>
        /// <param name="database">The database to be renamed.</param>
        /// <param name="newname">The new name of the database or file.</param>
        /// <param name="autoCommit">
        /// If true, enclose RenameDB within a transaction. If the call
        /// succeeds, changes made by the operation will be recoverable. If the
        /// call fails, the operation will have made no changes.
        /// </param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.  If
        /// null, but <paramref name="autoCommit"/> or <see cref="AutoCommit"/>
        /// is true, the operation will be implicitly transaction protected. 
        /// </param>
        public void RenameDB(string file,
            string database, string newname, bool autoCommit, Transaction txn) {
            dbenv.dbrename(Transaction.getDB_TXN(txn), file,
                database, newname, autoCommit ? DbConstants.DB_AUTO_COMMIT : 0);
        }

        /// <summary>
        /// Allow database files to be copied, and then the copy used in the
        /// same database environment as the original.
        /// </summary>
        /// <remarks>
        /// <para>
        /// All databases contain an ID string used to identify the database in
        /// the database environment cache. If a physical database file is
        /// copied, and used in the same environment as another file with the
        /// same ID strings, corruption can occur. ResetFileID creates new ID
        /// strings for all of the databases in the physical file.
        /// </para>
        /// <para>
        /// ResetFileID modifies the physical file, in-place. Applications
        /// should not reset IDs in files that are currently in use.
        /// </para>
        /// </remarks>
        /// <param name="file">
        /// The name of the physical file in which new file IDs are to be created.
        /// </param>
        /// <param name="encrypted">
        /// If true, he file contains encrypted databases.
        /// </param>
        public void ResetFileID(string file, bool encrypted) {
            dbenv.fileid_reset(file, encrypted ? DbConstants.DB_ENCRYPT : 0);
        }

        /// <summary>
        /// Allow database files to be moved from one transactional database
        /// environment to another. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Database pages in transactional database environments contain
        /// references to the environment's log files (that is, log sequence
        /// numbers, or <see cref="LSN"/>s). Copying or moving a database file
        /// from one database environment to another, and then modifying it, can
        /// result in data corruption if the LSNs are not first cleared.
        /// </para>
        /// <para>
        /// Note that LSNs should be reset before moving or copying the database
        /// file into a new database environment, rather than moving or copying
        /// the database file and then resetting the LSNs. Berkeley DB has
        /// consistency checks that may be triggered if an application calls 
        /// ResetLSN on a database in a new environment when the database LSNs
        /// still reflect the old environment.
        /// </para>
        /// <para>
        /// The ResetLSN method modifies the physical file, in-place.
        /// Applications should not reset LSNs in files that are currently in
        /// use.
        /// </para>
        /// </remarks>
        /// <param name="file"></param>
        /// <param name="encrypted"></param>
        public void ResetLSN(string file, bool encrypted) {
            dbenv.lsn_reset(file, encrypted ? DbConstants.DB_ENCRYPT : 0);
        }

        /// <summary>
        /// Limit the number of sequential write operations scheduled by the
        /// library when flushing dirty pages from the cache.
        /// </summary>
        /// <param name="maxWrites">
        /// The maximum number of sequential write operations scheduled by the
        /// library when flushing dirty pages from the cache, or 0 if there is
        /// no limitation on the number of sequential write operations.
        /// </param>
        /// <param name="pause">
        /// The number of microseconds the thread of control should pause before
        /// scheduling further write operations. It must be specified as an
        /// unsigned 32-bit number of microseconds, limiting the maximum pause
        /// to roughly 71 minutes.
        /// </param>
        public void SetMaxSequentialWrites(int maxWrites, uint pause) {
            dbenv.set_mp_max_write(maxWrites, pause);
        }

        /// <summary>
        /// Flush all modified pages in the cache to their backing files. 
        /// </summary>
        /// <remarks>
        /// Pages in the cache that cannot be immediately written back to disk
        /// (for example, pages that are currently in use by another thread of
        /// control) are waited for and written to disk as soon as it is
        /// possible to do so.
        /// </remarks>
        public void SyncMemPool() {
            SyncMemPool(null);
        }
        /// <summary>
        /// Flush modified pages in the cache with log sequence numbers less 
        /// than <paramref name="minLSN"/> to their backing files. 
        /// </summary>
        /// <remarks>
        /// Pages in the cache that cannot be immediately written back to disk
        /// (for example, pages that are currently in use by another thread of
        /// control) are waited for and written to disk as soon as it is
        /// possible to do so.
        /// </remarks>
        /// <param name="minLSN">
        /// All modified pages with a log sequence number less than the minLSN
        /// parameter are written to disk. If null, all modified pages in the
        /// cache are written to disk.
        /// </param>
        public void SyncMemPool(LSN minLSN) {
            dbenv.memp_sync(LSN.getDB_LSN(minLSN));
        }

        /// <summary>
        /// Ensure that a specified percent of the pages in the cache are clean,
        /// by writing dirty pages to their backing files. 
        /// </summary>
        /// <param name="pctClean">
        /// The percent of the pages in the cache that should be clean.
        /// </param>
        /// <returns>
        /// The number of pages written to reach the specified percentage is
        /// copied.
        /// </returns>
        public int TrickleCleanMemPool(int pctClean) {
            int ret = 0;
            dbenv.memp_trickle(pctClean, ref ret);
            return ret;
        }

        /// <summary>
        /// Append an informational message to the Berkeley DB database
        /// environment log files. 
        /// </summary>
        /// <overloads>
        /// WriteToLog allows applications to include information in the
        /// database environment log files, for later review using the
        /// db_printlog  utility. This method is intended for debugging and
        /// performance tuning.
        /// </overloads>
        /// <param name="str">The message to append to the log files</param>
        public void WriteToLog(string str) {
            dbenv.log_printf(null, str);
        }
        /// <summary>
        /// Append an informational message to the Berkeley DB database
        /// environment log files. 
        /// </summary>
        /// <overloads>
        /// WriteToLog allows applications to include information in the
        /// database environment log files, for later review using the
        /// db_printlog  utility. This method is intended for debugging and
        /// performance tuning.
        /// </overloads>
        /// <param name="str">The message to append to the log files</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; 
        /// otherwise null.
        /// </param>
        public void WriteToLog(string str, Transaction txn) {
            dbenv.log_printf(Transaction.getDB_TXN(txn), str);
        }

        #region Stats
        /// <summary>
        /// The locking subsystem statistics
        /// </summary>
        /// <returns>The locking subsystem statistics</returns>
        public LockStats LockingSystemStats() {
            return LockingSystemStats(false);
        }
        /// <summary>
        /// The locking subsystem statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The locking subsystem statistics</returns>
        public LockStats LockingSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            LockStatStruct st = dbenv.lock_stat(flags);
            return new LockStats(st);
        }
        /// <summary>
        /// The logging subsystem statistics
        /// </summary>
        /// <returns>The logging subsystem statistics</returns>
        public LogStats LoggingSystemStats() {
            return LoggingSystemStats(false);
        }
        /// <summary>
        /// The logging subsystem statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The logging subsystem statistics</returns>
        public LogStats LoggingSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            LogStatStruct st = dbenv.log_stat(flags);
            return new LogStats(st);
        }
        /// <summary>
        /// The memory pool (that is, the buffer cache) subsystem statistics
        /// </summary>
        /// <returns>The memory pool subsystem statistics</returns>
        public MPoolStats MPoolSystemStats() {
            return MPoolSystemStats(false);
        }
        /// <summary>
        /// The memory pool (that is, the buffer cache) subsystem statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The memory pool subsystem statistics</returns>
        public MPoolStats MPoolSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            MempStatStruct st = dbenv.memp_stat(flags);
            return new MPoolStats(st);
        }
        /// <summary>
        /// The mutex subsystem statistics
        /// </summary>
        /// <returns>The mutex subsystem statistics</returns>
        public MutexStats MutexSystemStats() {
            return MutexSystemStats(false);
        }
        /// <summary>
        /// The mutex subsystem statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The mutex subsystem statistics</returns>
        public MutexStats MutexSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            MutexStatStruct st = dbenv.mutex_stat(flags);
            return new MutexStats(st);
        }
        /// <summary>
        /// The replication manager statistics
        /// </summary>
        /// <returns>The replication manager statistics</returns>
        public RepMgrStats RepMgrSystemStats() {
            return RepMgrSystemStats(false);
        }
        /// <summary>
        /// The replication manager statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The replication manager statistics</returns>
        public RepMgrStats RepMgrSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            RepMgrStatStruct st = dbenv.repmgr_stat(flags);
            return new RepMgrStats(st);
        }
        /// <summary>
        /// The replication subsystem statistics
        /// </summary>
        /// <returns>The replication subsystem statistics</returns>
        public ReplicationStats ReplicationSystemStats() {
            return ReplicationSystemStats(false);
        }
        /// <summary>
        /// The replication subsystem statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The replication subsystem statistics</returns>
        public ReplicationStats ReplicationSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            ReplicationStatStruct st = dbenv.rep_stat(flags);
            return new ReplicationStats(st);
        }
        /// <summary>
        /// The transaction subsystem statistics
        /// </summary>
        /// <returns>The transaction subsystem statistics</returns>
        public TransactionStats TransactionSystemStats() {
            return TransactionSystemStats(false);
        }
        /// <summary>
        /// The transaction subsystem statistics
        /// </summary>
        /// <param name="clearStats">
        /// If true, reset statistics after returning their values.
        /// </param>
        /// <returns>The transaction subsystem statistics</returns>
        public TransactionStats TransactionSystemStats(bool clearStats) {
            uint flags = 0;
            flags |= clearStats ? DbConstants.DB_STAT_CLEAR : 0;
            TxnStatStruct st = dbenv.txn_stat(flags);
            return new TransactionStats(st);
        }
        #endregion Stats

        #region Print Stats
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        public void PrintLockingSystemStats() {
            PrintLockingSystemStats(false, false, false, false, false, false);
        }
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintLockingSystemStats(bool PrintAll, bool ClearStats) {
            PrintLockingSystemStats(
                PrintAll, ClearStats, false, false, false, false);
        }
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        /// <param name="ConflictMatrix">
        /// If true, display the lock conflict matrix.
        /// </param>
        /// <param name="Lockers">
        /// If true, Display the lockers within hash chains.
        /// </param>
        /// <param name="Objects">
        /// If true, display the lock objects within hash chains.
        /// </param>
        /// <param name="Parameters">
        /// If true, display the locking subsystem parameters.
        /// </param>
        public void PrintLockingSystemStats(bool PrintAll, bool ClearStats,
            bool ConflictMatrix, bool Lockers, bool Objects, bool Parameters) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;
            flags |= ConflictMatrix ? DbConstants.DB_STAT_LOCK_CONF : 0;
            flags |= Lockers ? DbConstants.DB_STAT_LOCK_LOCKERS : 0;
            flags |= Objects ? DbConstants.DB_STAT_LOCK_OBJECTS : 0;
            flags |= Parameters ? DbConstants.DB_STAT_LOCK_PARAMS : 0;

            dbenv.lock_stat_print(flags);
        }

        /// <summary>
        /// Display the logging subsystem statistical information, as described
        /// by <see cref="LogStats"/>.
        /// </summary>
        public void PrintLoggingSystemStats() {
            PrintLoggingSystemStats(false, false);
        }
        /// <summary>
        /// Display the logging subsystem statistical information, as described
        /// by <see cref="LogStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintLoggingSystemStats(bool PrintAll, bool ClearStats) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;

            dbenv.log_stat_print(flags);
        }

        /// <summary>
        /// Display the memory pool (that is, buffer cache) subsystem
        /// statistical information, as described by <see cref="MPoolStats"/>.
        /// </summary>
        public void PrintMPoolSystemStats() {
            PrintMPoolSystemStats(false, false, false);
        }
        /// <summary>
        /// Display the memory pool (that is, buffer cache) subsystem
        /// statistical information, as described by <see cref="MPoolStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintMPoolSystemStats(bool PrintAll, bool ClearStats) {
            PrintMPoolSystemStats(PrintAll, ClearStats, false);
        }
        /// <summary>
        /// Display the memory pool (that is, buffer cache) subsystem
        /// statistical information, as described by <see cref="MPoolStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        /// <param name="HashChains">
        /// If true, display the buffers with hash chains.
        /// </param>
        public void PrintMPoolSystemStats(
            bool PrintAll, bool ClearStats, bool HashChains) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;
            flags |= HashChains ? DbConstants.DB_STAT_MEMP_HASH : 0;

            dbenv.memp_stat_print(flags);
        }

        /// <summary>
        /// Display the mutex subsystem statistical information, as described
        /// by <see cref="MutexStats"/>.
        /// </summary>
        public void PrintMutexSystemStats() {
            PrintMutexSystemStats(false, false);
        }
        /// <summary>
        /// Display the mutex subsystem statistical information, as described
        /// by <see cref="MutexStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintMutexSystemStats(bool PrintAll, bool ClearStats) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;

            dbenv.mutex_stat_print(flags);
        }

        /// <summary>
        /// Display the replication manager statistical information, as
        /// described by <see cref="RepMgrStats"/>.
        /// </summary>
        public void PrintRepMgrSystemStats() {
            PrintRepMgrSystemStats(false, false);
        }
        /// <summary>
        /// Display the replication manager statistical information, as
        /// described by <see cref="RepMgrStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintRepMgrSystemStats(bool PrintAll, bool ClearStats) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;

            dbenv.repmgr_stat_print(flags);
        }

        /// <summary>
        /// Display the replication subsystem statistical information, as
        /// described by <see cref="ReplicationStats"/>.
        /// </summary>
        public void PrintReplicationSystemStats() {
            PrintReplicationSystemStats(false, false);
        }
        /// <summary>
        /// Display the replication subsystem statistical information, as
        /// described by <see cref="ReplicationStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void 
            PrintReplicationSystemStats(bool PrintAll, bool ClearStats) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;

            dbenv.rep_stat_print(flags);
        }

        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        public void PrintStats() {
            PrintStats(false, false, false);
        }
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintStats(bool PrintAll, bool ClearStats) {
            PrintStats(PrintAll, ClearStats, false);
        }
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        public void PrintSubsystemStats() {
            PrintStats(false, false, true);
        }
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void PrintSubsystemStats(bool PrintAll, bool ClearStats) {
            PrintStats(PrintAll, ClearStats, true);
        }
        /// <summary>
        /// Display the locking subsystem statistical information, as described
        /// by <see cref="LockStats"/>.
        /// </summary>
        private void PrintStats(bool all, bool clear, bool subs) {
            uint flags = 0;
            flags |= all ? DbConstants.DB_STAT_ALL : 0;
            flags |= clear ? DbConstants.DB_STAT_CLEAR : 0;
            flags |= subs ? DbConstants.DB_STAT_SUBSYSTEM : 0;
            dbenv.stat_print(flags);
        }

        /// <summary>
        /// Display the transaction subsystem statistical information, as
        /// described by <see cref="TransactionStats"/>.
        /// </summary>
        public void PrintTransactionSystemStats() {
            PrintTransactionSystemStats(false, false);
        }
        /// <summary>
        /// Display the transaction subsystem statistical information, as
        /// described by <see cref="TransactionStats"/>.
        /// </summary>
        /// <param name="PrintAll">
        /// If true, display all available information.
        /// </param>
        /// <param name="ClearStats">
        /// If true, reset statistics after displaying their values.
        /// </param>
        public void
            PrintTransactionSystemStats(bool PrintAll, bool ClearStats) {
            uint flags = 0;
            flags |= PrintAll ? DbConstants.DB_STAT_ALL : 0;
            flags |= ClearStats ? DbConstants.DB_STAT_CLEAR : 0;

            dbenv.txn_stat_print(flags);
        }
        #endregion Print Stats

        private uint getRepTimeout(int which) {
            uint ret = 0;
            dbenv.rep_get_timeout(which, ref ret);
            return ret;
        }
        private bool getRepConfig(uint which) {
            int onoff = 0;
            dbenv.rep_get_config(which, ref onoff);
            return (onoff != 0);
        }

        #region Unsupported Subsystem Methods
        /// <summary>
        /// The Berkeley DB process' environment may be permitted to specify
        /// information to be used when naming files; see Berkeley DB File
        /// Naming in the Programmer's Reference Guide for more information.
        /// </summary>
        public bool UseEnvironmentVars {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_USE_ENVIRON) != 0;
            }
        }
        private bool USE_ENVIRON_ROOT {
            get {
                uint flags = 0;
                dbenv.get_open_flags(ref flags);
                return (flags & DbConstants.DB_USE_ENVIRON_ROOT) != 0;
            }
        }
        private uint CreateLockerID() {
            uint ret = 0;
            dbenv.lock_id(ref ret);
            return ret;
        }
        private void FreeLockerID(uint locker) {
            dbenv.lock_id_free(locker);
        }

        private Lock GetLock(
            uint locker, bool wait, DatabaseEntry obj, LockMode mode) {
            return new Lock(dbenv.lock_get(locker,
                wait ? 0 : DbConstants.DB_LOCK_NOWAIT,
                DatabaseEntry.getDBT(obj), LockMode.GetMode(mode)));
        }

        private Mutex GetMutex(bool SelfBlock, bool SingleProcess) {
            uint m = 0;
            uint flags = 0;
            flags |= SelfBlock ? DbConstants.DB_MUTEX_SELF_BLOCK : 0;
            flags |= SingleProcess ? DbConstants.DB_MUTEX_PROCESS_ONLY : 0;
            dbenv.mutex_alloc(flags, ref m);
            return new Mutex(this, m);
        }

        private void LockMany(uint locker, bool wait, LockRequest[] vec) {
            LockMany(locker, wait, vec, null);
        }
        private void LockMany(
            uint locker, bool wait, LockRequest[] vec, LockRequest failedReq) {
            IntPtr[] reqList = new IntPtr[vec.Length];
            DB_LOCKREQ[] lst = new DB_LOCKREQ[vec.Length];

            for (int i = 0; i < vec.Length; i++) {
                reqList[i] = DB_LOCKREQ.getCPtr(
                    LockRequest.get_DB_LOCKREQ(vec[i])).Handle;
                lst[i] = LockRequest.get_DB_LOCKREQ(vec[i]);
            }

            dbenv.lock_vec(locker, wait ? 0 : DbConstants.DB_TXN_NOWAIT,
                reqList, vec.Length, LockRequest.get_DB_LOCKREQ(failedReq));

        }
        private void PutLock(Lock lck) {
            dbenv.lock_put(Lock.GetDB_LOCK(lck));
        }
        #endregion
    }
}