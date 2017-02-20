/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

/*
 * An interface class with prototype definitions of all event functions that
 * can be called via the Berkeley DB event callback mechanism.
 * A user can choose to implement the EventHandler class, and implement
 * handlers for all of the event types. Or they could extend the
 * EventHandlerAdapter class, and implement only those events relevant to
 * their application.
 */
/**
An interface classs with prototype definitions of all event functions that
can be called via the Berkeley DB event callback mechanism.
<p>
A user can choose to implement the EventHandler class, and implement handlers
for all of the event types. Alternatively it is possible to extend the
EventHandlerAdapter class, and implement only those events relevant to the
specific application.
<p>
The {@link com.sleepycat.db.EnvironmentConfig#setEventHandler EnvironmentConfig.setEventHandler} is used to provide
a mechanism for reporting event messages from the Berkeley DB library
to the application.
<p>
Berkeley DB is not re-entrant. Callback functions should not attempt
to make library calls (for example, to release locks or close open
handles). Re-entering Berkeley DB is not guaranteed to work correctly,
and the results are undefined.
*/
public interface EventHandler {
    /**
    A callback function to be called when a panic event is sent from the    Berkeley DB library.
    <p>    This event callback is received when an error occurs in the Berkeley DB
    library where the only solution is to shut down the application and run
    recovery. In such cases, the Berkeley DB methods will throw
    {@link com.sleepycat.db.RunRecoveryException RunRecoveryException} exceptions. It is often easier to simply exit
    the application when such errors occur, rather than gracefully return up
    the stack.
    <p>
    When this callback is received the database environment has failed. All
    threads of control in the database environment should exit the environment
    and recovery should be run.
    */
    public void handlePanicEvent();

    /**
    A callback function to be called when a Replication Client event is sent
    from the Berkeley DB library.
    <p>
    This event callback is received when this member of a replication group is
    now a client site.
    */
    public void handleRepClientEvent();

    /**
    A callback function to be called when an event is sent from the
    Berkeley DB library.
    <p>
    This event callback is received when this site has just won an election. An
    Application using the Base replication API should arrange for a call to
    the {@link com.sleepycat.db.Environment#startReplication Environment.startReplication} method after receiving this
    event to, reconfigure the local environment as a replication master.
    <p>
    Replication Manager applications may safely igore this event. The
    Replication Manager calls {@link com.sleepycat.db.Environment#startReplication Environment.startReplication}
    automatically on behalf of the application when appropriate (resulting in
    firing of the {@link com.sleepycat.db.EventHandler#handleRepMasterEvent EventHandler.handleRepMasterEvent} event).
    */
    public void handleRepElectedEvent();

    /**
    A callback function to be called when an event is sent from the
    Berkeley DB library.
    <p>
    This event callback is received when this site is now the master site of
    its replication group. It is the application's responsibility to begin
    acting as the master environment.
    */
    public void handleRepMasterEvent();

    /**
    A callback function to be called when an event is sent from the
    Berkeley DB library.
    <p>
    This event callback is received when the replication group of which this
    site is a member has just established a new master; the local site is not
    the new master.
    @param envId
    The environment ID of the new master site.
    */
    public void handleRepNewMasterEvent(int envId);

    /**
    A callback function to be called when an event is sent from the
    Berkeley DB library.
    <p>
    This event callback is received when the replication manager did not
    receive enough acknowledgements (based on the acknowledgement policy
    configured with {@link com.sleepycat.db.EnvironmentConfig#setReplicationManagerAckPolicy EnvironmentConfig.setReplicationManagerAckPolicy})
    to ensure a transaction's durability within the replication group. The
    transaction will be flushed to the master's local disk storage for
    durability.
    */
    public void handleRepPermFailedEvent();

    /**
    A callback function to be called when an event is sent from the
    Berkeley DB library.
    <p>
    This event callback is received when the client has completed startup
    synchronization and is now processing live log records received from the
    master.
    */
    public void handleRepStartupDoneEvent();

    /**
    A callback function to be called when an event is sent from the
    Berkeley DB library.
    <p>
    This event callback is received when a Berkeley DB write to stable storage
    failed.
    @param  errorCode
    If an operating system specific error code is available for the failure it
    will be passed in the errorCode parameter.
    */
    public void handleWriteFailedEvent(int errorCode);
}
