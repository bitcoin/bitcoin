/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.Db;
import com.sleepycat.db.internal.DbConstants;

/**
A secondary database handle.
<p>
Secondary databases are opened with {@link
Environment#openSecondaryDatabase Environment.openSecondaryDatabase} and are
always associated with a single primary database.  The distinguishing
characteristics of a secondary database are:
<ul>
<li>Records are automatically added to a secondary database when records are
added, modified and deleted in the primary database.  Direct calls to
<code>put()</code> methods on a secondary database are prohibited.</li>
<li>The {@link #delete delete} method of a secondary database will delete
the primary record and as well as all its associated secondary records.</li>
<li>Calls to all <code>get()</code> methods will return the data from the
associated primary database.</li>
<li>Additional <code>get()</code> method signatures are provided to return
the primary key in an additional <code>pKey</code> parameter.</li>
<li>Calls to {@link #openCursor openCursor} will return a {@link
SecondaryCursor}, which itself has <code>get()</code> methods that return
the data of the primary database and additional <code>get()</code> method
signatures for returning the primary key.</li>
<li>The {@link #openSecondaryCursor openSecondaryCursor} method is provided
to return a {@link SecondaryCursor} that doesn't require casting.</li>
</ul>
<p>
Before opening or creating a secondary database you must implement the {@link
SecondaryKeyCreator}
interface.
<p>
For example, to create a secondary database that supports duplicates:
<pre>
    Database primaryDb; // The primary database must already be open.
    SecondaryKeyCreator keyCreator; // Your key creator implementation.
    SecondaryConfig secConfig = new SecondaryConfig();
    secConfig.setAllowCreate(true);
    secConfig.setSortedDuplicates(true);
    secConfig.setKeyCreator(keyCreator);
    SecondaryDatabase newDb = env.openSecondaryDatabase(transaction,
                                                        "myDatabaseName",
                                                        primaryDb,
                                                        secConfig)
</pre>
<p>
If a primary database is to be associated with one or more secondary
databases, it may not be configured for duplicates.
<p>
Note that the associations between primary and secondary databases are not
stored persistently.  Whenever a primary database is opened for write access by
the application, the appropriate associated secondary databases should also be
opened by the application.  This is necessary to ensure data integrity when
changes are made to the primary database.
*/
public class SecondaryDatabase extends Database {
    private final Database primaryDatabase;

    /* package */
    SecondaryDatabase(final Db db, final Database primaryDatabase)
        throws DatabaseException {

        super(db);
        this.primaryDatabase = primaryDatabase;
    }

    /**
    Open a database.
<p>
The database is represented by the file and database parameters.
<p>
The currently supported database file formats (or <em>access
methods</em>) are Btree, Hash, Queue, and Recno.  The Btree format is a
representation of a sorted, balanced tree structure.  The Hash format
is an extensible, dynamic hashing scheme.  The Queue format supports
fast access to fixed-length records accessed sequentially or by logical
record number.  The Recno format supports fixed- or variable-length
records, accessed sequentially or by logical record number, and
optionally backed by a flat text file.
<p>
Storage and retrieval are based on key/data pairs; see {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}
for more information.
<p>
Opening a database is a relatively expensive operation, and maintaining
a set of open databases will normally be preferable to repeatedly
opening and closing the database for each new query.
<p>
In-memory databases never intended to be preserved on disk may be
created by setting both the fileName and databaseName parameters to
null.  Note that in-memory databases can only ever be shared by sharing
the single database handle that created them, in circumstances where
doing so is safe.  The environment variable <code>TMPDIR</code> may
be used as a directory in which to create temporary backing files.
<p>
@param fileName
The name of an underlying file that will be used to back the database.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param databaseName
An optional parameter that allows applications to have multiple
databases in a single file.  Although no databaseName parameter needs
to be specified, it is an error to attempt to open a second database in
a physical file that was not initially created using a databaseName
parameter.  Further, the databaseName parameter is not supported by the
Queue format.
<p>
@param primaryDatabase
a database handle for the primary database that is to be indexed.
<p>
@param config The secondary database open attributes.  If null, default attributes are used.
    */
    public SecondaryDatabase(final String fileName,
                             final String databaseName,
                             final Database primaryDatabase,
                             final SecondaryConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        this(SecondaryConfig.checkNull(config).openSecondaryDatabase(
                null, null, fileName, databaseName, primaryDatabase.db),
            primaryDatabase);
    }

