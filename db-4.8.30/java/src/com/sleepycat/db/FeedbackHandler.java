/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

/**
An interface specifying a function to be called to provide feedback.
*/
public interface FeedbackHandler {
    /**
    A function called with progress information when the database environment is being recovered.
    <p>
    It is up to this function to display this information in an appropriate manner.
    <p> 
    @param environment
    A reference to the enclosing database environment.
    <p>
    @param percent
    The percent of the operation completed, specified as an integer value between 0 and 100.
    */
    void recoveryFeedback(Environment environment, int percent);

    /**
    A function called with progress information when the database is being upgraded.
    <p>
    It is up to this function to display this information in an appropriate manner.
    <p>
    @param database
    A reference to the enclosing database.
    <p>
    @param percent
    The percent of the operation completed, specified as an integer value between 0 and 100.
    */
    void upgradeFeedback(Database database, int percent);

    /**
    A function called with progress information when the database is being verified.
    <p>
    It is up to this function to display this information in an appropriate manner.
    <p>
    @param database
    A reference to the enclosing database.
    <p>
    @param percent
    The percent of the operation completed, specified as an integer value between 0 and 100.
    */
    void verifyFeedback(Database database, int percent);
}
