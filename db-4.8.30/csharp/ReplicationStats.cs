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
    /// Statistical information about the replication subsystem
    /// </summary>
    public class ReplicationStats {
        private Internal.ReplicationStatStruct st;
        private LSN next;
        private LSN waiting;
        private LSN maxPerm;
        private LSN winner;
        internal ReplicationStats(Internal.ReplicationStatStruct stats) {
            st = stats;
            next = new LSN(st.st_next_lsn.file, st.st_next_lsn.offset);
            waiting = new LSN(st.st_waiting_lsn.file, st.st_waiting_lsn.offset);
            maxPerm =
                new LSN(st.st_max_perm_lsn.file, st.st_max_perm_lsn.offset);
            winner = new LSN(st.st_election_lsn.file, st.st_election_lsn.offset);

        }

        /// <summary>
        /// Log records currently queued. 
        /// </summary>
        public ulong CurrentQueuedLogRecords { get { return st.st_log_queued; } }
        /// <summary>
        /// Site completed client sync-up. 
        /// </summary>
        public bool ClientStartupComplete { get { return st.st_startup_complete != 0; } }
        /// <summary>
        /// Current replication status. 
        /// </summary>
        public uint Status { get { return st.st_status; } }
        /// <summary>
        /// Next LSN to use or expect. 
        /// </summary>
        public LSN NextLSN { get { return next; } }
        /// <summary>
        /// LSN we're awaiting, if any. 
        /// </summary>
        public LSN AwaitedLSN { get { return waiting; } }
        /// <summary>
        /// Maximum permanent LSN. 
        /// </summary>
        public LSN MaxPermanentLSN { get { return maxPerm; } }
        /// <summary>
        /// Next pg we expect. 
        /// </summary>
        public uint NextPage { get { return st.st_next_pg; } }
        /// <summary>
        /// pg we're awaiting, if any. 
        /// </summary>
        public uint AwaitedPage { get { return st.st_waiting_pg; } }
        /// <summary>
        /// # of times a duplicate master condition was detected.
        /// </summary>
        public uint DupMasters { get { return st.st_dupmasters; } }
        /// <summary>
        /// Current environment ID. 
        /// </summary>
        public int EnvID { get { return st.st_env_id; } }
        /// <summary>
        /// Current environment priority. 
        /// </summary>
        public uint EnvPriority { get { return st.st_env_priority; } }
        /// <summary>
        /// Bulk buffer fills. 
        /// </summary>
        public ulong BulkBufferFills { get { return st.st_bulk_fills; } }
        /// <summary>
        /// Bulk buffer overflows. 
        /// </summary>
        public ulong BulkBufferOverflows { get { return st.st_bulk_overflows; } }
        /// <summary>
        /// Bulk records stored. 
        /// </summary>
        public ulong BulkRecordsStored { get { return st.st_bulk_records; } }
        /// <summary>
        /// Transfers of bulk buffers. 
        /// </summary>
        public ulong BulkBufferTransfers { get { return st.st_bulk_transfers; } }
        /// <summary>
        /// Number of forced rerequests. 
        /// </summary>
        public ulong ForcedRerequests { get { return st.st_client_rerequests; } }
        /// <summary>
        /// Number of client service requests received by this client.
        /// </summary>
        public ulong ClientServiceRequests { get { return st.st_client_svc_req; } }
        /// <summary>
        /// Number of client service requests missing on this client.
        /// </summary>
        public ulong ClientServiceRequestsMissing { get { return st.st_client_svc_miss; } }
        /// <summary>
        /// Current generation number. 
        /// </summary>
        public uint CurrentGenerationNumber { get { return st.st_gen; } }
        /// <summary>
        /// Current election gen number. 
        /// </summary>
        public uint CurrentElectionGenerationNumber { get { return st.st_egen; } }
        /// <summary>
        /// Log records received multiply. 
        /// </summary>
        public ulong DuplicateLogRecords { get { return st.st_log_duplicated; } }
        /// <summary>
        /// Max. log records queued at once. 
        /// </summary>
        public ulong MaxQueuedLogRecords { get { return st.st_log_queued_max; } }
        /// <summary>
        /// Total # of log recs. ever queued. 
        /// </summary>
        public ulong QueuedLogRecords { get { return st.st_log_queued_total; } }
        /// <summary>
        /// Log records received and put. 
        /// </summary>
        public ulong ReceivedLogRecords { get { return st.st_log_records; } }
        /// <summary>
        /// Log recs. missed and requested. 
        /// </summary>
        public ulong MissedLogRecords { get { return st.st_log_requested; } }
        /// <summary>
        /// Env. ID of the current master. 
        /// </summary>
        public int MasterEnvID { get { return st.st_master; } }
        /// <summary>
        /// # of times we've switched masters. 
        /// </summary>
        public ulong MasterChanges { get { return st.st_master_changes; } }
        /// <summary>
        /// Messages with a bad generation #. 
        /// </summary>
        public ulong BadGenerationMessages { get { return st.st_msgs_badgen; } }
        /// <summary>
        /// Messages received and processed. 
        /// </summary>
        public ulong ReceivedMessages { get { return st.st_msgs_processed; } }
        /// <summary>
        /// Messages ignored because this site was a client in recovery.
        /// </summary>
        public ulong IgnoredMessages { get { return st.st_msgs_recover; } }
        /// <summary>
        /// # of failed message sends. 
        /// </summary>
        public ulong FailedMessageSends { get { return st.st_msgs_send_failures; } }
        /// <summary>
        /// # of successful message sends. 
        /// </summary>
        public ulong MessagesSent { get { return st.st_msgs_sent; } }
        /// <summary>
        /// # of NEWSITE msgs. received. 
        /// </summary>
        public ulong NewSiteMessages { get { return st.st_newsites; } }
        /// <summary>
        /// Current number of sites we will assume during elections.
        /// </summary>        
        public uint Sites { get { return st.st_nsites; } }
        /// <summary>
        /// # of times we were throttled. 
        /// </summary>
        public ulong Throttled { get { return st.st_nthrottles; } }
        /// <summary>
        /// # of times we detected and returned an OUTDATED condition.
        /// </summary>
        public ulong Outdated { get { return st.st_outdated; } }
        /// <summary>
        /// Pages received multiply. 
        /// </summary>
        public ulong DuplicatePages { get { return st.st_pg_duplicated; } }
        /// <summary>
        /// Pages received and stored. 
        /// </summary>
        public ulong ReceivedPages { get { return st.st_pg_records; } }
        /// <summary>
        /// Pages missed and requested. 
        /// </summary>
        public ulong MissedPages { get { return st.st_pg_requested; } }
        /// <summary>
        /// # of transactions applied. 
        /// </summary>
        public ulong AppliedTransactions { get { return st.st_txns_applied; } }
        /// <summary>
        /// # of STARTSYNC msgs delayed. 
        /// </summary>
        public ulong StartSyncMessagesDelayed { get { return st.st_startsync_delayed; } }

        /* Elections generally. */
        /// <summary>
        /// # of elections held. 
        /// </summary>
        public ulong Elections { get { return st.st_elections; } }
        /// <summary>
        /// # of elections won by this site. 
        /// </summary>
        public ulong ElectionsWon { get { return st.st_elections_won; } }

        /* Statistics about an in-progress election. */
        /// <summary>
        /// Current front-runner. 
        /// </summary>
        public int CurrentWinner { get { return st.st_election_cur_winner; } }
        /// <summary>
        /// Election generation number. 
        /// </summary>
        public uint ElectionGenerationNumber { get { return st.st_election_gen; } }
        /// <summary>
        /// Max. LSN of current winner. 
        /// </summary>
        public LSN CurrentWinnerMaxLSN { get { return winner; } }
        /// <summary>
        /// # of "registered voters". 
        /// </summary>
        public uint RegisteredSites { get { return st.st_election_nsites; } }
        /// <summary>
        /// # of "registered voters" needed. 
        /// </summary>
        public uint RegisteredSitesNeeded { get { return st.st_election_nvotes; } }
        /// <summary>
        /// Current election priority. 
        /// </summary>
        public uint ElectionPriority { get { return st.st_election_priority; } }
        /// <summary>
        /// Current election status. 
        /// </summary>
        public int ElectionStatus { get { return st.st_election_status; } }
        /// <summary>
        /// Election tiebreaker value. 
        /// </summary>
        public uint ElectionTiebreaker { get { return st.st_election_tiebreaker; } }
        /// <summary>
        /// Votes received in this round. 
        /// </summary>
        public uint Votes { get { return st.st_election_votes; } }
        /// <summary>
        /// Last election time seconds. 
        /// </summary>
        public uint ElectionTimeSec { get { return st.st_election_sec; } }
        /// <summary>
        /// Last election time useconds. 
        /// </summary>
        public uint ElectionTimeUSec { get { return st.st_election_usec; } }
        /// <summary>
        /// Maximum lease timestamp seconds. 
        /// </summary>
        public uint MaxLeaseSec { get { return st.st_max_lease_sec; } }
        /// <summary>
        /// Maximum lease timestamp useconds. 
        /// </summary>
        public uint MaxLeaseUSec { get { return st.st_max_lease_usec; } }
    }
}