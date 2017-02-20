/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.Dbc;

/**
A specialized join cursor for use in performing equality or natural joins on
secondary indices.
<p>
A join cursor is returned when calling {@link Database#join Database.join}.
<p>
To open a join cursor using two secondary cursors:
<pre>
    Transaction txn = ...
    Database primaryDb = ...
    SecondaryDatabase secondaryDb1 = ...
    SecondaryDatabase secondaryDb2 = ...
    <p>
    SecondaryCursor cursor1 = null;
    SecondaryCursor cursor2 = null;
    JoinCursor joinCursor = null;
    try {
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry data = new DatabaseEntry();
        <p>
        cursor1 = secondaryDb1.openSecondaryCursor(txn, null);
        cursor2 = secondaryDb2.openSecondaryCursor(txn, null);
        <p>
        key.setData(...); // initialize key for secondary index 1
        OperationStatus status1 =
        cursor1.getSearchKey(key, data, LockMode.DEFAULT);
        key.setData(...); // initialize key for secondary index 2
        OperationStatus status2 =
        cursor2.getSearchKey(key, data, LockMode.DEFAULT);
        <p>
        if (status1 == OperationStatus.SUCCESS &amp;&amp;
                status2 == OperationStatus.SUCCESS) {
            <p>
            SecondaryCursor[] cursors = {cursor1, cursor2};
            joinCursor = primaryDb.join(cursors, null);
            <p>
            while (true) {
                OperationStatus joinStatus = joinCursor.getNext(key, data,
                    LockMode.DEFAULT);
                if (joinStatus == OperationStatus.SUCCESS) {
                     // Do something with the key and data.
                } else {
                    break;
                }
            }
        }
    } finally {
        if (cursor1 != null) {
            cursor1.close();
        }
        if (cursor2 != null) {
            cursor2.close();
        }
        if (joinCursor != null) {
            joinCursor.close();
        }
    }
</pre>
*/
public class JoinCursor {
    private Database database;
    private Dbc dbc;
    private JoinConfig config;

    JoinCursor(final Database database,
               final Dbc dbc,
               final JoinConfig config) {
        this.database = database;
        this.dbc = dbc;
        this.config = config;
    }

    /**
    Closes the cursors that have been opened by this join cursor.
    <p>
    The cursors passed to {@link Database#join Database.join} are not closed
    by this method, and should be closed by the caller.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void close()
        throws DatabaseException {

        dbc.close();
    }

    /**
    Returns the primary database handle associated with this cursor.
    <p>
    @return the primary database handle associated with this cursor.
    */
    public Database getDatabase() {
        return database;
    }

    /**
    Returns this object's configuration.
    <p>
    @return this object's configuration.
    */
    public JoinConfig getConfig() {
        return config;
    }

    /**
    Returns the next primary key resulting from the join operation.
<p>
An entry is returned by the join cursor for each primary key/data pair having
all secondary key values that were specified using the array of secondary
cursors passed to {@link Database#join Database.join}.
<p>
@param key the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
<p>
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
<p>
@param lockMode the locking attributes; if null, default attributes are used.
<p>
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getNext(final DatabaseEntry key, LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, DatabaseEntry.IGNORE,
                DbConstants.DB_JOIN_ITEM |
                LockMode.getFlag(lockMode)));
    }

    /**
    Returns the next primary key and data resulting from the join operation.
<p>
An entry is returned by the join cursor for each primary key/data pair having
all secondary key values that were specified using the array of secondary
cursors passed to {@link Database#join Database.join}.
<p>
@param key the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
<p>
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
<p>
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
<p>
@param lockMode the locking attributes; if null, default attributes are used.
<p>
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getNext(final DatabaseEntry key,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }
}
