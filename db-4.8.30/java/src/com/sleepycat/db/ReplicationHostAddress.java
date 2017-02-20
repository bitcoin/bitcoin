/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
A simple wrapper class to hold information needed to define
a host in a replication group.
<p>
The ReplicationHostAddress can be used to refer to the current
site, or to a remote site in the replication group.
<p>
Used in the {@link com.sleepycat.db.EnvironmentConfig#replicationManagerAddRemoteSite EnvironmentConfig.replicationManagerAddRemoteSite}
and the {@link com.sleepycat.db.EnvironmentConfig#setReplicationManagerLocalSite EnvironmentConfig.setReplicationManagerLocalSite}
methods.
**/
public class ReplicationHostAddress {
    /**
    The network port component of the site address.
    **/
    public int port;
    /**
    The name component of the site address. Can be a fully qualified
    name, or a dotted format address in String format.
    **/
    public String host;

    /**
    Create a ReplicationHostAddress with default settings.
    <p>
    This is likely not what you want. The current default is
    host: localhost and port 0.
    <p>
    Only use this constructor if you plan to configure the object
    fields manually after construction.
    **/
    public ReplicationHostAddress()
    {
        this("localhost", 0);
    }

    /** 
    Create a ReplicationHostAddress with user defined host and port information.
    **/
    public ReplicationHostAddress(String host, int port)
    {
	this.port = port;
	this.host = host;
    }

    /** {@inheritDoc} */
    public String toString() {
	return host + ":" + port;
    }

    /** {@inheritDoc} */
    public boolean equals(Object o){
	try{
	    return this.toString().equals(((ReplicationHostAddress)o).toString());
	}catch (Exception e){
	    return false;
	}
    }
}
