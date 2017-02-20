/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;
import com.sleepycat.db.EventHandler;

/*
 * An abstract implementation of the EventHandler class can be extended by
 * the application to implement customized handling for any event generated
 * by Berkeley DB.
 */

/**
An abstract class that implements {@link com.sleepycat.db.EventHandler EventHandler}, used to specify a
callback function to be called when an event is sent from the Berkeley DB
library.
<p>
See the {@link com.sleepycat.db.EventHandler EventHandler} class documentation for information on event
callback handler usage.
*/
public abstract class EventHandlerAdapter implements EventHandler {
    /**
    See {@link com.sleepycat.db.EventHandler#handlePanicEvent EventHandler.handlePanicEvent} for details of this callback.
    */
    public void handlePanicEvent() {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleRepClientEvent EventHandler.handleRepClientEvent} for details of this
    callback.
    */
    public void handleRepClientEvent() {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleRepElectedEvent EventHandler.handleRepElectedEvent} for details of this
    callback.
    */
    public void handleRepElectedEvent() {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleRepMasterEvent EventHandler.handleRepMasterEvent} for details of this
    callback.
    */
    public void handleRepMasterEvent() {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleRepNewMasterEvent EventHandler.handleRepNewMasterEvent} for details of this
    callback.
    */
    public void handleRepNewMasterEvent(int envId) {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleRepPermFailedEvent EventHandler.handleRepPermFailedEvent} for details of this
    callback.
    */
    public void handleRepPermFailedEvent() {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleRepStartupDoneEvent EventHandler.handleRepStartupDoneEvent} for details of this
    callback.
    */
    public void handleRepStartupDoneEvent() {}
    /**
    See {@link com.sleepycat.db.EventHandler#handleWriteFailedEvent EventHandler.handleWriteFailedEvent} for details of this
    callback.
    */
    public void handleWriteFailedEvent(int errorCode) {}
}
