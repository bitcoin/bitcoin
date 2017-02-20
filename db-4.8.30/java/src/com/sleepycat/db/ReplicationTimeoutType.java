/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
The ReplicationTimeoutType defines the types of timeouts that can beconfigured for the Berkeley Db replication functionality.
<p>
The class is used in the {@link com.sleepycat.db.Environment#setReplicationTimeout Environment.setReplicationTimeout}method.
*/
public final class ReplicationTimeoutType {

    /**
    Configure the amount of time the replication manager's transport function
    waits to collect enough acknowledgments from replication group clients,
    before giving up and returning a failure indication.
    */
    public static final ReplicationTimeoutType ACK_TIMEOUT =
        new ReplicationTimeoutType("ACK_TIMEOUT", DbConstants.DB_REP_ACK_TIMEOUT);

    /**
    Configure the amount of time the replication manager will delay between
    completing a checkpoint and writing a checkpoint record into the log. This
    delay allows clients to complete their own checkpoints before the master
    requires completion of them. The default is 30 seconds.
    */
    public static final ReplicationTimeoutType CHECKPOINT_DELAY =
        new ReplicationTimeoutType("CHECKPOINT_DELAY", DbConstants.DB_REP_CHECKPOINT_DELAY);

    /**
    Configure the amount of time the replication manager will wait before
    trying to re-establish a connection to another site after a
    communication failure.
    */
    public static final ReplicationTimeoutType CONNECTION_RETRY =
        new ReplicationTimeoutType("CONNECTION_RETRY", DbConstants.DB_REP_CONNECTION_RETRY);

    /**
    The timeout period for an election.
    */
    public static final ReplicationTimeoutType ELECTION_TIMEOUT =
        new ReplicationTimeoutType("ELECTION_TIMEOUT", DbConstants.DB_REP_ELECTION_TIMEOUT);

    /**
    Configure the amount of time the replication manager will wait before
    retrying a failed election.
    */
    public static final ReplicationTimeoutType ELECTION_RETRY =
        new ReplicationTimeoutType("ELECTION_RETRY", DbConstants.DB_REP_ELECTION_RETRY);

    /**
    The amount of time the replication manager, running at a client site,
    waits for some message activity on the connection from the master
    (heartbeats or other messages) before concluding that the connection
    has been lost.  When 0 (the default), no monitoring is performed.
    */
    public static final ReplicationTimeoutType HEARTBEAT_MONITOR =
        new ReplicationTimeoutType("HEARTBEAT_MONITOR", DbConstants.DB_REP_HEARTBEAT_MONITOR);

    /** 
    The frequency at which the replication manager, running at a master site,
    broadcasts a heartbeat message in an otherwise idle system.  When 0
    (the default), no heartbeat messages will be sent.	
    */
    public static final ReplicationTimeoutType HEARTBEAT_SEND =
        new ReplicationTimeoutType("HEARTBEAT_SEND", DbConstants.DB_REP_HEARTBEAT_SEND);

    /**
    An optional configuration timeout period to wait for full election
    participation the first time the replication group finds a master. By
    default this option is turned off and normal election timeouts are used.
    (See the
    <a href="{@docRoot}/../programmer_reference/rep_elect.html">Elections</a>
    section in the Berkeley DB Reference Guide for more information.)
    */
    public static final ReplicationTimeoutType FULL_ELECTION_TIMEOUT =
        new ReplicationTimeoutType("FULL_ELECTION_TIMEOUT", DbConstants.DB_REP_FULL_ELECTION_TIMEOUT);

    /* package */
    static ReplicationTimeoutType fromInt(int type) {
        switch(type) {
        case DbConstants.DB_REP_ACK_TIMEOUT:
            return ACK_TIMEOUT;
        case DbConstants.DB_REP_ELECTION_TIMEOUT:
            return ELECTION_TIMEOUT;
        case DbConstants.DB_REP_ELECTION_RETRY:
            return ELECTION_RETRY;
        case DbConstants.DB_REP_CONNECTION_RETRY:
            return CONNECTION_RETRY;
        default:
            throw new IllegalArgumentException(
                "Unknown timeout type: " + type);
        }
    }

    private String statusName;
    private int id;

    private ReplicationTimeoutType(final String statusName, final int id) {
        this.statusName = statusName;
        this.id = id;
    }

    /* package */
    int getId() {
        return id;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "ReplicationTimeoutType." + statusName;
    }
}
