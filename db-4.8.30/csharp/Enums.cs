/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 */
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// The policy for how to handle database creation.
    /// </summary>
    public enum CreatePolicy : uint {
        /// <summary>
        /// Never create the database.
        /// </summary>
        NEVER = 0,
        /// <summary>
        /// Create the database if it does not already exist.
        /// </summary>
        IF_NEEDED = DbConstants.DB_CREATE,
        /// <summary>
        /// Do not open the database and return an error if it already exists.
        /// </summary>
        ALWAYS = DbConstants.DB_CREATE | DbConstants.DB_EXCL
    };

    /// <summary>
    /// Specifies the database operation whose progress is being reported
    /// </summary>
    public enum DatabaseFeedbackEvent : uint {
        /// <summary>
        /// The underlying database is being upgraded.
        /// </summary>
        UPGRADE = DbConstants.DB_UPGRADE,
        /// <summary>
        /// The underlying database is being verified.
        /// </summary>
        VERIFY = DbConstants.DB_VERIFY
    };

    /// <summary>
    /// Policy for duplicate data items in the database; that is, whether insertion
    /// when the key of the key/data pair being inserted already exists in the
    /// database will be successful. 
    /// </summary>
    public enum DuplicatesPolicy : uint {
        /// <summary>
        /// Insertion when the key of the key/data pair being inserted already
        /// exists in the database will fail.
        /// </summary>
        NONE = 0,
        /// <summary>
        /// Duplicates are allowed and mainted in sorted order, as determined by the
        /// duplicate comparison function.
        /// </summary>
        SORTED = DbConstants.DB_DUPSORT,
        /// <summary>
        /// Duplicates are allowed and ordered in the database by the order of
        /// insertion, unless the ordering is otherwise specified by use of a cursor
        /// operation or a duplicate sort function. 
        /// </summary>
        UNSORTED = DbConstants.DB_DUP
    };

    /// <summary>
    /// Specifies an algorithm used for encryption and decryption
    /// </summary>
    public enum EncryptionAlgorithm : uint {
        /// <summary>
        /// The default algorithm, or the algorithm previously used in an
        /// existing environment
        /// </summary>
        DEFAULT = 0,
        /// <summary>
        /// The Rijndael/AES algorithm
        /// </summary>
        /// <remarks>
        /// Also known as the Advanced Encryption Standard and Federal
        /// Information Processing Standard (FIPS) 197
        /// </remarks>
        AES = DbConstants.DB_ENCRYPT_AES
    };

    /// <summary>
    /// Specifies the environment operation whose progress is being reported
    /// </summary>
    public enum EnvironmentFeedbackEvent : uint {
        /// <summary>
        /// The environment is being recovered.
        /// </summary>
        RECOVERY = DbConstants.DB_RECOVER,
    };

    /// <summary>
    /// Specifies the action to take when deleting a foreign key
    /// </summary>
    public enum ForeignKeyDeleteAction : uint {
        /// <summary>
        /// Abort the deletion.
        /// </summary>
        ABORT = DbConstants.DB_FOREIGN_ABORT,
        /// <summary>
        /// Delete records that refer to the foreign key
        /// </summary>
        CASCADE = DbConstants.DB_FOREIGN_CASCADE,
        /// <summary>
        /// Nullify records that refer to the foreign key
        /// </summary>
        NULLIFY = DbConstants.DB_FOREIGN_NULLIFY
    };

    /// <summary>
    /// Specify the degree of isolation for transactional operations
    /// </summary>
    public enum Isolation {
        /// <summary>
        /// Read operations on the database may request the return of modified
        /// but not yet committed data.
        /// </summary>
        DEGREE_ONE,
        /// <summary>
        /// Provide for cursor stability but not repeatable reads. Data items
        /// which have been previously read by a transaction may be deleted or
        /// modified by other transactions before the original transaction
        /// completes.
        /// </summary>
        DEGREE_TWO,
        /// <summary>
        /// For the life of the transaction, every time a thread of control
        /// reads a data item, it will be unchanged from its previous value
        /// (assuming, of course, the thread of control does not itself modify
        /// the item).  This is Berkeley DB's default degree of isolation.
        /// </summary>
        DEGREE_THREE
    };

    /// <summary>
    /// Specify a Berkeley DB event
    /// </summary>
    public enum NotificationEvent : uint {
        /// <summary>
        /// The database environment has failed.
        /// </summary>
        /// <remarks>
        /// All threads of control in the database environment should exit the
        /// environment, and recovery should be run.
        /// </remarks>
        PANIC = DbConstants.DB_EVENT_PANIC,
        /// <summary>
        /// The local site is now a replication client.
        /// </summary>
        REP_CLIENT = DbConstants.DB_EVENT_REP_CLIENT,
        /// <summary>
        /// The local replication site has just won an election.
        /// </summary>
        /// <remarks>
        /// <para>
        /// An application using the Base replication API should arrange for a
        /// call to
        /// <see cref="DatabaseEnvironment.RepStartMaster"/> after
        /// receiving this event, to reconfigure the local environment as a
        /// replication master.
        /// </para>
        /// <para>
        /// Replication Manager applications may safely ignore this event. The
        /// Replication Manager calls
        /// <see cref="DatabaseEnvironment.RepStartMaster"/>
        /// automatically on behalf of the application when appropriate
        /// (resulting in firing of the <see cref="REP_MASTER"/> event).
        /// </para>
        /// </remarks>
        REP_ELECTED = DbConstants.DB_EVENT_REP_ELECTED,
        /// <summary>
        /// The local site is now the master site of its replication group. It
        /// is the application's responsibility to begin acting as the master
        /// environment.
        /// </summary>
        REP_MASTER = DbConstants.DB_EVENT_REP_MASTER,
        /// <summary>
        /// The replication group of which this site is a member has just
        /// established a new master; the local site is not the new master. The
        /// event_info parameter to the <see cref="EventNotifyDelegate"/>
        /// stores an integer containing the environment ID of the new master. 
        /// </summary>
        REP_NEWMASTER = DbConstants.DB_EVENT_REP_NEWMASTER,
        /// <summary>
        /// The replication manager did not receive enough acknowledgements
        /// (based on the acknowledgement policy configured with
        /// <see cref="ReplicationConfig.RepMgrAckPolicy"/>) to ensure a
        /// transaction's durability within the replication group. The
        /// transaction will be flushed to the master's local disk storage for
        /// durability.
        /// </summary>
        /// <remarks>
        /// This event is provided only to applications configured for the
        /// replication manager. 
        /// </remarks>
        REP_PERM_FAILED = DbConstants.DB_EVENT_REP_PERM_FAILED,
        /// <summary>
        /// The client has completed startup synchronization and is now
        /// processing live log records received from the master. 
        /// </summary>
        REP_STARTUPDONE = DbConstants.DB_EVENT_REP_STARTUPDONE,
        /// <summary>
        /// A Berkeley DB write to stable storage failed. 
        /// </summary>
        WRITE_FAILED = DbConstants.DB_EVENT_WRITE_FAILED
    };
}