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
    /// A class representing configuration parameters for a
    /// <see cref="DatabaseEnvironment"/>'s logging subsystem.
    /// </summary>
    public class LogConfig {
        /// <summary>
        /// If true, Berkeley DB will automatically remove log files that are no
        /// longer needed.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Automatic log file removal is likely to make catastrophic recovery
        /// impossible.
        /// </para>
        /// <para>
        /// Replication applications will rarely want to configure automatic log
        /// file removal as it increases the likelihood a master will be unable
        /// to satisfy a client's request for a recent log record.
        /// </para>
        /// </remarks>
        public bool AutoRemove;
        /// <summary>
        /// If true, Berkeley DB will flush log writes to the backing disk
        /// before returning from the write system call, rather than flushing
        /// log writes explicitly in a separate system call, as necessary. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// This is only available on some systems (for example, systems
        /// supporting the IEEE/ANSI Std 1003.1 (POSIX) standard O_DSYNC flag,
        /// or systems supporting the Windows FILE_FLAG_WRITE_THROUGH flag).
        /// This flag may result in inaccurate file modification times and other
        /// file-level information for Berkeley DB log files. This flag may
        /// offer a performance increase on some systems and a performance
        /// decrease on others.
        /// </para>
        /// </remarks>
        public bool ForceSync;
        /// <summary>
        /// If true, maintain transaction logs in memory rather than on disk.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This means that transactions exhibit the ACI (atomicity,
        /// consistency, and isolation) properties, but not D (durability); that
        /// is, database integrity will be maintained, but if the application or
        /// system fails, integrity will not persist. All database files must be
        /// verified and/or restored from a replication group master or archival
        /// backup after application or system failure.
        /// </para> 
        /// <para>
        /// When in-memory logs are configured and no more log buffer space is
        /// available, Berkeley DB methods may throw
        /// <see cref="FullLogBufferException"/>. When choosing log buffer and
        /// file sizes for in-memory logs, applications should ensure the
        /// in-memory log buffer size is large enough that no transaction will
        /// ever span the entire buffer, and avoid a state where the in-memory
        /// buffer is full and no space can be freed because a transaction that
        /// started in the first log "file" is still active.
        /// </para>
        /// </remarks>
        public bool InMemory;
        /// <summary>
        /// If true, turn off system buffering of Berkeley DB log files to avoid
        /// double caching.
        /// </summary>
        public bool NoBuffer;
        /// <summary>
        /// If true, zero all pages of a log file when that log file is created.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This has shown to provide greater transaction throughput in some
        /// environments. The log file will be zeroed by the thread which needs
        /// to re-create the new log file. Other threads may not write to the
        /// log file while this is happening.
        /// </para>
        /// </remarks>
        public bool ZeroOnCreate;

        internal uint ConfigFlags {
            get {
                uint ret = 0;
                if (AutoRemove)
                    ret |= DbConstants.DB_LOG_AUTO_REMOVE;
                if (ForceSync)
                    ret |= DbConstants.DB_LOG_DSYNC;
                if (InMemory)
                    ret |= DbConstants.DB_LOG_IN_MEMORY;
                if (NoBuffer)
                    ret |= DbConstants.DB_LOG_DIRECT;
                if (ZeroOnCreate)
                    ret |= DbConstants.DB_LOG_ZERO;
                return ret;
            }
        }

        internal bool bsizeIsSet;
        private uint _bsize;
        /// <summary>
        /// The size of the in-memory log buffer, in bytes.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When the logging subsystem is configured for on-disk logging, the
        /// default size of the in-memory log buffer is approximately 32KB. Log
        /// information is stored in-memory until the storage space fills up or
        /// transaction commit forces the information to be flushed to stable
        /// storage. In the presence of long-running transactions or
        /// transactions producing large amounts of data, larger buffer sizes
        /// can increase throughput.
        /// </para>
        /// <para>
        /// When the logging subsystem is configured for in-memory logging, the
        /// default size of the in-memory log buffer is 1MB. Log information is
        /// stored in-memory until the storage space fills up or transaction
        /// abort or commit frees up the memory for new transactions. In the
        /// presence of long-running transactions or transactions producing
        /// large amounts of data, the buffer size must be sufficient to hold
        /// all log information that can accumulate during the longest running
        /// transaction. When choosing log buffer and file sizes for in-memory
        /// logs, applications should ensure the in-memory log buffer size is
        /// large enough that no transaction will ever span the entire buffer,
        /// and avoid a state where the in-memory buffer is full and no space
        /// can be freed because a transaction that started in the first log
        /// "file" is still active.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// BufferSize will be ignored.
        /// </para>
        /// </remarks>
        public uint BufferSize {
            get { return _bsize; }
            set {
                bsizeIsSet = true;
                _bsize = value;
            }
        }

        /// <summary>
        /// The path of a directory to be used as the location of logging files.
        /// Log files created by the Log Manager subsystem will be created in
        /// this directory. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If no logging directory is specified, log files are created in the
        /// environment home directory. See Berkeley DB File Naming in the
        /// Programmer's Reference Guide for more information.
        /// </para>
        /// <para>
        /// For the greatest degree of recoverability from system or application
        /// failure, database files and log files should be located on separate
        /// physical devices.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Dir must be consistent with the existing environment or corruption
        /// can occur.
        /// </para>
        /// </remarks>
        public string Dir;

        internal bool modeIsSet;
        private int _mode;
        /// <summary>
        /// The absolute file mode for created log files.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This method is only useful for the rare Berkeley DB application that
        /// does not control its umask value.
        /// </para>
        /// <para>
        /// Normally, if Berkeley DB applications set their umask appropriately,
        /// all processes in the application suite will have read permission on
        /// the log files created by any process in the application suite.
        /// However, if the Berkeley DB application is a library, a process
        /// using the library might set its umask to a value preventing other
        /// processes in the application suite from reading the log files it
        /// creates. In this rare case, the DB_ENV->set_lg_filemode() method can
        /// be used to set the mode of created log files to an absolute value.
        /// </para>
        /// </remarks>
        public int FileMode {
            get { return _mode; }
            set {
                modeIsSet = true;
                _mode = value;
            }
        }

        internal bool maxSizeIsSet;
        private uint _maxSize;
        /// <summary>
        /// The maximum size of a single file in the log, in bytes. Because 
        /// <see cref="LSN.Offset"/> is an unsigned four-byte value, MaxFileSize
        /// may not be larger than the maximum unsigned four-byte value.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When the logging subsystem is configured for on-disk logging, the
        /// default size of a log file is 10MB.
        /// </para>
        /// <para>
        /// When the logging subsystem is configured for in-memory logging, the
        /// default size of a log file is 256KB. In addition, the
        /// <see cref="BufferSize">configured log buffer size</see> must be
        /// larger than the log file size. (The logging subsystem divides memory
        /// configured for in-memory log records into "files", as database
        /// environments configured for in-memory log records may exchange log
        /// records with other members of a replication group, and those members
        /// may be configured to store log records on-disk.) When choosing log
        /// buffer and file sizes for in-memory logs, applications should ensure
        /// the in-memory log buffer size is large enough that no transaction
        /// will ever span the entire buffer, and avoid a state where the
        /// in-memory buffer is full and no space can be freed because a
        /// transaction that started in the first log "file" is still active.
        /// </para>
        /// <para>
        /// See Log File Limits in the Programmer's Reference Guide for more
        /// information.
        /// </para>
        /// <para>
        /// If no size is specified by the application, the size last specified
        /// for the database region will be used, or if no database region
        /// previously existed, the default will be used.
        /// </para></remarks>
        public uint MaxFileSize {
            get { return _maxSize; }
            set {
                maxSizeIsSet = true;
                _maxSize = value;
            }
        }

        internal bool regionSizeIsSet;
        private uint _regionSize;
        /// <summary>
        /// Te size of the underlying logging area of the Berkeley DB
        /// environment, in bytes.
        /// </summary>
        /// <remarks>
        /// <para>
        /// By default, or if the value is set to 0, the default size is
        /// approximately 60KB. The log region is used to store filenames, and
        /// so may need to be increased in size if a large number of files will
        /// be opened and registered with the specified Berkeley DB
        /// environment's log manager.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// RegionSize will be ignored.
        /// </para>
        /// </remarks>
        public uint RegionSize {
            get { return _regionSize; }
            set {
                regionSizeIsSet = true;
                _regionSize = value;
            }
        }

        
                
    }
}
