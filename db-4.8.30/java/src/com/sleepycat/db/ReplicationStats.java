/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
Replication statistics for a database environment.
*/
public class ReplicationStats {
    // no public constructor
    /* package */ ReplicationStats() {}
    /**
    The environment is configured as a replication client, as reported by {@link #getStatus}.
    */
    public static final int REP_CLIENT = DbConstants.DB_REP_CLIENT;

    /**
    The environment is configured as a replication master, as reported by {@link #getStatus}.
    */
    public static final int REP_MASTER = DbConstants.DB_REP_MASTER;

    /**
    Replication is not configured for this environment, as reported by {@link #getStatus}.
    */
    public static final int REP_NONE = 0;

    private long st_log_queued;
    /** TODO */
    public long getLogQueued() {
        return st_log_queued;
    }

    private int st_startup_complete;
    /** TODO */
    public boolean getStartupComplete() {
        return (st_startup_complete != 0);
    }

    private int st_status;
    /** TODO */
    public int getStatus() {
        return st_status;
    }

    private LogSequenceNumber st_next_lsn;
    /** TODO */
    public LogSequenceNumber getNextLsn() {
        return st_next_lsn;
    }

    private LogSequenceNumber st_waiting_lsn;
    /** TODO */
    public LogSequenceNumber getWaitingLsn() {
        return st_waiting_lsn;
    }

    private LogSequenceNumber st_max_perm_lsn;
    /** TODO */
    public LogSequenceNumber getMaxPermLsn() {
        return st_max_perm_lsn;
    }

    private int st_next_pg;
    /** TODO */
    public int getNextPages() {
        return st_next_pg;
    }

    private int st_waiting_pg;
    /** TODO */
    public int getWaitingPages() {
        return st_waiting_pg;
    }

    private int st_dupmasters;
    /** TODO */
    public int getDupmasters() {
        return st_dupmasters;
    }

    private int st_env_id;
    /** TODO */
    public int getEnvId() {
        return st_env_id;
    }

    private int st_env_priority;
    /** TODO */
    public int getEnvPriority() {
        return st_env_priority;
    }

    private long st_bulk_fills;
    /** TODO */
    public long getBulkFills() {
        return st_bulk_fills;
    }

    private long st_bulk_overflows;
    /** TODO */
    public long getBulkOverflows() {
        return st_bulk_overflows;
    }

    private long st_bulk_records;
    /** TODO */
    public long getBulkRecords() {
        return st_bulk_records;
    }

    private long st_bulk_transfers;
    /** TODO */
    public long getBulkTransfers() {
        return st_bulk_transfers;
    }

    private long st_client_rerequests;
    /** TODO */
    public long getClientRerequests() {
        return st_client_rerequests;
    }

    private long st_client_svc_req;
    /** TODO */
    public long getClientSvcReq() {
        return st_client_svc_req;
    }

    private long st_client_svc_miss;
    /** TODO */
    public long getClientSvcMiss() {
        return st_client_svc_miss;
    }

    private int st_gen;
    /** TODO */
    public int getGen() {
        return st_gen;
    }

    private int st_egen;
    /** TODO */
    public int getEgen() {
        return st_egen;
    }

    private long st_log_duplicated;
    /** TODO */
    public long getLogDuplicated() {
        return st_log_duplicated;
    }

    private long st_log_queued_max;
    /** TODO */
    public long getLogQueuedMax() {
        return st_log_queued_max;
    }

    private long st_log_queued_total;
    /** TODO */
    public long getLogQueuedTotal() {
        return st_log_queued_total;
    }

    private long st_log_records;
    /** TODO */
    public long getLogRecords() {
        return st_log_records;
    }

    private long st_log_requested;
    /** TODO */
    public long getLogRequested() {
        return st_log_requested;
    }

    private int st_master;
    /** TODO */
    public int getMaster() {
        return st_master;
    }

    private long st_master_changes;
    /** TODO */
    public long getMasterChanges() {
        return st_master_changes;
    }

    private long st_msgs_badgen;
    /** TODO */
    public long getMsgsBadgen() {
        return st_msgs_badgen;
    }

    private long st_msgs_processed;
    /** TODO */
    public long getMsgsProcessed() {
        return st_msgs_processed;
    }

    private long st_msgs_recover;
    /** TODO */
    public long getMsgsRecover() {
        return st_msgs_recover;
    }

    private long st_msgs_send_failures;
    /** TODO */
    public long getMsgsSendFailures() {
        return st_msgs_send_failures;
    }

    private long st_msgs_sent;
    /** TODO */
    public long getMsgsSent() {
        return st_msgs_sent;
    }

    private long st_newsites;
    /** TODO */
    public long getNewsites() {
        return st_newsites;
    }

    private int st_nsites;
    /** TODO */
    public int getNumSites() {
        return st_nsites;
    }

    private long st_nthrottles;
    /** TODO */
    public long getNumThrottles() {
        return st_nthrottles;
    }

    private long st_outdated;
    /** TODO */
    public long getOutdated() {
        return st_outdated;
    }

    private long st_pg_duplicated;
    /** TODO */
    public long getPagesDuplicated() {
        return st_pg_duplicated;
    }

    private long st_pg_records;
    /** TODO */
    public long getPagesRecords() {
        return st_pg_records;
    }

    private long st_pg_requested;
    /** TODO */
    public long getPagesRequested() {
        return st_pg_requested;
    }

    private long st_txns_applied;
    /** TODO */
    public long getTxnsApplied() {
        return st_txns_applied;
    }

