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
    /// A class representing the return value of
    /// <see cref="DatabaseEnvironment.RepProcessMessage"/>.
    /// </summary>
    public class RepProcMsgResult {
        /// <summary>
        /// The result of processing an incoming replication message.
        /// </summary>
        public enum ProcMsgResult {
            /// <summary>
            /// The replication group has more than one master.
            /// </summary>
            /// <remarks>
            /// The application should reconfigure itself as a client by calling
            /// <see cref="DatabaseEnvironment.RepStartClient"/>,
            /// and then call for an election using
            /// <see cref="DatabaseEnvironment.RepHoldElection"/>.
            /// </remarks>
            DUPLICATE_MASTER,
            /// <summary>
            /// An unspecified error occurred.
            /// </summary>
            ERROR,
            /// <summary>
            /// An election is needed.
            /// </summary>
            /// <remarks>
            /// The application should call for an election using
            /// <see cref="DatabaseEnvironment.RepHoldElection"/>. 
            /// </remarks>
            HOLD_ELECTION,
            /// <summary>
            /// A message cannot be processed.
            /// </summary>
            /// <remarks>
            /// This is an indication that a message is irrelevant to the
            /// current replication state (for example, an old message from a
            /// previous generation arrives and is processed late).
            /// </remarks>
            IGNORED,
            /// <summary>
            /// Processing a message resulted in the processing of records that
            /// are permanent.
            /// </summary>
            /// <remarks>
            /// <see cref="RetLsn"/> is the maximum LSN of the permanent
            /// records stored.
            /// </remarks>
            IS_PERMANENT,
            /// <summary>
            /// A new master has been chosen but the client is unable to
            /// synchronize with the new master.
            /// </summary>
            /// <remarks>
            /// Possibly because the client has been configured with
            /// <see cref="ReplicationConfig.NoAutoInit"/> to turn off
            /// automatic internal initialization.
            /// </remarks>
            JOIN_FAILURE,
            /// <summary>
            /// The system received contact information from a new environment.
            /// </summary>
            /// <remarks>
            /// The rec parameter to
            /// <see cref="DatabaseEnvironment.RepProcessMessage"/> contains the
            /// opaque data specified in the cdata parameter to
            /// <see cref="DatabaseEnvironment.RepStartClient"/>. The
            /// application should take whatever action is needed to establish a
            /// communication channel with this new environment.
            /// </remarks>
            NEW_SITE,
            /// <summary>
            /// A message carrying a DB_REP_PERMANENT flag was processed
            /// successfully, but was not written to disk.
            /// </summary>
            /// <remarks>
            /// <see cref="RetLsn"/> is the LSN of this record. The application
            /// should take whatever action is deemed necessary to retain its
            /// recoverability characteristics. 
            /// </remarks>
            NOT_PERMANENT,
            /// <summary>
            /// Processing a message succeded.
            /// </summary>
            SUCCESS
        };

        /// <summary>
        /// The result of processing an incoming replication message.
        /// </summary>
        public ProcMsgResult Result;
        /// <summary>
        /// The log sequence number of the permanent log message that could not
        /// be written to disk if <see cref="Result"/> is
        /// <see cref="ProcMsgResult.NOT_PERMANENT"/>. The largest log
        /// sequence number of the permanent records that are now written to
        /// disk as a result of processing the message, if
        /// <see cref="Result"/> is
        /// <see cref="ProcMsgResult.IS_PERMANENT"/>. In all other cases the
        /// value is undefined. 
        /// </summary>
        public LSN RetLsn;

        internal RepProcMsgResult(int ret, LSN dblsn) {
            RetLsn = null;
            switch (ret) {
                case DbConstants.DB_REP_DUPMASTER:
                    Result = ProcMsgResult.DUPLICATE_MASTER;
                    break;
                case DbConstants.DB_REP_HOLDELECTION:
                    Result = ProcMsgResult.HOLD_ELECTION;
                    break;
                case DbConstants.DB_REP_IGNORE:
                    Result = ProcMsgResult.IGNORED;
                    break;
                case DbConstants.DB_REP_ISPERM:
                    Result = ProcMsgResult.IS_PERMANENT;
                    break;
                case DbConstants.DB_REP_JOIN_FAILURE:
                    Result = ProcMsgResult.JOIN_FAILURE;
                    break;
                case DbConstants.DB_REP_NEWSITE:
                    Result = ProcMsgResult.NEW_SITE;
                    break;
                case DbConstants.DB_REP_NOTPERM:
                    Result = ProcMsgResult.NOT_PERMANENT;
                    break;
                case 0:
                    Result = ProcMsgResult.SUCCESS;
                    break;
                default:
                    Result = ProcMsgResult.ERROR;
                    break;
            }
        }
    }
}
