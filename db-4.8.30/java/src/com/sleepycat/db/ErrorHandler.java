/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

/**
An interface specifying a callback function to be called when an error
occurs in the Berkeley DB library.
*/
public interface ErrorHandler {
    /**
    A callback function to be called when an error occurs in the
    Berkeley DB library.
    <p>
    When an error occurs in the Berkeley DB library, an exception is
    thrown.  In some cases, however, the exception may be insufficient
    to completely describe the cause of the error, especially during
    initial application debugging.
    <p>
    The {@link com.sleepycat.db.EnvironmentConfig#setErrorHandler EnvironmentConfig.setErrorHandler} and
    {@link com.sleepycat.db.DatabaseConfig#setErrorHandler DatabaseConfig.setErrorHandler} methods are used to enhance
    the mechanism for reporting error messages to the application.  In
    some cases, when an error occurs, Berkeley DB will invoke the
    ErrorHandler's object error method.  It is up to this method to
    display the error message in an appropriate manner.    <p>
    @param environment
    The enclosing database environment handle.
    <p>
    @param errpfx
    The prefix string, as previously configured by
    {@link com.sleepycat.db.EnvironmentConfig#setErrorPrefix EnvironmentConfig.setErrorPrefix} or
    {@link com.sleepycat.db.DatabaseConfig#setErrorPrefix DatabaseConfig.setErrorPrefix}.
    <p>
    @param msg
    An error message string.
    */
    void error(Environment environment, String errpfx, String msg);
}
