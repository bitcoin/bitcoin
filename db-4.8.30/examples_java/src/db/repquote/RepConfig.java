/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db.repquote;

import java.util.Vector;

import com.sleepycat.db.ReplicationHostAddress;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationManagerSiteInfo;

public class RepConfig
{
    /* Constant values used in the RepQuote application. */
    public static final String progname = "RepQuoteExample";
    public static final int CACHESIZE = 10 * 1024 * 1024;
    public static final int SLEEPTIME = 5000;

    /* Member variables containing configuration information. */
    public ReplicationManagerAckPolicy ackPolicy;
    public boolean bulk; /* Whether bulk transfer should be performed. */
    public String home; /* The home directory for rep files. */
    public Vector otherHosts; /* Stores an optional set of "other" hosts. */
    public int priority; /* Priority within the replication group. */
    public ReplicationManagerStartPolicy startPolicy;
    public ReplicationHostAddress thisHost; /* Host address to listen to. */
    /* Optional value specifying the # of sites in the replication group. */
    public int totalSites;
    public boolean verbose;

    /* Member variables used internally. */
    private int currOtherHost;
    private boolean gotListenAddress;

    public RepConfig()
    {
        startPolicy = ReplicationManagerStartPolicy.REP_ELECTION;
        home = "";
        gotListenAddress = false;
        totalSites = 0;
        priority = 100;
        verbose = false;
        currOtherHost = 0;
        thisHost = new ReplicationHostAddress();
        otherHosts = new Vector();
        ackPolicy = ReplicationManagerAckPolicy.QUORUM;
        bulk = false;
    }

    public java.io.File getHome()
    {
        return new java.io.File(home);
    }

    public void setThisHost(String host, int port)
    {
        gotListenAddress = true;
        thisHost.port = port;
        thisHost.host = host;
    }

    public ReplicationHostAddress getThisHost()
    {
        if (!gotListenAddress)
            System.err.println("Warning: no host specified, returning default.");
        return thisHost;
    }

    public boolean gotListenAddress() {
        return gotListenAddress;
    }

    public void addOtherHost(String host, int port, boolean peer)
    {
        ReplicationHostAddress newInfo =
            new ReplicationHostAddress(host, port);
        RepRemoteHost newHost = new RepRemoteHost(newInfo, peer);
        otherHosts.add(newHost);
    }

    public RepRemoteHost getFirstOtherHost()
    {
        currOtherHost = 0;
        if (otherHosts.size() == 0)
            return null;
        return (RepRemoteHost)otherHosts.get(currOtherHost);
    }

    public RepRemoteHost getNextOtherHost()
    {
        currOtherHost++;
        if (currOtherHost >= otherHosts.size())
            return null;
        return (RepRemoteHost)otherHosts.get(currOtherHost);
    }

    public RepRemoteHost getOtherHost(int i)
    {
        if (i >= otherHosts.size())
            return null;
        return (RepRemoteHost)otherHosts.get(i);
    }

}

