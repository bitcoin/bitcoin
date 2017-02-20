/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db.repquote;

import com.sleepycat.db.*;

/*
 * A simple wrapper class, that facilitates storing some
 * custom information with an Environment object.
 * The information is used by the Replication callback (handleEvent).
 */
public class RepQuoteEnvironment extends Environment
{
    private boolean appFinished;
    private boolean inClientSync;
    private boolean isMaster;

    public RepQuoteEnvironment(final java.io.File host,
        EnvironmentConfig config)
        throws DatabaseException, java.io.FileNotFoundException
    {
        super(host, config);
        appFinished = false;
        inClientSync = false;
        isMaster = false;
    }

    boolean getAppFinished()
    {
        return appFinished;
    }
    public void setAppFinished(boolean appFinished)
    {
        this.appFinished = appFinished;
    }
    boolean getInClientSync()
    {
        return inClientSync;
    }
    public void setInClientSync(boolean inClientSync)
    {
        this.inClientSync = inClientSync;
    }
    boolean getIsMaster()
    {
        return isMaster;
    }
    public void setIsMaster(boolean isMaster)
    {
        this.isMaster = isMaster;
    }
}

