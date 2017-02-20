/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
An interface specifying a replication transmit function, which sends
information to other members of the replication group.
*/
public interface ReplicationTransport {
    /**
    The callback used when Berkeley DB needs to transmit a replication message.
    This method must not call back down into Berkeley DB.  It must return 0 on
    success and non-zero on failure.  If the transmission fails, the message
    being sent is necessary to maintain database integrity, and the local log
    is not configured for synchronous flushing, the local log will be flushed;
    otherwise, any error from the function will be ignored.
    <p>
    @param environment
    The enclosing database environment handle.
    <p>
    @param control
    The first of the two data elements to be transmitted.
    <p>
    @param rec
    The second of the two data elements to be transmitted.
    <p>
    @param lsn
    If the type of message to be sent has an LSN associated with it,
    then the lsn contains the LSN of the record being sent.  This LSN
    can be used to determine that certain records have been processed
    successfully by clients.
    <p>
    @param envid
    A positive integer identifier that specifies the replication
    environment to which the message should be sent.
    <p>
    The value DB_EID_BROADCAST indicates that a message should be
    broadcast to every environment in the replication group.  The
    application may use a true broadcast protocol or may send the
    message in sequence to each machine with which it is in
    communication.  In both cases, the sending site should not be asked
    to process the message.
    <p>
    @param noBuffer
    The record being sent should be transmitted immediately and not buffered
    or delayed.
    <p>
    @param permanent
    The record being sent is critical for maintaining database integrity
    (for example, the message includes a transaction commit).  The
    application should take appropriate action to enforce the reliability
    guarantees it has chosen, such as waiting for acknowledgement from one
    or more clients.
    <p>
    @param anywhere
    The message is a client request that can be satisfied by another client as 
    well as by the master.
    <p>
    @param isRetry
    The message is a client request that has already been made and to which no 
    response was received.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    int send(Environment environment, DatabaseEntry control, DatabaseEntry rec,
             LogSequenceNumber lsn, int envid, boolean noBuffer,
             boolean permanent, boolean anywhere, boolean isRetry)
        throws DatabaseException;

    /**
    A message that should be broadcast to every environment in the
    replication group.  The application may use a true broadcast protocol or
    may send the message in sequence to each machine with which it is in
    communication.  In both cases, the sending site should not be asked to
    process the message.
    */
    int EID_BROADCAST = DbConstants.DB_EID_BROADCAST;

    /**
    An invalid environment ID, and may be used to initialize environment ID
    variables that are subsequently checked for validity.
    */
    int EID_INVALID = DbConstants.DB_EID_INVALID;
}
