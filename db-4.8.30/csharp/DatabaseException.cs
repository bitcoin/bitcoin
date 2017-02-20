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
    /// Represents errors that occur during Berkley DB operations.
    /// </summary>
    public class DatabaseException : Exception {
        /// <summary>
        /// The underlying error code from the Berkeley DB C library.
        /// </summary>
        public int ErrorCode;

        /// <summary>
        /// Throw an exception which corresponds to the specified Berkeley DB
        /// error code.
        /// </summary>
        /// <param name="err">The Berkeley DB error code</param>
        public static void ThrowException(int err) {
            switch (err) {
                case 0:
                    return;
                case ErrorCodes.DB_NOTFOUND:
                    throw new NotFoundException();
                case ErrorCodes.DB_BUFFER_SMALL:
                    throw new MemoryException();
                case ErrorCodes.DB_FOREIGN_CONFLICT:
                    throw new ForeignConflictException();
                case ErrorCodes.DB_KEYEMPTY:
                    throw new KeyEmptyException();
                case ErrorCodes.DB_KEYEXIST:
                    throw new KeyExistException();
                case ErrorCodes.DB_LOCK_DEADLOCK:
                    throw new DeadlockException();
                case ErrorCodes.DB_LOCK_NOTGRANTED:
                    throw new LockNotGrantedException();
                case ErrorCodes.DB_LOG_BUFFER_FULL:
                    throw new FullLogBufferException();
                case ErrorCodes.DB_OLD_VERSION:
                    throw new OldVersionException();
                case ErrorCodes.DB_PAGE_NOTFOUND:
                    throw new PageNotFoundException();
                case ErrorCodes.DB_REP_DUPMASTER:
                case ErrorCodes.DB_REP_HOLDELECTION:
                case ErrorCodes.DB_REP_IGNORE:
                case ErrorCodes.DB_REP_ISPERM:
                case ErrorCodes.DB_REP_JOIN_FAILURE:
                case ErrorCodes.DB_REP_NEWSITE:
                case ErrorCodes.DB_REP_NOTPERM:
                    return;
                case ErrorCodes.DB_REP_LEASE_EXPIRED:
                    throw new LeaseExpiredException();
                case ErrorCodes.DB_RUNRECOVERY:
                    throw new RunRecoveryException();
                case ErrorCodes.DB_SECONDARY_BAD:
                    throw new BadSecondaryException();
                case ErrorCodes.DB_VERIFY_BAD:
                    throw new VerificationException();
                case ErrorCodes.DB_VERSION_MISMATCH:
                    throw new VersionMismatchException();
                default:
                    throw new DatabaseException(err);
            }
        }

        /// <summary>
        /// Create a new DatabaseException, encapsulating a specific error code.
        /// </summary>
        /// <param name="err">The error code to encapsulate.</param>
        public DatabaseException(int err)
            : base(libdb_csharp.db_strerror(err)) {
            ErrorCode = err;
        }
    }

    /// <summary>
    /// A secondary index has been corrupted. This is likely the result of an
    /// application operating on related databases without first associating
    /// them.
    /// </summary>
    public class BadSecondaryException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the BadSecondaryException
        /// </summary>
        public BadSecondaryException() : base(ErrorCodes.DB_SECONDARY_BAD) { }
    }

    /// <summary>
    /// 
    /// </summary>
    public class ForeignConflictException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the ForeignConflictException
        /// </summary>
        public ForeignConflictException()
            : base(ErrorCodes.DB_FOREIGN_CONFLICT) { }
    }

    /// <summary>
    /// In-memory logs are configured and no more log buffer space is available.
    /// </summary>
    public class FullLogBufferException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the FullLogBufferException
        /// </summary>
        public FullLogBufferException()
            : base(ErrorCodes.DB_LOG_BUFFER_FULL) { }
    }

    /// <summary>
    /// The requested key/data pair logically exists but was never explicitly
    /// created by the application, or that the requested key/data pair was
    /// deleted and never re-created. In addition, the Queue access method will
    /// throw a KeyEmptyException for records that were created as part of a
    /// transaction that was later aborted and never re-created.
    /// </summary>
    /// <remarks>
    /// The Recno and Queue access methods will automatically create key/data
    /// pairs under some circumstances.
    /// </remarks>
    public class KeyEmptyException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the KeyEmptyException
        /// </summary>
        public KeyEmptyException() : base(ErrorCodes.DB_KEYEMPTY) { }
    }

    /// <summary>
    /// A key/data pair was inserted into the database using
    /// <see cref="Database.PutNoOverwrite"/> and the key already
    /// exists in the database, or using
    /// <see cref="BTreeDatabase.PutNoDuplicate"/> or
    /// <see cref="HashDatabase.PutNoDuplicate"/> and the key/data
    /// pair already exists in the database.
    /// </summary>
    public class KeyExistException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the KeyExistException
        /// </summary>
        public KeyExistException() : base(ErrorCodes.DB_KEYEXIST) { }
    }

    /// <summary>
    /// When multiple threads of control are modifying the database, there is
    /// normally the potential for deadlock. In Berkeley DB, deadlock is
    /// signified by a DeadlockException thrown from the Berkeley DB function.
    /// Whenever a Berkeley DB function throws a DeadlockException, the
    /// enclosing transaction should be aborted.
    /// </summary>
    public class DeadlockException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the DeadlockException
        /// </summary>
        public DeadlockException() : base(ErrorCodes.DB_LOCK_DEADLOCK) { }
    }

    /// <summary>
    /// The site's replication master lease has expired.
    /// </summary>
    public class LeaseExpiredException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the LeaseExpiredException
        /// </summary>
        public LeaseExpiredException()
            : base(ErrorCodes.DB_REP_LEASE_EXPIRED) { }
    }

    /// <summary>
    /// If <see cref="DatabaseEnvironmentConfig.TimeNotGranted"/> is true,
    /// database calls timing out based on lock or transaction timeout values
    /// will throw a LockNotGrantedException, instead of a DeadlockException.
    /// </summary>
    public class LockNotGrantedException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the LockNotGrantedException
        /// </summary>
        public LockNotGrantedException()
            : base(ErrorCodes.DB_LOCK_NOTGRANTED) { }
    }

    internal class MemoryException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the MemoryException
        /// </summary>
        internal MemoryException() : base(ErrorCodes.DB_BUFFER_SMALL) { }
    }

    /// <summary>
    /// The requested key/data pair did not exist in the database or that
    /// start-of- or end-of-file has been reached by a cursor.
    /// </summary>
    public class NotFoundException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the NotFoundException
        /// </summary>
        public NotFoundException() : base(ErrorCodes.DB_NOTFOUND) { }
    }

    /// <summary>
    /// This version of Berkeley DB is unable to upgrade a given database.
    /// </summary>
    public class OldVersionException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the OldVersionException
        /// </summary>
        public OldVersionException() : base(ErrorCodes.DB_OLD_VERSION) { }
    }

    /// <summary>
    /// 
    /// </summary>
    public class PageNotFoundException : DatabaseException {
        /// <summary>
        /// 
        /// </summary>
        public PageNotFoundException() : base(ErrorCodes.DB_PAGE_NOTFOUND) { }
    }

    /// <summary>
    /// Berkeley DB has encountered an error it considers fatal to an entire
    /// environment. Once a RunRecoveryException has been thrown by any
    /// interface, it will be returned from all subsequent Berkeley DB calls
    /// made by any threads of control participating in the environment.
    /// </summary>
    /// <remarks>
    /// An example of this type of fatal error is a corrupted database page. The
    /// only way to recover from this type of error is to have all threads of
    /// control exit the Berkeley DB environment, run recovery of the
    /// environment, and re-enter Berkeley DB. (It is not strictly necessary
    /// that the processes exit, although that is the only way to recover system
    /// resources, such as file descriptors and memory, allocated by
    /// Berkeley DB.)
    /// </remarks>
    public class RunRecoveryException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the RunRecoveryException
        /// </summary>
        public RunRecoveryException() : base(ErrorCodes.DB_RUNRECOVERY) { }
    }

    /// <summary>
    /// Thrown by <see cref="Database.Verify"/> if a database is
    /// corrupted, and by <see cref="Database.Salvage"/> if all
    /// key/data pairs in the file may not have been successfully output.
    /// </summary>
    public class VerificationException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the VerificationException
        /// </summary>
        public VerificationException() : base(ErrorCodes.DB_VERIFY_BAD) { }
    }

    /// <summary>
    /// The version of the Berkeley DB library doesn't match the version that
    /// created the database environment.
    /// </summary>
    public class VersionMismatchException : DatabaseException {
        /// <summary>
        /// Initialize a new instance of the VersionMismatchException
        /// </summary>
        public VersionMismatchException()
            : base(ErrorCodes.DB_VERSION_MISMATCH) { }
    }
}
