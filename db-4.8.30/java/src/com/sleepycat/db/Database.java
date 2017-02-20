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
import com.sleepycat.db.internal.DbSequence;
import com.sleepycat.db.internal.Dbc;

/**
A database handle.
<p>
Database attributes are specified in the {@link com.sleepycat.db.DatabaseConfig DatabaseConfig} class.
<p>
Database handles are free-threaded unless opened in an environment
that is not free-threaded.
<p>
To open an existing database with default attributes:
<blockquote><pre>
    Environment env = new Environment(home, null);
    Database myDatabase = env.openDatabase(null, "mydatabase", null);
</pre></blockquote>
To create a transactional database that supports duplicates:
<blockquote><pre>
    DatabaseConfig dbConfig = new DatabaseConfig();
    dbConfig.setTransactional(true);
    dbConfig.setAllowCreate(true);
    dbConfig.setSortedDuplicates(true);
    Database newlyCreateDb = env.openDatabase(txn, "mydatabase", dbConfig);
</pre></blockquote>
*/
public class Database {
    Db db;
    private int autoCommitFlag;
    int rmwFlag;

    /* package */
    Database(final Db db)
        throws DatabaseException {

        this.db = db;
        db.wrapper = this;
        this.autoCommitFlag =
            db.get_transactional() ? DbConstants.DB_AUTO_COMMIT : 0;
        rmwFlag = ((db.get_env().get_open_flags() &
                    DbConstants.DB_INIT_LOCK) != 0) ? DbConstants.DB_RMW : 0;
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
@param filename
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
@param config The database open attributes.  If null, default attributes are used.
    */
    public Database(final String filename,
                    final String databaseName,
                    final DatabaseConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        this(DatabaseConfig.checkNull(config).openDatabase(null, null,
            filename, databaseName));
        // Set up dbenv.wrapper
        new Environment(db.get_env());
    }

    /**
    Flush any cached database information to disk and discard the database
handle.
<p>
The database handle should not be closed while any other handle that
refers to it is not yet closed; for example, database handles should not
be closed while cursor handles into the database remain open, or
transactions that include operations on the database have not yet been
committed or aborted.  Specifically, this includes {@link com.sleepycat.db.Cursor Cursor} and
{@link com.sleepycat.db.Transaction Transaction} handles.
<p>
Because key/data pairs are cached in memory, failing to sync the file
with the {@link com.sleepycat.db.Database#close Database.close} or {@link com.sleepycat.db.Database#sync Database.sync} methods
may result in inconsistent or lost information.
<p>
When multiple threads are using the {@link com.sleepycat.db.Database Database} handle
concurrently, only a single thread may call this method.
<p>
The database handle may not be accessed again after this method is
called, regardless of the method's success or failure.
<p>
When called on a database that is the primary database for a secondary
index, the primary database should be closed only after all secondary
indices which reference it have been closed.
@param noSync
Do not flush cached information to disk.  The noSync parameter is a
dangerous option.  It should be set only if the application is doing
logging (with transactions) so that the database is recoverable after a
system or application crash, or if the database is always generated from
scratch after any system or application crash.
<b>
It is important to understand that flushing cached information to disk
only minimizes the window of opportunity for corrupted data.
<p>
</b>
Although unlikely, it is possible for database corruption to happen if
a system or application crash occurs while writing data to the database.
To ensure that database corruption never occurs, applications must
either: use transactions and logging with automatic recovery; use
logging and application-specific recovery; or edit a copy of the
database, and once all applications using the database have successfully
called this method, atomically replace the original database with the
updated copy.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void close(final boolean noSync)
        throws DatabaseException {

        db.close(noSync ? DbConstants.DB_NOSYNC : 0);
    }

    /**
    Flush any cached database information to disk and discard the database
handle.
<p>
The database handle should not be closed while any other handle that
refers to it is not yet closed; for example, database handles should not
be closed while cursor handles into the database remain open, or
transactions that include operations on the database have not yet been
committed or aborted.  Specifically, this includes {@link com.sleepycat.db.Cursor Cursor} and
{@link com.sleepycat.db.Transaction Transaction} handles.
<p>
Because key/data pairs are cached in memory, failing to sync the file
with the {@link com.sleepycat.db.Database#close Database.close} or {@link com.sleepycat.db.Database#sync Database.sync} methods
may result in inconsistent or lost information.
<p>
When multiple threads are using the {@link com.sleepycat.db.Database Database} handle
concurrently, only a single thread may call this method.
<p>
The database handle may not be accessed again after this method is
called, regardless of the method's success or failure.
<p>
When called on a database that is the primary database for a secondary
index, the primary database should be closed only after all secondary
indices which reference it have been closed.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public void close()
        throws DatabaseException {

        close(false);
    }

    /**
    Compact a Btree or Recno database or returns unused Btree,
    Hash or Recno database pages to the underlying filesystem.
    @param txn
    If the operation is part of an application-specified transaction, the txnid
    parameter is a transaction handle returned from {@link
    Environment#beginTransaction}, otherwise <code>null</code>.
    If no transaction handle is specified, but the operation occurs in a
    transactional database, the operation will be implicitly transaction
    protected using multiple transactions.  Transactions will be comitted at
    points to avoid holding much of the tree locked.
    Any deadlocks encountered will be cause the operation to retried from
    the point of the last commit.
    @param start
    If not <code>null</code>, the <code>start</code> parameter is the starting
    point for compaction in a Btree or Recno database.  Compaction will start
    at the smallest key greater than or equal to the specified key.  If
    <code>null</code>, compaction will start at the beginning of the database.
    @param stop
    If not <code>null</code>, the <code>stop</code> parameter is the stopping
    point for compaction in a Btree or Recno database.  Compaction will stop at
    the page with the smallest key greater than the specified key.  If
    <code>null</code>, compaction will stop at the end of the database.
    @param end
    If not <code>null</code>, the <code>end</code> parameter will be filled in
    with the key marking the end of the compaction operation in a Btree or
    Recno database. It is generally the first key of the page where processing
    stopped.
    @param config The compaction operation attributes.  If null, default attributes are used.
    **/
    public CompactStats compact(final Transaction txn,
                                final DatabaseEntry start,
                                final DatabaseEntry stop,
                                final DatabaseEntry end,
                                CompactConfig config)
        throws DatabaseException {

        config = CompactConfig.checkNull(config);
        CompactStats compact = new CompactStats(config.getFillPercent(),
            config.getTimeout(), config.getMaxPages());
        db.compact((txn == null) ? null : txn.txn,
            start, stop, compact, config.getFlags(), end);
        return compact;
    }

    /**
    Return a cursor into the database.
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
    A database cursor.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public Cursor openCursor(final Transaction txn, CursorConfig config)
        throws DatabaseException {

        return new Cursor(this, CursorConfig.checkNull(config).openCursor(
            db, (txn == null) ? null : txn.txn), config);
    }

    /**
    Open a sequence in the database.
    <p>
    @param txn
    For a transactional database, an explicit transaction may be specified, or
    null may be specified to use auto-commit.  For a non-transactional
    database, null must be specified.
    <p>
    @param key
    The key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} of the sequence.
    <p>
    @param config
    The sequence attributes.  If null, default attributes are used.
    <p>
    @return
    A sequence handle.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public Sequence openSequence(final Transaction txn,
                                 final DatabaseEntry key,
                                 final SequenceConfig config)
        throws DatabaseException {

        return new Sequence(SequenceConfig.checkNull(config).openSequence(
            db, (txn == null) ? null : txn.txn, key), config);
    }

    /**
    Remove the sequence from the database.  This method should not be called if
    there are open handles on this sequence.
    <p>
    @param txn
    For a transactional database, an explicit transaction may be specified, or
    null may be specified to use auto-commit.  For a non-transactional
    database, null must be specified.
    <p>
    @param key
    The key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} of the sequence.
    <p>
    @param config
    The sequence attributes.  If null, default attributes are used.
    */
    public void removeSequence(final Transaction txn,
                               final DatabaseEntry key,
                               SequenceConfig config)
        throws DatabaseException {

        config = SequenceConfig.checkNull(config);
        final DbSequence seq = config.openSequence(
            db, (txn == null) ? null : txn.txn, key);
        seq.remove((txn == null) ? null : txn.txn,
            (txn == null && db.get_transactional()) ?
            DbConstants.DB_AUTO_COMMIT | (config.getAutoCommitNoSync() ?
            DbConstants.DB_TXN_NOSYNC : 0) : 0);
    }

    /**
Return the database's underlying file name.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database's underlying file name.
    */
    public String getDatabaseFile()
        throws DatabaseException {

        return db.get_filename();
    }

    /**
Return the database name.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The database name.
    */
    public String getDatabaseName()
        throws DatabaseException {

        return db.get_dbname();
    }

    /**
    Return this Database object's configuration.
    <p>
    This may differ from the configuration used to open this object if
    the database existed previously.
    <p>
    @return
    This Database object's configuration.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public DatabaseConfig getConfig()
        throws DatabaseException {

        return new DatabaseConfig(db);
    }

    /**
    Change the settings in an existing database handle.
    <p>
    @param config The environment attributes.  If null, default attributes are used.
    <p>
    <p>
@throws IllegalArgumentException if an invalid parameter was specified.
<p>
@throws DatabaseException if a failure occurs.
    */
    public void setConfig(DatabaseConfig config)
        throws DatabaseException {

        config.configureDatabase(db, getConfig());
    }

    /**
Return the {@link com.sleepycat.db.Environment Environment} handle for the database environment
    underlying the {@link com.sleepycat.db.Database Database}.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The {@link com.sleepycat.db.Environment Environment} handle for the database environment
    underlying the {@link com.sleepycat.db.Database Database}.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public Environment getEnvironment()
        throws DatabaseException {

        return db.get_env().wrapper;
    }

    /**
Return the handle for the cache file underlying the database.
<p>
This method may be called at any time during the life of the application.
<p>
@return
The handle for the cache file underlying the database.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public CacheFile getCacheFile()
        throws DatabaseException {

        return new CacheFile(db.get_mpf());
    }

    /**
    <p>
Append the key/data pair to the end of the database.
<p>
The underlying database must be a Queue or Recno database.  The record
number allocated to the record is returned in the key parameter.
<p>
There is a minor behavioral difference between the Recno and Queue
access methods this method.  If a transaction enclosing this method
aborts, the record number may be decremented (and later reallocated by
a subsequent operation) in the Recno access method, but will not be
decremented or reallocated in the Queue access method.
<p>
It may be useful to modify the stored data based on the generated key.
If a callback function is specified using {@link com.sleepycat.db.DatabaseConfig#setRecordNumberAppender DatabaseConfig.setRecordNumberAppender}, it will be called after the record number has
been selected, but before the data has been stored.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus append(final Transaction txn,
                                  final DatabaseEntry key,
                                  final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.put((txn == null) ? null : txn.txn, key, data,
                DbConstants.DB_APPEND | ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    Return the record number and data from the available record closest to
the head of the queue, and delete the record.  The record number will be
returned in the <code>key</code> parameter, and the data will be returned
in the <code>data</code> parameter.  A record is available if it is not
deleted and is not currently locked.  The underlying database must be
of type Queue for this method to be called.
<p>
@param txn
For a transactional database, an explicit transaction may be specified to
transaction-protect the operation, or null may be specified to perform the
operation without transaction protection.  For a non-transactional database,
null must be specified.
@param key the  key
returned as output.  Its byte array does not need to be initialized by the
caller.
@param data the  data
returned as output.  Its byte array does not need to be initialized by the
caller.
@param wait
if there is no record available, this parameter determines whether the
method waits for one to become available, or returns immediately with
status <code>NOTFOUND</code>.
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
    public OperationStatus consume(final Transaction txn,
                                   final DatabaseEntry key,
                                   final DatabaseEntry data,
                                   final boolean wait)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.get((txn == null) ? null : txn.txn,
                key, data,
                (wait ? DbConstants.DB_CONSUME_WAIT : DbConstants.DB_CONSUME) |
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    Remove key/data pairs from the database.
    <p>
    The key/data pair associated with the specified key is discarded
    from the database.  In the presence of duplicate key values, all
    records associated with the designated key will be discarded.
    <p>
    The key/data pair is also deleted from any associated secondary
    databases.
    <p>
    @param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
    <p>
    @param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
    <p>
    @return
    The method will return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if the
    specified key is not found in the database;
        The method will return {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the
    database is a Queue or Recno database and the specified key exists,
    but was never explicitly created by the application or was later
    deleted;
    otherwise the method will return {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    <p>
    <p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus delete(final Transaction txn,
                                  final DatabaseEntry key)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.del((txn == null) ? null : txn.txn, key,
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    Remove key/data pairs from the database.
    <p>
    The key/data pair associated with the specified key is discarded
    from the database.  In the presence of duplicate key values, all
    records associated with the designated key will be discarded.
    <p>
    The key/data pair is also deleted from any associated secondary
    databases.
    <p>
    @param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
    <p>
    @param keys the set of keys {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
    <p>
    @return
    The method will return {@link com.sleepycat.db.OperationStatus#NOTFOUND OperationStatus.NOTFOUND} if the
    specified key is not found in the database;
        The method will return {@link com.sleepycat.db.OperationStatus#KEYEMPTY OperationStatus.KEYEMPTY} if the
    database is a Queue or Recno database and the specified key exists,
    but was never explicitly created by the application or was later
    deleted;
    otherwise the method will return {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}.
    <p>
    <p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus deleteMultiple(final Transaction txn,
                                  final MultipleEntry keys)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.del((txn == null) ? null : txn.txn, keys,
                DbConstants.DB_MULTIPLE | 
		((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    Checks if the specified key appears in the database.
<p>
@param txn
For a transactional database, an explicit transaction may be specified to
transaction-protect the operation, or null may be specified to perform the
operation without transaction protection.  For a non-transactional database,
null must be specified.
<p>
@param key the  key
used as input.  It must be initialized with a non-null byte array by the
caller.
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
    public OperationStatus exists(final Transaction txn,
                               final DatabaseEntry key)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.exists((txn == null) ? null : txn.txn, key,
                ((txn == null) ? autoCommitFlag : 0)));
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
@param key the  key
used as input.  It must be initialized with a non-null byte array by the
caller.
<p>
<p>
@param data the  data
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
                               final DatabaseEntry data,
                               final LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.get((txn == null) ? null : txn.txn,
                key, data,
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    Return an estimate of the proportion of keys in the database less
    than, equal to, and greater than the specified key.
    <p>
    The underlying database must be of type Btree.
    <p>
    This method does not retain the locks it acquires for the life of
    the transaction, so estimates are not repeatable.
    <p>
    @param key
    The key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} being compared.
    <p>
    @param txn
For a transactional database, an explicit transaction may be specified to
transaction-protect the operation, or null may be specified to perform the
operation without transaction protection.  For a non-transactional database,
null must be specified.
    <p>
    @return
    An estimate of the proportion of keys in the database less than,
    equal to, and greater than the specified key.
    <p>
    <p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public KeyRange getKeyRange(final Transaction txn,
                                final DatabaseEntry key)
        throws DatabaseException {

        final KeyRange range = new KeyRange();
        db.key_range((txn == null) ? null : txn.txn, key, range, 0);
        return range;
    }

    /**
    Retrieves the key/data pair with the given key and data value, that is, both
the key and data items must match.
<p>
@param txn
For a transactional database, an explicit transaction may be specified to
transaction-protect the operation, or null may be specified to perform the
operation without transaction protection.  For a non-transactional database,
null must be specified.
@param key the  key
used as input.  It must be initialized with a non-null byte array by the
caller.
@param data the  data
used as input.  It must be initialized with a non-null byte array by the
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
                                         final DatabaseEntry data,
                                         final LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.get((txn == null) ? null : txn.txn,
                key, data,
                DbConstants.DB_GET_BOTH |
                LockMode.getFlag(lockMode) |
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
    public OperationStatus getSearchRecordNumber(final Transaction txn,
                                           final DatabaseEntry key,
                                           final DatabaseEntry data,
                                           final LockMode lockMode)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.get((txn == null) ? null : txn.txn,
                key, data,
                DbConstants.DB_SET_RECNO |
                LockMode.getFlag(lockMode) |
                ((data == null) ? 0 : data.getMultiFlag())));
    }

    /**
    <p>
Store the key/data pair into the database.
<p>
If the key already appears in the database and duplicates are not
configured, the existing key/data pair will be replaced.  If the key
already appears in the database and sorted duplicates are configured,
the new data value is inserted at the correct sorted location.
If the key already appears in the database and unsorted duplicates are
configured, the new data value is appended at the end of the duplicate
set.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus put(final Transaction txn,
                               final DatabaseEntry key,
                               final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.put((txn == null) ? null : txn.txn,
                key, data,
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    <p>
Store a set of key/data pairs into the database.
<p>
The key and data parameters must contain corresponding key/data pairs. That is
the first entry in the multiple key is inserted with the first entry from the
data parameter. Similarly for all remaining keys in the set.
<p>
This method may not be called on databases configured with unsorted duplicates.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param key the set of keys {@link com.sleepycat.db.MultipleEntry MultipleEntry} operated on.
<p>
@param data the set of data {@link com.sleepycat.db.MultipleEntry MultipleEntry} stored.
<p>
@param overwrite if this flag is true and any of the keys already exist in the database, they will be replaced. Otherwise a KEYEXIST error will be returned.
<p>
@return
If any of the key/data pairs already appear in the database, this method will
return {@link com.sleepycat.db.OperationStatus#KEYEXIST OperationStatus.KEYEXIST}.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putMultiple(final Transaction txn,
                                       final MultipleEntry key,
                                       final MultipleEntry data,
				       final boolean overwrite)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.put((txn == null) ? null : txn.txn,
                key, data,
                DbConstants.DB_MULTIPLE |
		(overwrite ? DbConstants.DB_OVERWRITE : 0) |
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    <p>
Store a set of key/data pairs into the database.
<p>
This method may not be called on databases configured with unsorted duplicates.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param key the key and data sets {@link com.sleepycat.db.MultipleEntry MultipleEntry} operated on.
<p>
@param overwrite if this flag is true and any of the keys already exist in the database, they will be replaced. Otherwise a KEYEXIST error will be returned.
<p>
@return
If any of the key/data pairs already appear in the database, this method will
return {@link com.sleepycat.db.OperationStatus#KEYEXIST OperationStatus.KEYEXIST}.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putMultipleKey(final Transaction txn,
                                          final MultipleEntry key,
					  final boolean overwrite)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.put((txn == null) ? null : txn.txn,
                key, null, 
                DbConstants.DB_MULTIPLE_KEY |
		(overwrite ? DbConstants.DB_OVERWRITE : 0) |
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    <p>
Store the key/data pair into the database if it does not already appear
in the database.
<p>
This method may only be called if the underlying database has been
configured to support sorted duplicates.
(This method may not be specified to the Queue or Recno access methods.)
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
@return
If the key/data pair already appears in the database, this method will
return {@link com.sleepycat.db.OperationStatus#KEYEXIST OperationStatus.KEYEXIST}.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putNoDupData(final Transaction txn,
                                        final DatabaseEntry key,
                                        final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.put((txn == null) ? null : txn.txn,
                key, data,
                DbConstants.DB_NODUPDATA |
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    <p>
Store the key/data pair into the database if the key does not already
appear in the database.
<p>
This method will fail if the key already exists in the database, even
if the database supports duplicates.
<p>
@param txn
For a transactional database, an explicit transaction may be specified, or null
may be specified to use auto-commit.  For a non-transactional database, null
must be specified.
<p>
@param key the key {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} operated on.
<p>
@param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry} stored.
<p>
@return
If the key already appears in the database, this method will return
{@link com.sleepycat.db.OperationStatus#KEYEXIST OperationStatus.KEYEXIST}.
<p>
<p>
@throws DeadlockException if the operation was selected to resolve a
deadlock.
<p>
@throws DatabaseException if a failure occurs.
    */
    public OperationStatus putNoOverwrite(final Transaction txn,
                                          final DatabaseEntry key,
                                          final DatabaseEntry data)
        throws DatabaseException {

        return OperationStatus.fromInt(
            db.put((txn == null) ? null : txn.txn,
                key, data,
                DbConstants.DB_NOOVERWRITE |
                ((txn == null) ? autoCommitFlag : 0)));
    }

    /**
    Creates a specialized join cursor for use in performing equality or
    natural joins on secondary indices.
    <p>
    Each cursor in the <code>cursors</code> array must have been
    initialized to refer to the key on which the underlying database should
    be joined.  Typically, this initialization is done by calling
    {@link Cursor#getSearchKey Cursor.getSearchKey}.
    <p>
    Once the cursors have been passed to this method, they should not be
    accessed or modified until the newly created join cursor has been
    closed, or else inconsistent results may be returned.  However, the
    position of the cursors will not be changed by this method or by the
    methods of the join cursor.
    <p>
    @param cursors an array of cursors associated with this primary
    database.
    <p>
    @param config The join attributes.  If null, default attributes are used.
    <p>
    @return
    a specialized cursor that returns the results of the equality join
    operation.
    <p>
    @throws DatabaseException if a failure occurs.
    <p>
    @see JoinCursor
    */
    public JoinCursor join(final Cursor[] cursors, JoinConfig config)
        throws DatabaseException {

        config = JoinConfig.checkNull(config);

        final Dbc[] dbcList = new Dbc[cursors.length];
        for (int i = 0; i < cursors.length; i++)
            dbcList[i] = (cursors[i] == null) ? null : cursors[i].dbc;

        return new JoinCursor(this,
            db.join(dbcList, config.getFlags()), config);
    }

    /**
    Empty the database, discarding all records it contains.
    <p>
    When called on a database configured with secondary indices, this
    method truncates the primary database and all secondary indices.  If
    configured to return a count of the records discarded, the returned
    count is the count of records discarded from the primary database.
    <p>
    It is an error to call this method on a database with open cursors.
    <p>
    @param txn
    For a transactional database, an explicit transaction may be specified, or
    null may be specified to use auto-commit.  For a non-transactional
    database, null must be specified.
    <p>
    @param countRecords
    If true, count and return the number of records discarded.
    <p>
    @return
    The number of records discarded, or -1 if returnCount is false.
    <p>
    @throws DeadlockException if the operation was selected to resolve a
    deadlock.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public int truncate(final Transaction txn, boolean countRecords)
        throws DatabaseException {

        // XXX: implement countRecords in C
        int count = db.truncate((txn == null) ? null : txn.txn,
            ((txn == null) ? autoCommitFlag : 0));

        return countRecords ? count : -1;
    }

    /**
    Return database statistics.
    <p>
    If this method has not been configured to avoid expensive operations
    (using the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method), it will access
    some of or all the pages in the database, incurring a severe
    performance penalty as well as possibly flushing the underlying
        buffer pool.
    <p>
    In the presence of multiple threads or processes accessing an active
    database, the information returned by this method may be out-of-date.
        <p>
    If the database was not opened read-only and this method was not
    configured using the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, cached
    key and record numbers will be updated after the statistical
    information has been gathered.
    <p>
    @param txn
    For a transactional database, an explicit transaction may be specified to
    transaction-protect the operation, or null may be specified to perform the
    operation without transaction protection.  For a non-transactional
    database, null must be specified.
    <p>
    @param config
    The statistics returned; if null, default statistics are returned.
    <p>
    @return
    Database statistics.
    <p>
    @throws DeadlockException if the operation was selected to resolve a
    deadlock.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public DatabaseStats getStats(final Transaction txn, StatsConfig config)
        throws DatabaseException {

        return (DatabaseStats)db.stat((txn == null) ? null : txn.txn,
            StatsConfig.checkNull(config).getFlags());
    }

    /**
    <p>
Remove a database.
<p>
If no database is specified, the underlying file specified is removed.
<p>
Applications should never remove databases with open {@link com.sleepycat.db.Database Database}
handles, or in the case of removing a file, when any database in the
file has an open handle.  For example, some architectures do not permit
the removal of files with open system handles.  On these architectures,
attempts to remove databases currently in use by any thread of control
in the system may fail.
<p>
If the database was opened within a database environment, the
environment variable DB_HOME may be used as the path of the database
environment home.
<p>
This method is affected by any database directory specified with
{@link com.sleepycat.db.EnvironmentConfig#addDataDir EnvironmentConfig.addDataDir}, or by setting the "set_data_dir"
string in the database environment's DB_CONFIG file.
<p>
The {@link com.sleepycat.db.Database Database} handle may not be accessed
again after this method is called, regardless of this method's success
or failure.
<p>
@param fileName
The physical file which contains the database to be removed.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param databaseName
The database to be removed.
<p>
@param config The database remove attributes.  If null, default attributes are used.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public static void remove(final String fileName,
                              final String databaseName,
                              DatabaseConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        final Db db = DatabaseConfig.checkNull(config).createDatabase(null);
        db.remove(fileName, databaseName, 0);
    }

    /**
    <p>
Rename a database.
<p>
If no database name is specified, the underlying file specified is
renamed, incidentally renaming all of the databases it contains.
<p>
Applications should never rename databases that are currently in use.
If an underlying file is being renamed and logging is currently enabled
in the database environment, no database in the file may be open when
this method is called.  In particular, some architectures do not permit
renaming files with open handles.  On these architectures, attempts to
rename databases that are currently in use by any thread of control in
the system may fail.
<p>
If the database was opened within a database environment, the
environment variable DB_HOME may be used as the path of the database
environment home.
<p>
This method is affected by any database directory specified with
{@link com.sleepycat.db.EnvironmentConfig#addDataDir EnvironmentConfig.addDataDir}, or by setting the "set_data_dir"
string in the database environment's DB_CONFIG file.
<p>
The {@link com.sleepycat.db.Database Database} handle may not be accessed
again after this method is called, regardless of this method's success
or failure.
<p>
@param fileName
The physical file which contains the database to be renamed.
On Windows platforms, this argument will be interpreted as a UTF-8
string, which is equivalent to ASCII for Latin characters.
<p>
@param oldDatabaseName
The database to be renamed.
<p>
@param newDatabaseName
The new name of the database or file.
<p>
@param config The database rename attributes.  If null, default attributes are used.
<p>
<p>
@throws DatabaseException if a failure occurs.
    */
    public static void rename(final String fileName,
                              final String oldDatabaseName,
                              final String newDatabaseName,
                              DatabaseConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        final Db db = DatabaseConfig.checkNull(config).createDatabase(null);
        db.rename(fileName, oldDatabaseName, newDatabaseName, 0);
    }

    /**
    Sorts a DatabaseEntry with multiple matching key/data pairs.
    <p>
    If specified, the application specific btree comparison and duplicate 
    comparison functions will be used.
    <p>
    @param entries
    A MultipleKeyDataEntry that contains matching pairs of key/data items,
    the sorted entries will be returned in the original entries object.
    */
    public static void sortMultipleKeyData(MultipleKeyDataEntry entries)
        throws DatabaseException {
        Database.sortMultiple(entries, null);
    }

    /**
    Sorts a DatabaseEntry with multiple key or data pairs.
    <p>
    If specified, the application specific btree comparison function will be
    used.
    <p>
    @param entries
    A MultipleDataEntry that contains multiple key or data items,
    the sorted entries will be returned in the original entries object.
    */
    public static void sortMultipleKeyOrData(MultipleDataEntry entries)
        throws DatabaseException {
        Database.sortMultiple(entries, null);
    }

    /**
    Sorts two DatabaseEntry objects with multiple key and data pairs.
    <p>
    If specified, the application specific btree comparison and duplicate 
    comparison functions will be used.
    <p>
    The key and data parameters must contain "pairs" of items. That is the n-th
    entry in keys corresponds to the n-th entry in datas.
    @param keys
    A MultipleDataEntry that contains multiple key items, the sorted entries
    will be returned in the original entries object.
    @param datas
    A MultipleDataEntry that contains multiple data items, the sorted entries
    will be returned in the original entries object.
    */
    public static void sortMultipleKeyAndData(
        MultipleDataEntry keys, MultipleDataEntry datas)
        throws DatabaseException {
        Database.sortMultiple(keys, datas);
    }

    private static void sortMultiple(MultipleEntry keys, MultipleEntry datas)
        throws DatabaseException {
        final Db db = DatabaseConfig.DEFAULT.createDatabase(null);
        db.sort_multiple(keys, datas);
        db.close(0);
    }
    	

    /**
    Flush any cached information to disk.
    <p>
    If the database is in memory only, this method has no effect and
    will always succeed.
    <p>
    <b>It is important to understand that flushing cached information to
    disk only minimizes the window of opportunity for corrupted data.</b>
    <p>
    Although unlikely, it is possible for database corruption to happen
    if a system or application crash occurs while writing data to the
    database.  To ensure that database corruption never occurs,
    applications must either: use transactions and logging with
    automatic recovery; use logging and application-specific recovery;
    or edit a copy of the database, and once all applications using the
    database have successfully closed the copy of the database,
    atomically replace the original database with the updated copy.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public void sync()
        throws DatabaseException {

        db.sync(0);
    }

    /**
    Upgrade all of the databases included in the specified file.
    <p>
    If no upgrade is necessary, always returns success.
    <p>
    <b>
    Database upgrades are done in place and are destructive. For example,
    if pages need to be allocated and no disk space is available, the
    database may be left corrupted.  Backups should be made before databases
    are upgraded.
    </b>
    <p>
    <b>
    The following information is only meaningful when upgrading databases
    from releases before the Berkeley DB 3.1 release:
    </b>
    <p>
    As part of the upgrade from the Berkeley DB 3.0 release to the 3.1
    release, the on-disk format of duplicate data items changed.  To
    correctly upgrade the format requires applications to specify
    whether duplicate data items in the database are sorted or not.
    Configuring the database object to support sorted duplicates by the
    {@link com.sleepycat.db.DatabaseConfig#setSortedDuplicates DatabaseConfig.setSortedDuplicates} method informs this
    method that the duplicates are sorted; otherwise they are assumed
    to be unsorted.  Incorrectly specifying this configuration
    information may lead to database corruption.
    <p>
    Further, because this method upgrades a physical file (including all
    the databases it contains), it is not possible to use this method
    to upgrade files in which some of the databases it includes have
    sorted duplicate data items, and some of the databases it includes
    have unsorted duplicate data items.  If the file does not have more
    than a single database, if the databases do not support duplicate
    data items, or if all of the databases that support duplicate data
    items support the same style of duplicates (either sorted or
    unsorted), this method will work correctly as long as the duplicate
    configuration is correctly specified.  Otherwise, the file cannot
    be upgraded using this method; it must be upgraded manually by
    dumping and reloading the databases.
    <p>
    Unlike all other database operations, upgrades may only be done on
    a system with the same byte-order as the database.
    <p>
    @param fileName
    The physical file containing the databases to be upgraded.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public static void upgrade(final String fileName,
                        DatabaseConfig config)
        throws DatabaseException, java.io.FileNotFoundException {

        final Db db = DatabaseConfig.checkNull(config).createDatabase(null);
        db.upgrade(fileName,
            config.getSortedDuplicates() ? DbConstants.DB_DUPSORT : 0);
        db.close(0);
    }

    /**
    Return if all of the databases in a file are uncorrupted.
    <p>
    This method optionally outputs the databases' key/data pairs to a
    file stream.
    <p>
    <b>
    This method does not perform any locking, even in database
    environments are configured with a locking subsystem.  As such, it
    should only be used on files that are not being modified by another
    thread of control.
    </b>
    <p>
    This method may not be called after the database is opened.
    <p>
    If the database was opened within a database environment, the
environment variable DB_HOME may be used as the path of the database
environment home.
<p>
This method is affected by any database directory specified with
{@link com.sleepycat.db.EnvironmentConfig#addDataDir EnvironmentConfig.addDataDir}, or by setting the "set_data_dir"
string in the database environment's DB_CONFIG file.
    <p>
    The {@link com.sleepycat.db.Database Database} handle may not be accessed
again after this method is called, regardless of this method's success
or failure.
    <p>
    @param fileName
    The physical file in which the databases to be verified are found.
    <p>
    @param databaseName
    The database in the file on which the database checks for btree and
    duplicate sort order and for hashing are to be performed.  This
    parameter should be set to null except when the operation has been
    been configured by {@link com.sleepycat.db.VerifyConfig#setOrderCheckOnly VerifyConfig.setOrderCheckOnly}.
    <p>
    @param dumpStream
    An optional file stream to which the databases' key/data pairs are
    written.  This parameter should be set to null except when the
    operation has been been configured by {@link com.sleepycat.db.VerifyConfig#setSalvage VerifyConfig.setSalvage}.
    <p>
    @param verifyConfig The verify operation attributes.  If null, default attributes are used.
    <p>
    @param dbConfig The database attributes.  If null, default attributes are used.
    <p>
    @return
    True, if all of the databases in the file are uncorrupted.  If this
    method returns false, and the operation was configured by
    {@link com.sleepycat.db.VerifyConfig#setSalvage VerifyConfig.setSalvage}, all of the key/data pairs in the
    file may not have been successfully output.
    <p>
    <p>
@throws DatabaseException if a failure occurs.
    */
    public static boolean verify(final String fileName,
                                 final String databaseName,
                                 final java.io.PrintStream dumpStream,
                                 VerifyConfig verifyConfig,
                                 DatabaseConfig dbConfig)
        throws DatabaseException, java.io.FileNotFoundException {

        final Db db = DatabaseConfig.checkNull(dbConfig).createDatabase(null);
        return db.verify(fileName, databaseName, dumpStream,
            VerifyConfig.checkNull(verifyConfig).getFlags());
    }
}
