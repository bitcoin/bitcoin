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
This class provides definitions of the various start policies thatcan be specified when starting a replication client using the{@link com.sleepycat.db.Environment#replicationManagerStart Environment.replicationManagerStart} call.
*/
public final class ReplicationManagerStartPolicy {

    /**
    Start as a master site, and do not call for an election. Note there must
    never be more than a single master in any replication group, and only one
    site should ever be started with the DB_REP_MASTER flag specified.
    */
    public static final ReplicationManagerStartPolicy REP_MASTER =
        new ReplicationManagerStartPolicy(
        "REP_MASTER", DbConstants.DB_REP_MASTER);

    /**
    Start as a client site, and do not call for an election.
    */
    public static final ReplicationManagerStartPolicy REP_CLIENT =
        new ReplicationManagerStartPolicy(
        "REP_CLIENT", DbConstants.DB_REP_CLIENT);

    /**
    Start as a client, and call for an election if no master is found.
    */
    public static final ReplicationManagerStartPolicy REP_ELECTION =
        new ReplicationManagerStartPolicy(
        "REP_ELECTION", DbConstants.DB_REP_ELECTION);

    /* package */
    static ReplicationManagerStartPolicy fromInt(int type) {
        switch(type) {
        case DbConstants.DB_REP_MASTER:
            return REP_MASTER;
        case DbConstants.DB_REP_CLIENT:
            return REP_CLIENT;
        case DbConstants.DB_REP_ELECTION:
            return REP_ELECTION;
        default:
            throw new IllegalArgumentException(
                "Unknown rep start policy: " + type);
        }
    }

    private String statusName;
    private int id;

    private ReplicationManagerStartPolicy(final String statusName,
        final int id) {

        this.statusName = statusName;
        this.id = id;
    }

    /* package */
    int getId() {
        return id;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "ReplicationManagerStartPolicy." + statusName;
    }
}

