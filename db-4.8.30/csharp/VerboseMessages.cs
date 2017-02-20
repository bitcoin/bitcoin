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
    /// Enable specific additional informational and debugging messages.
    /// </summary>
    public class VerboseMessages {
        /// <summary>
        /// Display additional information when doing deadlock detection.
        /// </summary>
        public bool Deadlock;
        /// <summary>
        /// Display additional information when performing filesystem operations
        /// such as open, close or rename. May not be available on all
        /// platforms. 
        /// </summary>
        public bool FileOps;
        /// <summary>
        /// Display additional information when performing all filesystem
        /// operations, including read and write. May not be available on all
        /// platforms. 
        /// </summary>
        public bool AllFileOps;
        /// <summary>
        /// Display additional information when performing recovery.
        /// </summary>
        public bool Recovery;
        /// <summary>
        /// Display additional information concerning support for
        /// <see cref="DatabaseEnvironment.Register"/>
        /// </summary>
        public bool Register;
        /// <summary>
        /// Display all detailed information about replication. This includes
        /// the information displayed by all of the other Replication* and
        /// RepMgr* values. 
        /// </summary>
        public bool Replication;
        /// <summary>
        /// Display detailed information about Replication Manager connection
        /// failures. 
        /// </summary>
        public bool RepMgrConnectionFailure;
        /// <summary>
        /// Display detailed information about general Replication Manager
        /// processing. 
        /// </summary>
        public bool RepMgrMisc;
        /// <summary>
        /// Display detailed information about replication elections.
        /// </summary>
        public bool ReplicationElection;
        /// <summary>
        /// Display detailed information about replication master leases. 
        /// </summary>
        public bool ReplicationLease;
        /// <summary>
        /// Display detailed information about general replication processing
        /// not covered by the other Replication* values. 
        /// </summary>
        public bool ReplicationMisc;
        /// <summary>
        /// Display detailed information about replication message processing. 
        /// </summary>
        public bool ReplicationMessages;
        /// <summary>
        /// Display detailed information about replication client
        /// synchronization. 
        /// </summary>
        public bool ReplicationSync;
        /// <summary>
        /// 
        /// </summary>
        public bool ReplicationTest;
        /// <summary>
        /// Display the waits-for table when doing deadlock detection.
        /// </summary>
        public bool WaitsForTable;

        internal uint MessagesOn {
            get {
                uint ret = 0;
                ret |= Deadlock ? DbConstants.DB_VERB_DEADLOCK : 0;
                ret |= FileOps ? DbConstants.DB_VERB_FILEOPS : 0;
                ret |= AllFileOps ? DbConstants.DB_VERB_FILEOPS_ALL : 0;
                ret |= Recovery ? DbConstants.DB_VERB_RECOVERY : 0;
                ret |= Register ? DbConstants.DB_VERB_REGISTER : 0;
                ret |= Replication ? DbConstants.DB_VERB_REPLICATION : 0;
                ret |= RepMgrConnectionFailure ? DbConstants.DB_VERB_REPMGR_CONNFAIL : 0;
                ret |= RepMgrMisc ? DbConstants.DB_VERB_REPMGR_MISC : 0;
                ret |= ReplicationElection ? DbConstants.DB_VERB_REP_ELECT : 0;
                ret |= ReplicationLease ? DbConstants.DB_VERB_REP_LEASE : 0;
                ret |= ReplicationMisc ? DbConstants.DB_VERB_REP_MISC : 0;
                ret |= ReplicationMessages ? DbConstants.DB_VERB_REP_MSGS : 0;
                ret |= ReplicationSync ? DbConstants.DB_VERB_REP_SYNC : 0;
                ret |= ReplicationTest ? DbConstants.DB_VERB_REP_TEST : 0;
                ret |= WaitsForTable ? DbConstants.DB_VERB_WAITSFOR : 0;
                return ret;
            }
        }
        internal uint MessagesOff {
            get{
                uint ret = 0;
                ret |= Deadlock ? 0 : DbConstants.DB_VERB_DEADLOCK;
                ret |= FileOps ? 0 : DbConstants.DB_VERB_FILEOPS;
                ret |= AllFileOps ? 0 : DbConstants.DB_VERB_FILEOPS_ALL;
                ret |= Recovery ? 0 : DbConstants.DB_VERB_RECOVERY;
                ret |= Register ? 0 : DbConstants.DB_VERB_REGISTER;
                ret |= Replication ? 0 : DbConstants.DB_VERB_REPLICATION;
                ret |= RepMgrConnectionFailure ? 0 : DbConstants.DB_VERB_REPMGR_CONNFAIL;
                ret |= RepMgrMisc ? 0 : DbConstants.DB_VERB_REPMGR_MISC;
                ret |= ReplicationElection ? 0 : DbConstants.DB_VERB_REP_ELECT;
                ret |= ReplicationLease ? 0 : DbConstants.DB_VERB_REP_LEASE;
                ret |= ReplicationMisc ? 0 : DbConstants.DB_VERB_REP_MISC;
                ret |= ReplicationMessages ? 0 : DbConstants.DB_VERB_REP_MSGS;
                ret |= ReplicationSync ? 0 : DbConstants.DB_VERB_REP_SYNC;
                ret |= ReplicationTest ? 0 : DbConstants.DB_VERB_REP_TEST;
                ret |= WaitsForTable ? 0 : DbConstants.DB_VERB_WAITSFOR;
                return ret;
            }
        }

        internal static VerboseMessages FromFlags(uint flags) {
            VerboseMessages ret = new VerboseMessages();

            ret.Deadlock = ((flags & DbConstants.DB_VERB_DEADLOCK) != 0);
            ret.FileOps = ((flags & DbConstants.DB_VERB_FILEOPS) != 0);
            ret.AllFileOps = ((flags & DbConstants.DB_VERB_FILEOPS_ALL) != 0);
            ret.Recovery = ((flags & DbConstants.DB_VERB_RECOVERY) != 0);
            ret.Register = ((flags & DbConstants.DB_VERB_REGISTER) != 0);
            ret.Replication = ((flags & DbConstants.DB_VERB_REPLICATION) != 0);
            ret.RepMgrConnectionFailure = ((flags & DbConstants.DB_VERB_REPMGR_CONNFAIL) != 0);
            ret.RepMgrMisc = ((flags & DbConstants.DB_VERB_REPMGR_MISC) != 0);
            ret.ReplicationElection = ((flags & DbConstants.DB_VERB_REP_ELECT) != 0);
            ret.ReplicationLease = ((flags & DbConstants.DB_VERB_REP_LEASE) != 0);
            ret.ReplicationMisc = ((flags & DbConstants.DB_VERB_REP_MISC) != 0);
            ret.ReplicationMessages = ((flags & DbConstants.DB_VERB_REP_MSGS) != 0);
            ret.ReplicationSync = ((flags & DbConstants.DB_VERB_REP_SYNC) != 0);
            ret.ReplicationTest = ((flags & DbConstants.DB_VERB_REP_TEST) != 0);
            ret.WaitsForTable = ((flags & DbConstants.DB_VERB_WAITSFOR) != 0);
            
            return ret;
        }
    }
}
