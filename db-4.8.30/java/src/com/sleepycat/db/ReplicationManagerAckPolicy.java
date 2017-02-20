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
A class that provides definitions for the types of network ack policyto use
when transmitting messages between replication sites using theReplication
Manager.
<p>
Set using the {@link com.sleepycat.db.EnvironmentConfig#setReplicationManagerAckPolicy EnvironmentConfig.setReplicationManagerAckPolicy} API.
*/
public final class ReplicationManagerAckPolicy {

    /**
    The master should wait until all replication clients have acknowledged
    each permanent replication message.
    */
    public static final ReplicationManagerAckPolicy ALL =
        new ReplicationManagerAckPolicy("ALL", DbConstants.DB_REPMGR_ACKS_ALL);

    /**
    The master should wait until all electable peers have acknowledged
    each permanent replication message (where "electable peer" means a
    client capable of being subsequently elected master of the
    replication group).
    */
    public static final ReplicationManagerAckPolicy ALL_PEERS =
        new ReplicationManagerAckPolicy(
        "ALL_PEERS", DbConstants.DB_REPMGR_ACKS_ALL_PEERS);

    /**
    The master should not wait for any client replication message
    acknowledgments.
    */
    public static final ReplicationManagerAckPolicy NONE =
        new ReplicationManagerAckPolicy(
        "NONE", DbConstants.DB_REPMGR_ACKS_NONE);

    /**
    The master should wait until at least one client site has acknowledged
    each permanent replication message.
    */
    public static final ReplicationManagerAckPolicy ONE =
        new ReplicationManagerAckPolicy("ONE", DbConstants.DB_REPMGR_ACKS_ONE);

    /**
    The master should wait until at least one electable peer has acknowledged
    each permanent replication message (where "electable peer" means a client
    capable of being subsequently elected master of the replication group).
    */
    public static final ReplicationManagerAckPolicy ONE_PEER =
        new ReplicationManagerAckPolicy(
            "ONE_PEER", DbConstants.DB_REPMGR_ACKS_ONE_PEER);

    /**
    The master should wait until it has received acknowledgements from the
    minimum number of electable peers sufficient to ensure that the effect
    of the permanent record remains durable if an election is held (where
    "electable peer" means a client capable of being subsequently elected
    master of the replication group). This is the default acknowledgement
    policy.
    */
    public static final ReplicationManagerAckPolicy QUORUM =
        new ReplicationManagerAckPolicy(
            "QUORUM", DbConstants.DB_REPMGR_ACKS_QUORUM);

    /* package */
    static ReplicationManagerAckPolicy fromInt(int type) {
        switch(type) {
        case DbConstants.DB_REPMGR_ACKS_ALL:
            return ALL;
        case DbConstants.DB_REPMGR_ACKS_ALL_PEERS:
            return ALL_PEERS;
        case DbConstants.DB_REPMGR_ACKS_NONE:
            return NONE;
        case DbConstants.DB_REPMGR_ACKS_ONE:
            return ONE;
        case DbConstants.DB_REPMGR_ACKS_ONE_PEER:
            return ONE_PEER;
        case DbConstants.DB_REPMGR_ACKS_QUORUM:
            return QUORUM;
        default:
            throw new IllegalArgumentException(
                "Unknown ACK policy: " + type);
        }
    }

    private String statusName;
    private int id;

    private ReplicationManagerAckPolicy(final String statusName, final int id) {
        this.statusName = statusName;
        this.id = id;
    }

    /* package */
    int getId() {
        return id;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "ReplicationManagerAckPolicy." + statusName;
    }
}

