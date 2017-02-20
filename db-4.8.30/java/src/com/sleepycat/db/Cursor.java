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
A database cursor. Cursors are used for operating on collections of
records, for iterating over a database, and for saving handles to
individual records, so that they can be modified after they have been
read.
<p>
Cursors may be used by multiple threads, but only serially.  That is,
the application must serialize access to the handle.
<p>
If the cursor is to be used to perform operations on behalf of a
transaction, the cursor must be opened and closed within the context of
that single transaction.
<p>
Once the cursor close method has been called, the handle may not be
accessed again, regardless of the close method's success or failure.
<p>
To obtain a cursor with default attributes:
<blockquote><pre>
    Cursor cursor = myDatabase.openCursor(txn, null);
</pre></blockquote>
To customize the attributes of a cursor, use a CursorConfig object.
<blockquote><pre>
    CursorConfig config = new CursorConfig();
    config.setDirtyRead(true);
    Cursor cursor = myDatabase.openCursor(txn, config);
</pre></blockquote>
<p>
Modifications to the database during a sequential scan will be reflected
in the scan; that is, records inserted behind a cursor will not be
returned while records inserted in front of a cursor will be returned.
In Queue and Recno databases, missing entries (that is, entries that
were never explicitly created or that were created and then deleted)
will be ignored during a sequential scan.
*/
public class Cursor {
    /* package */ Dbc dbc;
    /* package */ Database database;
    /* package */ CursorConfig config;

    // Constructor needed by Java RPC server
    protected Cursor(final Database database, final CursorConfig config) {
        this.database = database;
        this.config = config;
    }

    Cursor(final Database database, final Dbc dbc, final CursorConfig config)
        throws DatabaseException {

        this.database = database;
        this.dbc = dbc;
        this.config = config;
    }

    public synchronized void close()
        throws DatabaseException {

        if (dbc != null) {
            try {
                dbc.close();
            } finally {
                dbc = null;
            }
        }
    }

    /**
    Return a new cursor with the same transaction and locker ID as the
    original cursor.
    <p>
    This is useful when an application is using locking and requires two
    or more cursors in the same thread of control.
    <p>
    @param samePosition
    If true, the newly created cursor is initialized to refer to the
    same position in the database as the original cursor (if any) and
    hold the same locks (if any). If false, or the original cursor does
    not hold a database position and locks, the returned cursor is
    uninitialized and will behave like a newly created cursor.
    <p>
    @return
    A new cursor with the same transaction and locker ID as the original
    cursor.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public Cursor dup(final boolean samePosition)
        throws DatabaseException {

        return new Cursor(database,
            dbc.dup(samePosition ? DbConstants.DB_POSITION : 0), config);
    }

    /**
    Return this cursor's configuration.
    <p>
    This may differ from the configuration used to open this object if
    the cursor existed previously.
    <p>
    @return
    This cursor's configuration.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public CursorConfig getConfig() {
        return config;
    }

    /**
    Return the Database handle associated with this Cursor.
    <p>
    @return
    The Database handle associated with this Cursor.
    <p>
    */
    public Database getDatabase() {
        return database;
    }