    /** {@inheritDoc} */
    public Cursor openCursor(final Transaction txn, final CursorConfig config)
        throws DatabaseException {

        return openSecondaryCursor(txn, config);
    }

    /**
    Obtain a cursor on a database, returning a <code>SecondaryCursor</code>.
    Calling this method is the equivalent of calling {@link #openCursor} and
    casting the result to {@link SecondaryCursor}.
    <p>
    @param txn
    To use a cursor for writing to a transactional database, an explicit
    transaction must be specified.  For read-only access to a transactional
    database, the transaction may be null.  For a non-transactional database,
    the transaction must be null.
    <p>
    To transaction-protect cursor operations, cursors must be opened and closed
    within the context of a transaction, and the txn parameter specifies the
    transaction context in which the cursor will be used.
    <p>
    @param config
    The cursor attributes.  If null, default attributes are used.
    <p>
    @return
    A secondary database cursor.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public SecondaryCursor openSecondaryCursor(final Transaction txn,
                                               final CursorConfig config)
        throws DatabaseException {

        return new SecondaryCursor(this,
            CursorConfig.checkNull(config).openCursor(db,
                (txn == null) ? null : txn.txn), config);
    }

    /**
    Returns the primary database associated with this secondary database.
    <p>
    @return the primary database associated with this secondary database.
    */
    public Database getPrimaryDatabase() {
        return primaryDatabase;
    }

    /** {@inheritDoc} */
    public DatabaseConfig getConfig()
        throws DatabaseException {

        return getSecondaryConfig();
    }

    /**
    Returns a copy of the secondary configuration of this database.
    <p>
    @return a copy of the secondary configuration of this database.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public SecondaryConfig getSecondaryConfig()
        throws DatabaseException {

        return new SecondaryConfig(db);
    }

    /**
    Retrieves the key/data pair with the given key.  If the matching key has
duplicate values, the first data item in the set of duplicates is returned.
Retrieval of duplicates requires the use of {@link Cursor} operations.
<p>
@param txn
For a transactional database, an explicit transaction may be specified to
transaction-protect the operation, or null may be specified to perform the
operation without transaction protection.  For a non-transactional database,
null must be specified.
<p>
@param key the secondary key
used as input.  It must be initialized with a non-null byte array by the
caller.
<p>
@param pKey the primary key
returned as output.  Its byte array does not need to be initialized by the
caller.
<p>
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
<p>
@param lockMode the locking attributes; if null, default attributes are used.
<p>
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus get(final Transaction txn,
                               final DatabaseEntry key,
                               final DatabaseEntry pKey,
                               final DatabaseEntry data,
                               final LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.pget((txn == null) ? null : txn.txn, key, pKey, data,
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Retrieves the key/data pair with the specified secondary and primary key, that
is, both the primary and secondary key items must match.
<p>
@param txn
For a transactional database, an explicit transaction may be specified to
transaction-protect the operation, or null may be specified to perform the
operation without transaction protection.  For a non-transactional database,
null must be specified.
@param key the secondary key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param pKey the primary key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param data the primary data
returned as output.  Its byte array does not need to be initialized by the
caller.
<p>
@param lockMode the locking attributes; if null, default attributes are used.
<p>
@return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if no matching key/data pair is
found; {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the database is a Queue or Recno database and the specified key exists, but was never explicitly created by the application or was later deleted; otherwise, {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus getSearchBoth(final Transaction txn,
                                         final DatabaseEntry key,
                                         final DatabaseEntry pKey,
                                         final DatabaseEntry data,
                                         final LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.pget((txn == null) ? null : txn.txn, key, pKey, data,
                DbConstants.DB_GET_BOTH | LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Retrieves the key/data pair associated with the specific numbered record of the database.
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
    public OperationStatus getSearchRecordNumber(final Transaction txn,
                                                 final DatabaseEntry key,
                                                 final DatabaseEntry pKey,
                                                 final DatabaseEntry data,
                                                 final LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.pget((txn == null) ? null : txn.txn, key, pKey, data,
                DbConstants.DB_SET_RECNO | LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }
}
