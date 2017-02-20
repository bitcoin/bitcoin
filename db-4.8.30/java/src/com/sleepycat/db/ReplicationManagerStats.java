/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 */

package com.sleepycat.db;

/**
Replication Manager statistics for a database environment.
*/
public class ReplicationManagerStats {
    // no public constructor
    /* package */ ReplicationManagerStats() {}

    private long st_perm_failed;
    /** TODO */
    public long getPermFailed() {
        return st_perm_failed;
    }

    private long st_msgs_queued;
    /** TODO */
    public long getMsgsQueued() {
        return st_msgs_queued;
    }

    private long st_msgs_dropped;
    /** TODO */
    public long getMsgsDropped() {
        return st_msgs_dropped;
    }

    private long st_connection_drop;
    /** TODO */
    public long getConnectionDrop() {
        return st_connection_drop;
    }

    private long st_connect_fail;
    /** TODO */
    public long getConnectFail() {
        return st_connect_fail;
    }

    /**
    For convenience, the ReplicationManagerStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "ReplicationManagerStats:"
            + "\n  st_perm_failed=" + st_perm_failed
            + "\n  st_msgs_queued=" + st_msgs_queued
            + "\n  st_msgs_dropped=" + st_msgs_dropped
            + "\n  st_connection_drop=" + st_connection_drop
            + "\n  st_connect_fail=" + st_connect_fail
            ;
    }
}
