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
    /// Constants representing error codes returned by the Berkeley DB library.
    /// </summary>
    public class ErrorCodes {
        ///<summary>
        /// User memory too small for return. 
        ///</summary>
        public const int DB_BUFFER_SMALL = DbConstants.DB_BUFFER_SMALL;
        ///<summary>
        /// "Null" return from 2ndary callbk. 
        ///</summary>
        public const int DB_DONOTINDEX = DbConstants.DB_DONOTINDEX;
        ///<summary>
        /// A foreign db constraint triggered. 
        ///</summary>
        public const int DB_FOREIGN_CONFLICT = DbConstants.DB_FOREIGN_CONFLICT;
        ///<summary>
        /// Key/data deleted or never created. 
        ///</summary>
        public const int DB_KEYEMPTY = DbConstants.DB_KEYEMPTY;
        ///<summary>
        /// The key/data pair already exists. 
        ///</summary>
        public const int DB_KEYEXIST = DbConstants.DB_KEYEXIST;
        ///<summary>
        /// Deadlock. 
        ///</summary>
        public const int DB_LOCK_DEADLOCK = DbConstants.DB_LOCK_DEADLOCK;
        ///<summary>
        /// Lock unavailable. 
        ///</summary>
        public const int DB_LOCK_NOTGRANTED = DbConstants.DB_LOCK_NOTGRANTED;
        ///<summary>
        /// In-memory log buffer full. 
        ///</summary>
        public const int DB_LOG_BUFFER_FULL = DbConstants.DB_LOG_BUFFER_FULL;
        ///<summary>
        /// Server panic return. 
        ///</summary>
        public const int DB_NOSERVER = DbConstants.DB_NOSERVER;
        ///<summary>
        /// Bad home sent to server. 
        ///</summary>
        public const int DB_NOSERVER_HOME = DbConstants.DB_NOSERVER_HOME;
        ///<summary>
        /// Bad ID sent to server. 
        ///</summary>
        public const int DB_NOSERVER_ID = DbConstants.DB_NOSERVER_ID;
        ///<summary>
        /// Key/data pair not found (EOF). 
        ///</summary>
        public const int DB_NOTFOUND = DbConstants.DB_NOTFOUND;
        ///<summary>
        /// Out-of-date version. 
        ///</summary>
        public const int DB_OLD_VERSION = DbConstants.DB_OLD_VERSION;
        ///<summary>
        /// Requested page not found. 
        ///</summary>
        public const int DB_PAGE_NOTFOUND = DbConstants.DB_PAGE_NOTFOUND;
        ///<summary>
        /// There are two masters. 
        ///</summary>
        public const int DB_REP_DUPMASTER = DbConstants.DB_REP_DUPMASTER;
        ///<summary>
        /// Rolled back a commit. 
        ///</summary>
        public const int DB_REP_HANDLE_DEAD = DbConstants.DB_REP_HANDLE_DEAD;
        ///<summary>
        /// Time to hold an election. 
        ///</summary>
        public const int DB_REP_HOLDELECTION = DbConstants.DB_REP_HOLDELECTION;
        ///<summary>
        /// This msg should be ignored.
        ///</summary>
        public const int DB_REP_IGNORE = DbConstants.DB_REP_IGNORE;
        ///<summary>
        /// Cached not written perm written.
        ///</summary>
        public const int DB_REP_ISPERM = DbConstants.DB_REP_ISPERM;
        ///<summary>
        /// Unable to join replication group. 
        ///</summary>
        public const int DB_REP_JOIN_FAILURE = DbConstants.DB_REP_JOIN_FAILURE;
        ///<summary>
        /// Master lease has expired. 
        ///</summary>
        public const int DB_REP_LEASE_EXPIRED = DbConstants.DB_REP_LEASE_EXPIRED;
        ///<summary>
        /// API/Replication lockout now. 
        ///</summary>
        public const int DB_REP_LOCKOUT = DbConstants.DB_REP_LOCKOUT;
        ///<summary>
        /// New site entered system. 
        ///</summary>
        public const int DB_REP_NEWSITE = DbConstants.DB_REP_NEWSITE;
        ///<summary>
        /// Permanent log record not written. 
        ///</summary>
        public const int DB_REP_NOTPERM = DbConstants.DB_REP_NOTPERM;
        ///<summary>
        /// Site cannot currently be reached. 
        ///</summary>
        public const int DB_REP_UNAVAIL = DbConstants.DB_REP_UNAVAIL;
        ///<summary>
        /// Panic return. 
        ///</summary>
        public const int DB_RUNRECOVERY = DbConstants.DB_RUNRECOVERY;
        ///<summary>
        /// Secondary index corrupt. 
        ///</summary>
        public const int DB_SECONDARY_BAD = DbConstants.DB_SECONDARY_BAD;
        ///<summary>
        /// Verify failed; bad format. 
        ///</summary>
        public const int DB_VERIFY_BAD = DbConstants.DB_VERIFY_BAD;
        ///<summary>
        /// Environment version mismatch. 
        ///</summary>
        public const int DB_VERSION_MISMATCH = DbConstants.DB_VERSION_MISMATCH;

    }
}