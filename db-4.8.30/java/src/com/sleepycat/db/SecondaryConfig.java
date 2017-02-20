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
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbTxn;

/**
The configuration properties of a <code>SecondaryDatabase</code> extend
those of a primary <code>Database</code>.
The secondary database configuration is specified when calling {@link
Environment#openSecondaryDatabase Environment.openSecondaryDatabase}.
<p>
To create a configuration object with default attributes:
<pre>
    SecondaryConfig config = new SecondaryConfig();
</pre>
To set custom attributes:
<pre>
    SecondaryConfig config = new SecondaryConfig();
    config.setAllowCreate(true);
    config.setSortedDuplicates(true);
    config.setKeyCreator(new MyKeyCreator());
</pre>
<p>
<hr>
<p>
NOTE: There are two situations where the use of secondary databases without
transactions requires special consideration.  When using a transactional
database or when doing read operations only, this note does not apply.
<ul>
<li>If secondary is configured to not allow duplicates, when the secondary
is being updated it is possible that an error will occur when the secondary
key value in a record being added is already present in the database.  A
<code>DatabaseException</code> will be thrown in this situation.</li>
<li>If a foreign key constraint is configured with the delete action
<code>ABORT</code> (the default setting), a <code>DatabaseException</code>
will be thrown if an attempt is made to delete a referenced foreign
key.</li>
</ul>
In both cases, the operation will be partially complete because the primary
database record will have already been updated or deleted.  In the presence
of transactions, the exception will cause the transaction to abort.  Without
transactions, it is the responsibility of the caller to handle the results
of the incomplete update or to take steps to prevent this situation from
happening in the first place.
<p>
</hr>
<p>
@see Environment#openSecondaryDatabase Environment.openSecondaryDatabase
@see SecondaryDatabase
*/
public class SecondaryConfig extends DatabaseConfig implements Cloneable {
    /*
     * For internal use, to allow null as a valid value for
     * the config parameter.
     */
    public static final SecondaryConfig DEFAULT = new SecondaryConfig();

