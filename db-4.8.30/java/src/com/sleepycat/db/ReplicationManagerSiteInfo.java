/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/** 
A simple wrapper class to hold information needed to define a replication site.  
<p>
ReplicationManagerSiteInfo objects are returned by
{@link com.sleepycat.db.Environment#getReplicationManagerSiteList
Environment.getReplicationManagerSiteList}
*/
public class ReplicationManagerSiteInfo
{
    /** The replication site's address */
    public ReplicationHostAddress addr;
    /** The replication site's identifier */
    public int eid;
    private int status;
    
    /** 
    Create a ReplicationManagerSiteInfo with the given information, isConnected defaults to false.
    */
    public ReplicationManagerSiteInfo(ReplicationHostAddress hostAddr, int eid)
    {
	this(hostAddr, eid, false);
    }
	
    /** 
    Create a ReplicationManagerSiteInfo with the given information.
    */
    public ReplicationManagerSiteInfo(ReplicationHostAddress hostAddr, int eid, boolean isConnected)
    {
        this.addr = hostAddr;
	this.eid = eid;
	this.status = isConnected ? DbConstants.DB_REPMGR_CONNECTED : 0;
    }

    /**
    The replication site is connected.
    */
    public boolean isConnected() {
        return ((this.status & DbConstants.DB_REPMGR_CONNECTED) != 0);
    }
}
