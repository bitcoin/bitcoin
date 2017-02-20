/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package db.repquote_gsg;

import java.util.Vector;

import com.sleepycat.db.ReplicationHostAddress;
import com.sleepycat.db.ReplicationManagerStartPolicy;

public class RepConfig
{
    // Constant values used in the RepQuote application.
    public static final String progname = "RepQuoteExampleGSG";
    public static final int CACHESIZE = 10 * 1024 * 1024;
    public static final int SLEEPTIME = 5000;

    // Member variables containing configuration information.
    // String specifying the home directory for rep files.
    public String home;
    // Stores an optional set of "other" hosts.
    public Vector<ReplicationHostAddress> otherHosts;
    // Priority within the replication group.
    public int priority; 
    public ReplicationManagerStartPolicy startPolicy;
    // The host address to listen to.
    public ReplicationHostAddress thisHost;
    // Optional parameter specifying the # of sites in the 
    // replication group.
    public int totalSites;

    // Member variables used internally.
    private int currOtherHost;
    private boolean gotListenAddress;

    public RepConfig()
    {
        startPolicy = ReplicationManagerStartPolicy.REP_ELECTION;
        home = "";
        gotListenAddress = false;
        totalSites = 0;
        priority = 100;
        currOtherHost = 0;
        thisHost = new ReplicationHostAddress();
        otherHosts = new Vector<ReplicationHostAddress>();
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

    public void addOtherHost(String host, int port)
    {
        ReplicationHostAddress newInfo = new ReplicationHostAddress(host, port);
        otherHosts.add(newInfo);
    }

    public ReplicationHostAddress getFirstOtherHost()
    {
        currOtherHost = 0;
        if (otherHosts.size() == 0)
            return null;
        return (ReplicationHostAddress)otherHosts.get(currOtherHost);
    }

    public ReplicationHostAddress getNextOtherHost()
    {
        currOtherHost++;
        if (currOtherHost >= otherHosts.size())
            return null;
        return (ReplicationHostAddress)otherHosts.get(currOtherHost);
    }

    public ReplicationHostAddress getOtherHost(int i)
    {
        if (i >= otherHosts.size())
            return null;
        return (ReplicationHostAddress)otherHosts.get(i);
    }
}

