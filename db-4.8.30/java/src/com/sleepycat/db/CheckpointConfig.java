/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbConstants;

/**
Specifies the attributes of an application invoked checkpoint operation.
*/
public class CheckpointConfig {
    /**
    Default configuration used if null is passed to
    {@link com.sleepycat.db.Environment#checkpoint Environment.checkpoint}.
    */
    public static final CheckpointConfig DEFAULT = new CheckpointConfig();

    private boolean force = false;
    private int kbytes = 0;
    private int minutes = 0;

    /**
    An instance created using the default constructor is initialized
    with the system's default settings.
    */
    public CheckpointConfig() {
    }

    /* package */
    static CheckpointConfig checkNull(CheckpointConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    /**
    Configure the checkpoint log data threshold, in kilobytes.
    <p>
    The default is 0 for this class and the database environment.
    <p>
    @param kbytes
    If the kbytes parameter is non-zero, a checkpoint will be performed if more
    than kbytes of log data have been written since the last checkpoint.
    */
    public void setKBytes(final int kbytes) {
        this.kbytes = kbytes;
    }

    /**
Return the checkpoint log data threshold, in kilobytes.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The checkpoint log data threshold, in kilobytes.
    */
    public int getKBytes() {
        return kbytes;
    }

    /**
    Configure the checkpoint time threshold, in minutes.
    <p>
    The default is 0 for this class and the database environment.
    <p>
    @param minutes
    If the minutes parameter is non-zero, a checkpoint is performed if more than
    min minutes have passed since the last checkpoint.
    */
    public void setMinutes(final int minutes) {
        this.minutes = minutes;
    }

    /**
    Return the checkpoint time threshold, in minutes.
    <p>
    @return
    The checkpoint time threshold, in minutes.
    */
    public int getMinutes() {
        return minutes;
    }

    /**
    Configure the checkpoint force option.
    <p>
    The default is false for this class and the BDB JE environment.
    <p>
    @param force
    If set to true, force a checkpoint, even if there has been no
    activity since the last checkpoint.
    */
    public void setForce(final boolean force) {
        this.force = force;
    }

    /**
    Return the configuration of the checkpoint force option.
    <p>
    @return
    The configuration of the checkpoint force option.
    */
    public boolean getForce() {
        return force;
    }

    /* package */
    void runCheckpoint(final DbEnv dbenv)
        throws DatabaseException {

        dbenv.txn_checkpoint(kbytes, minutes, force ? DbConstants.DB_FORCE : 0);
    }
}
