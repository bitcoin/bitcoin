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

/** Specifies the attributes of a verification operation. */
public final class VerboseConfig {
    /**
    Display additional information when doing deadlock detection.
    */
    public static final VerboseConfig DEADLOCK =
        new VerboseConfig("DEADLOCK", DbConstants.DB_VERB_DEADLOCK);
    /**
    Display additional information when performing filesystem operations such
    as open, close or rename. May not be available on all platforms.
    */
    public static final VerboseConfig FILEOPS =
        new VerboseConfig("FILEOPS", DbConstants.DB_VERB_FILEOPS);
    /**
    Display additional information when performing all filesystem operations,
    including read and write. May not be available on all platforms.
    */
    public static final VerboseConfig FILEOPS_ALL =
        new VerboseConfig("FILEOPS_ALL", DbConstants.DB_VERB_FILEOPS_ALL);
    /**
    Display additional information when performing recovery.
    */
    public static final VerboseConfig RECOVERY =
        new VerboseConfig("RECOVERY", DbConstants.DB_VERB_RECOVERY);
    /**
    Display additional information concerning support for {@link
    EnvironmentConfig#setRegister}.
    */
    public static final VerboseConfig REGISTER =
        new VerboseConfig("REGISTER", DbConstants.DB_VERB_REGISTER);
    /**
    Display all detailed information about replication.  This includes the
    information displayed by all of the other REPLICATION_* and REPMGR_*
    values.
    */
    public static final VerboseConfig REPLICATION =
        new VerboseConfig("REPLICATION", DbConstants.DB_VERB_REPLICATION);
    /**
    Display detailed information about Replication Manager connection failures.
    */
    public static final VerboseConfig REPMGR_CONNFAIL =
        new VerboseConfig("REPLICATIONMGR_CONNFAIL", DbConstants.DB_VERB_REPMGR_CONNFAIL);
    /**
    Display detailed information about genereal Replication Manager processing.
    */
    public static final VerboseConfig REPMGR_MISC =
        new VerboseConfig("REPLICATIONMGR_MISC", DbConstants.DB_VERB_REPMGR_MISC);
    /**
    Display detailed information about replication elections.
    */
    public static final VerboseConfig REPLICATION_ELECTION =
        new VerboseConfig("REPLICATION_ELECTION", DbConstants.DB_VERB_REP_ELECT);
    /**
    Display detailed information about replication master leases.
    */
    public static final VerboseConfig REPLICATION_LEASE =
        new VerboseConfig("REPLICATION_LEASE", DbConstants.DB_VERB_REP_LEASE);
    /**
    Display detailed information about general replication processing not
    covered by the other REPLICATION_* values.
    */
    public static final VerboseConfig REPLICATION_MISC =
        new VerboseConfig("REPLICATION_MISC", DbConstants.DB_VERB_REP_MISC);
    /**
    Display detailed information about replication message processing.
    */
    public static final VerboseConfig REPLICATION_MSGS =
        new VerboseConfig("REPLICATION_MSGS", DbConstants.DB_VERB_REP_MSGS);
    /**
    Display detailed information about replication client synchronization.
    */
    public static final VerboseConfig REPLICATION_SYNC =
        new VerboseConfig("REPLICATION_SYNC", DbConstants.DB_VERB_REP_SYNC);
    /**
    Display temporary replication test information.
    */
    public static final VerboseConfig REPLICATION_TEST =
        new VerboseConfig("REPLICATION_TEST", DbConstants.DB_VERB_REP_TEST);
    /**
    Display the waits-for table when doing deadlock detection.
    */
    public static final VerboseConfig WAITSFOR =
        new VerboseConfig("WAITSFOR", DbConstants.DB_VERB_WAITSFOR);

    /* Package */
    int getInternalFlag() {
        return verboseFlag;
    }
    /* For toString */
    private String verboseName;
    private int verboseFlag;

    private VerboseConfig(final String verboseName, int verboseFlag) {
        this.verboseName = verboseName;
        this.verboseFlag = verboseFlag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "VerboseConfig." + verboseName;
    }
}