    /**
    Return a comparison of the two cursors.
    <p>
    @return
    An integer representing the result of the comparison. 0 is equal, 1 
    indicates this cursor is greater than OtherCursor, -1 indicates that 
    OtherCursor is greater than this cursor.
    <p>
    <p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public int compare(Cursor OtherCursor)
        throws DatabaseException {

        return dbc.cmp(OtherCursor.dbc, 0);
    }

    /**
    Return a count of the number of data items for the key to which the
    cursor refers.
    <p>
    @return
    A count of the number of data items for the key to which the cursor
    refers.
    <p>
    <p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public int count()
        throws DatabaseException {

        return dbc.count(0);
    }

    /**
    Delete the key/data pair to which the cursor refers.
    <p>
    When called on a cursor opened on a database that has been made into a
    secondary index, this method the key/data pair from the primary database
    and all secondary indices.
    <p>
    The cursor position is unchanged after a delete, and subsequent calls
to cursor functions expecting the cursor to refer to an existing key
will fail.
    <p>
    <p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus delete()
        throws DatabaseException {

        return OperationStatus.fromInt(dbc.del(0));
    }

    /**
    Returns the key/data pair to which the cursor refers.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the key/pair at the cursor
position has been deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getCurrent(final DatabaseEntry key,
                                      final DatabaseEntry data,
                                      LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_CURRENT |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the first key/data pair of the database, and return
that pair.  If the first key has duplicate values, the first data item
in the set of duplicates is returned.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getFirst(final DatabaseEntry key,
                                    final DatabaseEntry data,
                                    LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_FIRST |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the last key/data pair of the database, and return
that pair.  If the last key has duplicate values, the last data item in
the set of duplicates is returned.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getLast(final DatabaseEntry key,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_LAST |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the next key/data pair and return that pair.  If
the matching key has duplicate values, the first data item in the set
of duplicates is returned.
<p>
If the cursor is not yet initialized, move the cursor to the first
key/data pair of the database, and return that pair.  Otherwise, the
cursor is moved to the next key/data pair of the database, and that pair
is returned.  In the presence of duplicate key values, the value of the
key may not change.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getNext(final DatabaseEntry key,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_NEXT |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    If the next key/data pair of the database is a duplicate data record for
the current key/data pair, move the cursor to the next key/data pair
of the database and return that pair.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getNextDup(final DatabaseEntry key,
                                      final DatabaseEntry data,
                                      LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_NEXT_DUP |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the next non-duplicate key/data pair and return
that pair.  If the matching key has duplicate values, the first data
item in the set of duplicates is returned.
<p>
If the cursor is not yet initialized, move the cursor to the first
key/data pair of the database, and return that pair.  Otherwise, the
cursor is moved to the next non-duplicate key of the database, and that
key/data pair is returned.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getNextNoDup(final DatabaseEntry key,
                                        final DatabaseEntry data,
                                        LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_NEXT_NODUP |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the previous key/data pair and return that pair.
If the matching key has duplicate values, the last data item in the set
of duplicates is returned.
<p>
If the cursor is not yet initialized, move the cursor to the last
key/data pair of the database, and return that pair.  Otherwise, the
cursor is moved to the previous key/data pair of the database, and that
pair is returned. In the presence of duplicate key values, the value of
the key may not change.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getPrev(final DatabaseEntry key,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_PREV |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    If the previous key/data pair of the database is a duplicate data record
for the current key/data pair, move the cursor to the previous key/data
pair of the database and return that pair.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getPrevDup(final DatabaseEntry key,
                                      final DatabaseEntry data,
                                      LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_PREV_DUP |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the previous non-duplicate key/data pair and return
that pair.  If the matching key has duplicate values, the last data item
in the set of duplicates is returned.
<p>
If the cursor is not yet initialized, move the cursor to the last
key/data pair of the database, and return that pair.  Otherwise, the
cursor is moved to the previous non-duplicate key of the database, and
that key/data pair is returned.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getPrevNoDup(final DatabaseEntry key,
                                        final DatabaseEntry data,
                                        LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_PREV_NODUP |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Return the record number associated with the cursor.  The record number
will be returned in the data parameter.
<p>
For this method to be called, the underlying database must be of type
Btree, and it must have been configured to support record numbers.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param data the  data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getRecordNumber(final DatabaseEntry data,
                                           LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(DatabaseEntry.IGNORE, data,
                DbConstants.DB_GET_RECNO |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the given key of the database, and return the datum
associated with the given key.  If the matching key has duplicate
values, the first data item in the set of duplicates is returned.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchKey(final DatabaseEntry key,
                                        final DatabaseEntry data,
                                        LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_SET |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the closest matching key of the database, and return
the data item associated with the matching key.  If the matching key has
duplicate values, the first data item in the set of duplicates is returned.
<p>
The returned key/data pair is for the smallest key greater than or equal
to the specified key (as determined by the key comparison function),
permitting partial key matches and range searches.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
used as input and returned as output.  It must be initialized with a non-null
byte array by the caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchKeyRange(final DatabaseEntry key,
                                             final DatabaseEntry data,
                                             LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_SET_RANGE |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the specified key/data pair, where both the key and
data items must match.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param data the  data
used as input.  It must be initialized with a non-null byte array by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchBoth(final DatabaseEntry key,
                                         final DatabaseEntry data,
                                         LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_GET_BOTH |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the specified key and closest matching data item of the
database.
<p>
In the case of any database supporting sorted duplicate sets, the returned
key/data pair is for the smallest data item greater than or equal to the
specified data item (as determined by the duplicate comparison function),
permitting partial matches and range searches in duplicate data sets.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
used as input and returned as output.  It must be initialized with a non-null
byte array by the caller.
@param data the  data
used as input and returned as output.  It must be initialized with a non-null
byte array by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchBothRange(final DatabaseEntry key,
                                              final DatabaseEntry data,
                                              LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_GET_BOTH_RANGE |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the specific numbered record of the database, and
return the associated key/data pair.
<p>
The data field of the specified key must be a byte array containing a
record number, as described in {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}.  This determines
the record to be retrieved.
<p>
For this method to be called, the underlying database must be of type
Btree, and it must have been configured to support record numbers.
<p>
If this method fails for any reason, the position of the cursor will be
unchanged.
@throws NullPointerException if a DatabaseEntry parameter is null or
does not contain a required non-null byte array.
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
<p>
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchRecordNumber(final DatabaseEntry key,
                                                 final DatabaseEntry data,
                                                 LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.get(key, data, DbConstants.DB_SET_RECNO |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Store a key/data pair into the database.
<p>
If the put method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the put method fails for any reason, the
state of the cursor will be unchanged.
<p>
If the key already appears in the database and duplicates are supported,
the new data value is inserted at the correct sorted location.  If the
key already appears in the database and duplicates are not supported,
the existing key/data pair will be replaced.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus put(final DatabaseEntry key,
                               final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(key, data, DbConstants.DB_KEYLAST));
    }

    /**
    Store a key/data pair into the database.
<p>
If the putAfter method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the putAfter method fails for any reason, the
state of the cursor will be unchanged.
<p>
In the case of the Btree and Hash access methods, insert the data
element as a duplicate element of the key to which the cursor refers.
The new element appears immediately
after
the current cursor position.  It is an error to call this method if the
underlying Btree or Hash database does not support duplicate data items.
The key parameter is ignored.
<p>
In the case of the Hash access method, the putAfter method will fail and
throw an exception if the current cursor record has already been deleted.
<p>
In the case of the Recno access method, it is an error to call this
method if the underlying Recno database was not configured to have
mutable record numbers.  A new key is created, all records after the
inserted item are automatically renumbered, and the key of the new
record is returned in the key parameter.  The initial value of the key
parameter is ignored.
<p>
The putAfter method may not be called for the Queue access method.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putAfter(final DatabaseEntry key,
                                    final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(key, data, DbConstants.DB_AFTER));
    }

    /**
    Store a key/data pair into the database.
<p>
If the putBefore method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the putBefore method fails for any reason, the
state of the cursor will be unchanged.
<p>
In the case of the Btree and Hash access methods, insert the data
element as a duplicate element of the key to which the cursor refers.
The new element appears immediately
before
the current cursor position.  It is an error to call this method if the
underlying Btree or Hash database does not support duplicate data items.
The key parameter is ignored.
<p>
In the case of the Hash access method, the putBefore method will fail and
throw an exception if the current cursor record has already been deleted.
<p>
In the case of the Recno access method, it is an error to call this
method if the underlying Recno database was not configured to have
mutable record numbers.  A new key is created, all records after the
inserted item are automatically renumbered, and the key of the new
record is returned in the key parameter.  The initial value of the key
parameter is ignored.
<p>
The putBefore method may not be called for the Queue access method.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putBefore(final DatabaseEntry key,
                                     final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(key, data, DbConstants.DB_BEFORE));
    }

    /**
    Store a key/data pair into the database.
<p>
If the putNoOverwrite method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the putNoOverwrite method fails for any reason, the
state of the cursor will be unchanged.
<p>
If the key already appears in the database, putNoOverwrite will return
{@link com.sleepycat.db.OperationStatus#KEYEXIST OperationStatus.KEYEXIST}.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putNoOverwrite(final DatabaseEntry key,
                                          final DatabaseEntry data)
        throws DatabaseException {

        /*
         * The tricks here are making sure the cursor doesn't move on error and
         * noticing that if the key exists, that's an error and we don't want
         * to return the data.
         */
        Dbc tempDbc = dbc.dup(0);
        try {
            int errCode = tempDbc.get(key, DatabaseEntry.IGNORE,
                DbConstants.DB_SET | database.rmwFlag);
            if (errCode == 0)
                return OperationStatus.KEYEXIST;
            else if (errCode != DbConstants.DB_NOTFOUND &&
                errCode != DbConstants.DB_KEYEMPTY)
                return OperationStatus.fromInt(errCode);
            else {
                Dbc tdbc = dbc;
                dbc = tempDbc;
                tempDbc = tdbc;

                return OperationStatus.fromInt(
                    dbc.put(key, data, DbConstants.DB_KEYLAST));
            }
        } finally {
            tempDbc.close();
        }
    }

    /**
    Store a key/data pair into the database.
<p>
If the putKeyFirst method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the putKeyFirst method fails for any reason, the
state of the cursor will be unchanged.
<p>
In the case of the Btree and Hash access methods, insert the specified
key/data pair into the database.
<p>
If the underlying database supports duplicate data items, and if the
key already exists in the database and a duplicate sort function has
been specified, the inserted data item is added in its sorted location.
If the key already exists in the database and no duplicate sort function
has been specified, the inserted data item is added as the
first
of the data items for that key.
<p>
The putKeyFirst method may not be called for the Queue or Recno access methods.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putKeyFirst(final DatabaseEntry key,
                                       final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(key, data, DbConstants.DB_KEYFIRST));
    }

    /**
    Store a key/data pair into the database.
<p>
If the putKeyLast method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the putKeyLast method fails for any reason, the
state of the cursor will be unchanged.
<p>
In the case of the Btree and Hash access methods, insert the specified
key/data pair into the database.
<p>
If the underlying database supports duplicate data items, and if the
key already exists in the database and a duplicate sort function has
been specified, the inserted data item is added in its sorted location.
If the key already exists in the database and no duplicate sort function
has been specified, the inserted data item is added as the
last
of the data items for that key.
<p>
The putKeyLast method may not be called for the Queue or Recno access methods.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putKeyLast(final DatabaseEntry key,
                                      final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(key, data, DbConstants.DB_KEYLAST));
    }

    /**
    Store a key/data pair into the database.
<p>
If the putNoDupData method succeeds, the cursor is always positioned to refer to
the newly inserted item.  If the putNoDupData method fails for any reason, the
state of the cursor will be unchanged.
<p>
In the case of the Btree and Hash access methods, insert
the specified key/data pair into the database, unless a key/data pair
comparing equally to it already exists in the database.  If a matching
key/data pair already exists in the database, {@link com.sleepycat.db.OperationStatus#KEYEXIST OperationStatus.KEYEXIST} is returned.
<p>
This method may only be called if the underlying database has been
configured to support sorted duplicate data items.
<p>
This method may not be called for the Queue or Recno access methods.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putNoDupData(final DatabaseEntry key,
                                        final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(key, data, DbConstants.DB_NODUPDATA));
    }

    /**
    Replaces the data in the key/data pair at the current cursor position.
    <p>
    Whether the putCurrent method succeeds or fails for any reason, the state
    of the cursor will be unchanged.
    <p>
    Overwrite the data of the key/data pair to which the cursor refers with the
    specified data item. This method will return OperationStatus.NOTFOUND if
    the cursor currently refers to an already-deleted key/data pair.
    <p>
    For a database that does not support duplicates, the data may be changed by
    this method.  If duplicates are supported, the data may be changed only if
    a custom partial comparator is configured and the comparator considers the
    old and new data to be equal (that is, the comparator returns zero).  For
    more information on partial comparators see {@link
    DatabaseConfig#setDuplicateComparator}.
    <p>
    If the old and new data are unequal according to the comparator, a {@code
    DatabaseException} is thrown.  Changing the data in this case would change
    the sort order of the record, which would change the cursor position, and
    this is not allowed.  To change the sort order of a record, delete it and
    then re-insert it.
    <p>
    @param data - the data DatabaseEntry stored.
    <br>
    @throws DeadlockException - if the operation was selected to resolve a
    deadlock.
    <br>
    @throws IllegalArgumentException - if an invalid parameter was specified.
    <br>
    @throws DatabaseException - if the old and new data are not equal according
    to the configured duplicate comparator or default comparator, or if a
    failure occurs.
    <br>
    */
    public OperationStatus putCurrent(final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.put(DatabaseEntry.UNUSED, data, DbConstants.DB_CURRENT));
    }

    /**
    Get the cache priority for pages referenced by the cursor.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public CacheFilePriority getPriority()
        throws DatabaseException {

        return CacheFilePriority.fromFlag(dbc.get_priority());
    }

    /**
    Set the cache priority for pages referenced by the DBC handle.
    <p>
    The priority of a page biases the replacement algorithm to be more or less
    likely to discard a page when space is needed in the buffer pool. The bias
    is temporary, and pages will eventually be discarded if they are not
    referenced again. The DBcursor->set_priority method is only advisory, and
    does not guarantee pages will be treated in a specific way.
    <p>
    This method may be called at any time during the life of the application.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void setPriority(final CacheFilePriority priority)
        throws DatabaseException {

        dbc.set_priority(priority.getFlag());
    }
}
