/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;

/** The return status from processing a replication message. */
public final class ReplicationStatus {
    static final ReplicationStatus SUCCESS =
        new ReplicationStatus("SUCCESS", 0);

    private int errCode;
    private DatabaseEntry cdata;
    private int envid;
    private LogSequenceNumber lsn;

    /* For toString */
    private String statusName;

    private ReplicationStatus(final String statusName,
                              final int errCode,
                              final DatabaseEntry cdata,
                              final int envid,
                              final LogSequenceNumber lsn) {
        this.statusName = statusName;
        this.errCode = errCode;
        this.cdata = cdata;
        this.envid = envid;
        this.lsn = lsn;
    }

    private ReplicationStatus(final String statusName, final int errCode) {
        this(statusName, errCode, null, 0, null);
    }

    /**
    The operation succeeded.
    */
    public boolean isSuccess() {
        return errCode == 0;
    }

    /**
    This message cannot be processed.
    This is an indication that this message is irrelevant to the current
    replication state (for example, an old message from a previous
    generation arrives and is processed late).
    **/
    public boolean isIgnore() {
        return errCode == DbConstants.DB_REP_IGNORE;
    }

    /**
    Processing this message resulted in the processing of records that
    are permanent.  The maximum LSN of the permanent records stored is
    available from the getLSN method.
    */
    public boolean isPermanent() {
        return errCode == DbConstants.DB_REP_ISPERM;
    }

    /**
    The system received contact information from a new environment.  A
    copy of the opaque data specified in the cdata parameter to the
    {@link com.sleepycat.db.Environment#startReplication Environment.startReplication} is available from the
    getCDAta method.  The application should take whatever action is
    needed to establish a communication channel with this new
    environment.
    */
    public boolean isNewSite() {
        return errCode == DbConstants.DB_REP_NEWSITE;
    }

    /**
    A message carrying a DB_REP_PERMANENT flag was processed successfully,
    but was not written to disk.  The LSN of this record is available from
    the getLSN method.  The application should take whatever action is
    deemed necessary to retain its recoverability characteristics.
    */
    public boolean isNotPermanent() {
        return errCode == DbConstants.DB_REP_NOTPERM;
    }

    /**
    Whenever the system receives contact information from a new
    environment, a copy of the opaque data specified in the cdata
    parameter to the {@link com.sleepycat.db.Environment#startReplication Environment.startReplication} is available
    from the getCDAta method.  The application should take whatever
    action is needed to establish a communication channel with this new
    environment.
    */
    public DatabaseEntry getCData() {
        return cdata;
    }

    /**
    Return the environment ID associated with the operation.  In most cases,
    this is the same as the environment ID passed to {@link com.sleepycat.db.Environment#processReplicationMessage Environment.processReplicationMessage}.  However, if a new master is elected, this
    method returns the environment ID of the new master.  It is the
    application's responsibility to ensure that the matching node begins acting
    as the master environment.
    */
    public int getEnvID() {
        return envid;
    }

    /**
    Whenever processing a messages results in the processing of messages
    that are permanent, or a message carrying a DB_REP_PERMANENT flag
    was processed successfully, but was not written to disk, the LSN of
    the record is available from the getLSN method.  The application
    should take whatever action is deemed necessary to retain its
    recoverability characteristics.
    */
    public LogSequenceNumber getLSN() {
        return lsn;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "ReplicationStatus." + statusName;
    }

    /* package */
    static ReplicationStatus getStatus(final int errCode,
                                       final DatabaseEntry cdata,
                                       final int envid,
                                       final LogSequenceNumber lsn) {
        switch(errCode) {
        case 0:
            return SUCCESS;
        case DbConstants.DB_REP_IGNORE:
            return IGNORE;
        case DbConstants.DB_REP_ISPERM:
            return new ReplicationStatus("ISPERM", errCode, cdata, envid, lsn);
        case DbConstants.DB_REP_NEWSITE:
            return new ReplicationStatus("NEWSITE", errCode, cdata, envid, lsn);
        case DbConstants.DB_REP_NOTPERM:
            return new ReplicationStatus("NOTPERM", errCode, cdata, envid, lsn);
        default:
            throw new IllegalArgumentException(
                "Unknown error code: " + DbEnv.strerror(errCode));
        }
    }

    private static final ReplicationStatus IGNORE =
        new ReplicationStatus("IGNORE", DbConstants.DB_REP_IGNORE);
}