    private long st_startsync_delayed;
    /** TODO */
    public long getStartSyncDelayed() {
        return st_startsync_delayed;
    }

    private long st_elections;
    /** TODO */
    public long getElections() {
        return st_elections;
    }

    private long st_elections_won;
    /** TODO */
    public long getElectionsWon() {
        return st_elections_won;
    }

    private int st_election_cur_winner;
    /** TODO */
    public int getElectionCurWinner() {
        return st_election_cur_winner;
    }

    private int st_election_gen;
    /** TODO */
    public int getElectionGen() {
        return st_election_gen;
    }

    private LogSequenceNumber st_election_lsn;
    /** TODO */
    public LogSequenceNumber getElectionLsn() {
        return st_election_lsn;
    }

    private int st_election_nsites;
    /** TODO */
    public int getElectionNumSites() {
        return st_election_nsites;
    }

    private int st_election_nvotes;
    /** TODO */
    public int getElectionNumVotes() {
        return st_election_nvotes;
    }

    private int st_election_priority;
    /** TODO */
    public int getElectionPriority() {
        return st_election_priority;
    }

    private int st_election_status;
    /** TODO */
    public int getElectionStatus() {
        return st_election_status;
    }

    private int st_election_tiebreaker;
    /** TODO */
    public int getElectionTiebreaker() {
        return st_election_tiebreaker;
    }

    private int st_election_votes;
    /** TODO */
    public int getElectionVotes() {
        return st_election_votes;
    }

    private int st_election_sec;
    /** TODO */
    public int getElectionSec() {
        return st_election_sec;
    }

    private int st_election_usec;
    /** TODO */
    public int getElectionUsec() {
        return st_election_usec;
    }

    private int st_max_lease_sec;
    /** TODO */
    public int getMaxLeaseSec() {
        return st_max_lease_sec;
    }

    private int st_max_lease_usec;
    /** TODO */
    public int getMaxLeaseUsec() {
        return st_max_lease_usec;
    }

    /**
    For convenience, the ReplicationStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "ReplicationStats:"
            + "\n  st_log_queued=" + st_log_queued
            + "\n  st_startup_complete=" + (st_startup_complete != 0)
            + "\n  st_status=" + st_status
            + "\n  st_next_lsn=" + st_next_lsn
            + "\n  st_waiting_lsn=" + st_waiting_lsn
            + "\n  st_max_perm_lsn=" + st_max_perm_lsn
            + "\n  st_next_pg=" + st_next_pg
            + "\n  st_waiting_pg=" + st_waiting_pg
            + "\n  st_dupmasters=" + st_dupmasters
            + "\n  st_env_id=" + st_env_id
            + "\n  st_env_priority=" + st_env_priority
            + "\n  st_bulk_fills=" + st_bulk_fills
            + "\n  st_bulk_overflows=" + st_bulk_overflows
            + "\n  st_bulk_records=" + st_bulk_records
            + "\n  st_bulk_transfers=" + st_bulk_transfers
            + "\n  st_client_rerequests=" + st_client_rerequests
            + "\n  st_client_svc_req=" + st_client_svc_req
            + "\n  st_client_svc_miss=" + st_client_svc_miss
            + "\n  st_gen=" + st_gen
            + "\n  st_egen=" + st_egen
            + "\n  st_log_duplicated=" + st_log_duplicated
            + "\n  st_log_queued_max=" + st_log_queued_max
            + "\n  st_log_queued_total=" + st_log_queued_total
            + "\n  st_log_records=" + st_log_records
            + "\n  st_log_requested=" + st_log_requested
            + "\n  st_master=" + st_master
            + "\n  st_master_changes=" + st_master_changes
            + "\n  st_msgs_badgen=" + st_msgs_badgen
            + "\n  st_msgs_processed=" + st_msgs_processed
            + "\n  st_msgs_recover=" + st_msgs_recover
            + "\n  st_msgs_send_failures=" + st_msgs_send_failures
            + "\n  st_msgs_sent=" + st_msgs_sent
            + "\n  st_newsites=" + st_newsites
            + "\n  st_nsites=" + st_nsites
            + "\n  st_nthrottles=" + st_nthrottles
            + "\n  st_outdated=" + st_outdated
            + "\n  st_pg_duplicated=" + st_pg_duplicated
            + "\n  st_pg_records=" + st_pg_records
            + "\n  st_pg_requested=" + st_pg_requested
            + "\n  st_txns_applied=" + st_txns_applied
            + "\n  st_startsync_delayed=" + st_startsync_delayed
            + "\n  st_elections=" + st_elections
            + "\n  st_elections_won=" + st_elections_won
            + "\n  st_election_cur_winner=" + st_election_cur_winner
            + "\n  st_election_gen=" + st_election_gen
            + "\n  st_election_lsn=" + st_election_lsn
            + "\n  st_election_nsites=" + st_election_nsites
            + "\n  st_election_nvotes=" + st_election_nvotes
            + "\n  st_election_priority=" + st_election_priority
            + "\n  st_election_status=" + st_election_status
            + "\n  st_election_tiebreaker=" + st_election_tiebreaker
            + "\n  st_election_votes=" + st_election_votes
            + "\n  st_election_sec=" + st_election_sec
            + "\n  st_election_usec=" + st_election_usec
            + "\n  st_max_lease_sec=" + st_max_lease_sec
            + "\n  st_max_lease_usec=" + st_max_lease_usec
            ;
    }
}