    /* package */
    static SecondaryConfig checkNull(SecondaryConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    private boolean allowPopulate;
    private boolean immutableSecondaryKey;
    private Db foreign;
    private ForeignKeyDeleteAction fkDelAction;
    private ForeignKeyNullifier keyNullifier;
    private ForeignMultiKeyNullifier multiKeyNullifier;
    private SecondaryKeyCreator keyCreator;
    private SecondaryMultiKeyCreator multiKeyCreator;

    /**
    Creates an instance with the system's default settings.
    */
    public SecondaryConfig() {
    }

    /**
    Specifies whether automatic population of the secondary is allowed.
    <p>
    If automatic population is allowed, when the secondary database is
    opened it is checked to see if it is empty.  If it is empty, the
    primary database is read in its entirety and keys are added to the
    secondary database using the information read from the primary.
    <p>
    If this property is set to true and the database is transactional, the
    population of the secondary will be done within the explicit or auto-commit
    transaction that is used to open the database.
    <p>
    @param allowPopulate whether automatic population of the secondary is
    allowed.
    */
    public void setAllowPopulate(final boolean allowPopulate) {
        this.allowPopulate = allowPopulate;
    }

    /**
    Returns whether automatic population of the secondary is allowed.  If
    {@link #setAllowPopulate} has not been called, this method returns
    false.
    <p>
    @return whether automatic population of the secondary is allowed.
    <p>
    @see #setAllowPopulate
    */
    public boolean getAllowPopulate() {
        return allowPopulate;
    }

    /**
    Specifies whether the secondary key is immutable.
    <p>
    Specifying that a secondary key is immutable can be used to optimize
    updates when the secondary key in a primary record will never be changed
    after that primary record is inserted.  For immutable secondary keys, a
    best effort is made to avoid calling
    <code>SecondaryKeyCreator.createSecondaryKey</code> when a primary record
    is updated.  This optimization may reduce the overhead of an update
    operation significantly if the <code>createSecondaryKey</code> operation is
    expensive.
    <p>
    Be sure to set this property to true only if the secondary key in the
    primary record is never changed.  If this rule is violated, the secondary
    index will become corrupted, that is, it will become out of sync with the
    primary.
    <p>
    @param immutableSecondaryKey whether the secondary key is immutable.
    */
    public void setImmutableSecondaryKey(final boolean immutableSecondaryKey) {
        this.immutableSecondaryKey = immutableSecondaryKey;
    }

    /**
    Returns whether the secondary key is immutable.  If
    {@link #setImmutableSecondaryKey} has not been called, this method returns
    false.
    <p>
    @return whether the secondary key is immutable.
    <p>
    @see #setImmutableSecondaryKey
    */
    public boolean getImmutableSecondaryKey() {
        return immutableSecondaryKey;
    }

    /**
    Specifies the user-supplied object used for creating single-valued
    secondary keys.
    <p>
    Unless the primary database is read-only, a key creator is required
    when opening a secondary database.  Either a KeyCreator or MultiKeyCreator
    must be specified, but both may not be specified.
    <p>
    Unless the primary database is read-only, a key creator is required
    when opening a secondary database.
    <p>
    @param keyCreator the user-supplied object used for creating single-valued
    secondary keys.
    */
    public void setKeyCreator(final SecondaryKeyCreator keyCreator) {
        this.keyCreator = keyCreator;
    }

    /**
    Returns the user-supplied object used for creating single-valued secondary
    keys.
    <p>
    @return the user-supplied object used for creating single-valued secondary
    keys.
    <p>
    @see #setKeyCreator
    */
    public SecondaryKeyCreator getKeyCreator() {
        return keyCreator;
    }

    /**
    Specifies the user-supplied object used for creating multi-valued
    secondary keys.
    <p>
    Unless the primary database is read-only, a key creator is required
    when opening a secondary database.  Either a KeyCreator or MultiKeyCreator
    must be specified, but both may not be specified.
    <p>
    @param multiKeyCreator the user-supplied object used for creating multi-valued
    secondary keys.
    */
    public void setMultiKeyCreator(final SecondaryMultiKeyCreator multiKeyCreator) {
        this.multiKeyCreator = multiKeyCreator;
    }

    /**
    Returns the user-supplied object used for creating multi-valued secondary
    keys.
    <p>
    @return the user-supplied object used for creating multi-valued secondary
    keys.
    <p>
    @see #setKeyCreator
    */
    public SecondaryMultiKeyCreator getMultiKeyCreator() {
        return multiKeyCreator;
    }

    public void setForeignKeyDatabase(Database foreignDb){
	this.foreign = foreignDb.db;
    }

    public Db getForeignKeyDatabase(){
	return foreign;
    }

    public void setForeignKeyDeleteAction(ForeignKeyDeleteAction action){
	this.fkDelAction = action;
    }

    public ForeignKeyDeleteAction getForeignKeyDeleteAction(){
	return fkDelAction;
    }

    public void setForeignKeyNullifier(ForeignKeyNullifier keyNullifier){
	this.keyNullifier = keyNullifier;
    }

    public ForeignKeyNullifier getForeignKeyNullifier(){
	return keyNullifier;
    }
    
    public void setForeignMultiKeyNullifier(ForeignMultiKeyNullifier multiKeyNullifier){
	this.multiKeyNullifier = multiKeyNullifier;
    }

    public ForeignMultiKeyNullifier getForeignMultiKeyNullifier(){
	return multiKeyNullifier;
    }

    /* package */
    Db openSecondaryDatabase(final DbEnv dbenv,
                             final DbTxn txn,
                             final String fileName,
                             final String databaseName,
                             final Db primary)
        throws DatabaseException, java.io.FileNotFoundException {
	int associateFlags = 0;
	int foreignFlags = 0;
        associateFlags |= allowPopulate ? DbConstants.DB_CREATE : 0;
        if (getTransactional() && txn == null)
            associateFlags |= DbConstants.DB_AUTO_COMMIT;
        if (immutableSecondaryKey)
            associateFlags |= DbConstants.DB_IMMUTABLE_KEY;

        final Db db = super.openDatabase(dbenv, txn, fileName, databaseName);
        boolean succeeded = false;
        try {
            /*
             * The multi-key creator must be set before the call to associate
             * so that we can work out whether the C API callback should be
             * set or not.
             */
            db.set_secmultikey_create(multiKeyCreator);
            primary.associate(txn, db, keyCreator, associateFlags);
 	    if (foreign != null){
		db.set_foreignmultikey_nullifier(multiKeyNullifier);
		foreign.associate_foreign(db, keyNullifier, foreignFlags | fkDelAction.getId());
	    }
            succeeded = true;
            return db;
        } finally {
            if (!succeeded)
                try {
                    db.close(0);
                } catch (Throwable t) {
                    // Ignore it -- there is already an exception in flight.
                }
        }
    }

    /* package */
    SecondaryConfig(final Db db)
        throws DatabaseException {

        super(db);

        // XXX: There is no way to find out what flags were passed to associate.
        allowPopulate = false;
        immutableSecondaryKey = false;
        keyCreator = db.get_seckey_create();
        multiKeyCreator = db.get_secmultikey_create();
    }
}

