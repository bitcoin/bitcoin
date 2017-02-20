/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

/**
An interface specifying a callback function that modifies stored data
based on a generated key.
*/
public interface RecordNumberAppender {
    /**
    A callback function to modify the stored database based on the
    generated key.
    <p>
    When storing records using {@link com.sleepycat.db.Database#append Database.append} it may be
    useful to modify the stored data based on the generated key.    This function will be called after the record number has been
    selected, but before the data has been stored.
    <p>    The callback function may modify the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}.    <p>    The callback function must throw a {@link com.sleepycat.db.DatabaseException DatabaseException} object
    to encapsulate the error on failure.  That object will be thrown to
    caller of {@link com.sleepycat.db.Database#append Database.append}.
    <p>
    @param db
    The enclosing database handle.
    <p>    @param data
    The data to be stored.
    <p>
    @param recno
    The generated record number.
    */
    void appendRecordNumber(Database db, DatabaseEntry data, int recno)
        throws DatabaseException;
}
