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
A database cursor for a secondary database. Cursors are not thread safe
and the application is responsible for coordinating any multithreaded
access to a single cursor object.
<p>
Secondary cursors are returned by
{@link SecondaryDatabase#openCursor SecondaryDatabase.openCursor} and
{@link SecondaryDatabase#openSecondaryCursor
SecondaryDatabase.openSecondaryCursor}.  The distinguishing characteristics
of a secondary cursor are:
<ul>
<li>Direct calls to <code>put()</code> methods on a secondary cursor are
prohibited.
<li>The {@link #delete} method of a secondary cursor will delete the primary
record and as well as all its associated secondary records.
<li>Calls to all get methods will return the data from the associated
primary database.
<li>Additional get method signatures are provided to return the primary key
in an additional pKey parameter.
<li>Calls to {@link #dup} will return a {@link SecondaryCursor}.
<li>The {@link #dupSecondary} method is provided to return a {@link
SecondaryCursor} that doesn't require casting.
</ul>
<p>
To obtain a secondary cursor with default attributes:
<blockquote><pre>
    SecondaryCursor cursor = myDb.openSecondaryCursor(txn, null);
</pre></blockquote>
To customize the attributes of a cursor, use a CursorConfig object.
<blockquote><pre>
    CursorConfig config = new CursorConfig();
    config.setDirtyRead(true);
    SecondaryCursor cursor = myDb.openSecondaryCursor(txn, config);
</pre></blockquote>
*/
public class SecondaryCursor extends Cursor {
    /* package */
    SecondaryCursor(final SecondaryDatabase database,
                    final Dbc dbc,
                    final CursorConfig config)
        throws DatabaseException {

        super(database, dbc, config);
    }

    /**
    Return the SecondaryDatabase handle associated with this Cursor.
    <p>
    @return
    The SecondaryDatabase handle associated with this Cursor.
    <p>
    */
    public SecondaryDatabase getSecondaryDatabase() {
        return (SecondaryDatabase)super.getDatabase();
    }

    /**
    Returns a new <code>SecondaryCursor</code> for the same transaction as
    the original cursor.
    */
    public Cursor dup(final boolean samePosition)
        throws DatabaseException {

        return dupSecondary(samePosition);
    }

    /**
    Returns a new copy of the cursor as a <code>SecondaryCursor</code>.
    <p>
    Calling this method is the equivalent of calling {@link #dup} and
    casting the result to {@link SecondaryCursor}.
    <p>
    @see #dup
    */
    public SecondaryCursor dupSecondary(final boolean samePosition)
        throws DatabaseException {

        return new SecondaryCursor(getSecondaryDatabase(),
            dbc.dup(samePosition ? DbConstants.DB_POSITION : 0), config);
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the key/pair at the cursor
position has been deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getCurrent(final DatabaseEntry key,
                                      final DatabaseEntry pKey,
                                      final DatabaseEntry data,
                                      LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_CURRENT | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getFirst(final DatabaseEntry key,
                                    final DatabaseEntry pKey,
                                    final DatabaseEntry data,
                                    LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_FIRST | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getLast(final DatabaseEntry key,
                                   final DatabaseEntry pKey,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_LAST | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getNext(final DatabaseEntry key,
                                   final DatabaseEntry pKey,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_NEXT | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getNextDup(final DatabaseEntry key,
                                      final DatabaseEntry pKey,
                                      final DatabaseEntry data,
                                      LockMode lockMode)
        throws DatabaseException {

       return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_NEXT_DUP | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getNextNoDup(final DatabaseEntry key,
                                        final DatabaseEntry pKey,
                                        final DatabaseEntry data,
                                        LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_NEXT_NODUP | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getPrev(final DatabaseEntry key,
                                   final DatabaseEntry pKey,
                                   final DatabaseEntry data,
                                   LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_PREV | LockMode.getFlag(lockMode) |
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getPrevDup(final DatabaseEntry key,
                                      final DatabaseEntry pKey,
                                      final DatabaseEntry data,
                                      LockMode lockMode)
        throws DatabaseException {

        /*
         * "Get the previous duplicate" isn't directly supported by the C API,
         * so here's how to get it: dup the cursor and call getPrev, then dup
         * the result and call getNextDup.  If both succeed then there was a
         * previous duplicate and the first dup is sitting on it.  Keep that,
         * and call getCurrent to fill in the user's buffers.
         */
        Dbc dup1 = dbc.dup(DbConstants.DB_POSITION);
        try {
            int errCode = dup1.get(DatabaseEntry.IGNORE, DatabaseEntry.IGNORE,
                DbConstants.DB_PREV | LockMode.getFlag(lockMode));
            if (errCode == 0) {
                Dbc dup2 = dup1.dup(DbConstants.DB_POSITION);
                try {
                    errCode = dup2.get(DatabaseEntry.IGNORE,
                        DatabaseEntry.IGNORE,
                        DbConstants.DB_NEXT_DUP | LockMode.getFlag(lockMode));
                } finally {
                    dup2.close();
                }
            }
            if (errCode == 0)
                errCode = dup1.pget(key, pKey, data,
                    DbConstants.DB_CURRENT | LockMode.getFlag(lockMode) |
                    ((data == null) ? 0 : data.getMultiFlag()));
            if (errCode == 0) {
                Dbc tdbc = dbc;
                dbc = dup1;
                dup1 = tdbc;
            }
            return OperationStatus.fromInt(errCode);
        } finally {
            dup1.close();
        }
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
@param key the secondary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getPrevNoDup(final DatabaseEntry key,
                                        final DatabaseEntry pKey,
                                        final DatabaseEntry data,
                                        LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_PREV_NODUP | LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Return the record number associated with the cursor.  The record number
will be returned in the data parameter.
<p>
For this method to be called, the underlying database must be of type
Btree, and it must have been configured to support record numbers.
<p>
When called on a cursor opened on a database that has been made into a
secondary index, the method returns the record numbers of both the
secondary and primary databases.  If either underlying database is not of
type Btree or is not configured with record numbers, the out-of-band
record number of 0 is returned.
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
@param secondaryRecno the secondary record number
returned as output.  Its byte array does not need to be initialized by the
caller.
@param primaryRecno the primary record number
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getRecordNumber(final DatabaseEntry secondaryRecno,
                                           final DatabaseEntry primaryRecno,
                                           LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(DatabaseEntry.IGNORE, secondaryRecno, primaryRecno,
                DbConstants.DB_GET_RECNO | LockMode.getFlag(lockMode)));
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
@param key the secondary key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchKey(final DatabaseEntry key,
                                        final DatabaseEntry pKey,
                                        final DatabaseEntry data,
                                        LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_SET | LockMode.getFlag(lockMode) |
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
@param key the secondary key
used as input and returned as output.  It must be initialized with a non-null
byte array by the caller.
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchKeyRange(final DatabaseEntry key,
                                             final DatabaseEntry pKey,
                                             final DatabaseEntry data,
                                             LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_SET_RANGE | LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the specified secondary and primary key, where both
the primary and secondary key items must match.
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
@param key the secondary key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param pKey the primary key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchBoth(final DatabaseEntry key,
                                         final DatabaseEntry pKey,
                                         final DatabaseEntry data,
                                         LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_GET_BOTH | LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Move the cursor to the specified secondary key and closest matching primary
key of the database.
<p>
In the case of any database supporting sorted duplicate sets, the returned
key/data pair is for the smallest primary key greater than or equal to the
specified primary key (as determined by the key comparison function),
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
@param key the secondary key
used as input and returned as output.  It must be initialized with a non-null
byte array by the caller.
@param pKey the primary key
used as input and returned as output.  It must be initialized with a non-null
byte array by the caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchBothRange(final DatabaseEntry key,
                                              final DatabaseEntry pKey,
                                              final DatabaseEntry data,
                                              LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(key, pKey, data,
                DbConstants.DB_GET_BOTH_RANGE | LockMode.getFlag(lockMode) |
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
@param secondaryRecno the secondary record number
used as input.  It must be initialized with a non-null byte array by the
caller.
@param data the primary data
returned as output.  Multiple results can be retrieved by passing an object
that is a subclass of {@link com.sleepycat.db.MultipleEntry MultipleEntry}, otherwise its byte array does not
need to be initialized by the caller.
@param lockMode the locking attributes; if null, default attributes are used.
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    */
    public OperationStatus getSearchRecordNumber(
            final DatabaseEntry secondaryRecno,
            final DatabaseEntry pKey,
            final DatabaseEntry data,
            LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            dbc.pget(secondaryRecno, pKey, data,
                DbConstants.DB_SET_RECNO | LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }
}
