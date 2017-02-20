/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment; // for javadoc
import com.sleepycat.persist.evolve.IncompatibleClassException;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.model.AnnotationModel;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawStore; // for javadoc

/**
 * Configuration properties used with an {@link EntityStore} or {@link
 * RawStore}.
 *
 * <p>{@code StoreConfig} objects are thread-safe.  Multiple threads may safely
 * call the methods of a shared {@code StoreConfig} object.</p>
 *
 * <p>See the {@link <a href="package-summary.html#example">package
 * summary example</a>} for an example of using a {@code StoreConfig}.</p>
 *
 * @author Mark Hayes
 */
public class StoreConfig implements Cloneable {

    /**
     * The default store configuration containing properties as if the
     * configuration were constructed and not modified.
     */
    public static final StoreConfig DEFAULT = new StoreConfig();

    private boolean allowCreate;
    private boolean exclusiveCreate;
    private boolean transactional;
    private boolean readOnly;
    private boolean secondaryBulkLoad;
    private EntityModel model;
    private Mutations mutations;
    private DatabaseNamer databaseNamer = DatabaseNamer.DEFAULT;

    /**
     * Creates an entity store configuration object with default properties.
     */
    public StoreConfig() {
    }

    /**
     * Returns a shallow copy of the configuration.
     *
     * @deprecated As of JE 4.0.13, replaced by {@link
     * StoreConfig#clone()}.</p>
     */
    public StoreConfig cloneConfig() {
        try {
            return (StoreConfig) super.clone();
        } catch (CloneNotSupportedException cannotHappen) {
            return null;
        }
    }

    /**
     * Returns a shallow copy of the configuration.
     */
    @Override
    public StoreConfig clone() {
        try {
            return (StoreConfig) super.clone();
        } catch (CloneNotSupportedException cannotHappen) {
            return null;
        }
    }

    /**
     * Specifies whether creation of a new store is allowed.  By default this
     * property is false.
     *
     * <p>If this property is false and the internal store metadata database
     * does not exist, {@link DatabaseException} will be thrown when the store
     * is opened.</p>
     */
    public void setAllowCreate(boolean allowCreate) {
        this.allowCreate = allowCreate;
    }

    /**
     * Returns whether creation of a new store is allowed.
     */
    public boolean getAllowCreate() {
        return allowCreate;
    }

    /**
     * Specifies whether opening an existing store is prohibited.  By default
     * this property is false.
     *
     * <p>If this property is true and the internal store metadata database
     * already exists, {@link DatabaseException} will be thrown when the store
     * is opened.</p>
     */
    public void setExclusiveCreate(boolean exclusiveCreate) {
        this.exclusiveCreate = exclusiveCreate;
    }

    /**
     * Returns whether opening an existing store is prohibited.
     */
    public boolean getExclusiveCreate() {
        return exclusiveCreate;
    }

    /**
     * Sets the transactional configuration property.  By default this property
     * is false.
     *
     * <p>This property is true to open all store indices for transactional
     * access.  True may not be specified if the environment is not also
     * transactional.</p>
     */
    public void setTransactional(boolean transactional) {
        this.transactional = transactional;
    }

    /**
     * Returns the transactional configuration property.
     */
    public boolean getTransactional() {
        return transactional;
    }

    /**
     * Sets the read-only configuration property.  By default this property is
     * false.
     *
     * <p>This property is true to open all store indices for read-only access,
     * or false to open them for read-write access.  False may not be specified
     * if the environment is read-only.</p>
     */
    public void setReadOnly(boolean readOnly) {
        this.readOnly = readOnly;
    }

    /**
     * Returns the read-only configuration property.
     */
    public boolean getReadOnly() {
        return readOnly;
    }


    /**
     * Sets the bulk-load-secondaries configuration property.  By default this
     * property is false.
     *
     * <p>This property is true to cause the initial creation of secondary
     * indices to be performed as a bulk load.  If this property is true and
     * {@link EntityStore#getSecondaryIndex EntityStore.getSecondaryIndex} has
     * never been called for a secondary index, that secondary index will not
     * be created or written as records are written to the primary index.  In
     * addition, if that secondary index defines a foreign key constraint, the
     * constraint will not be enforced.</p>
     *
     * <p>The secondary index will be populated later when the {code
     * getSecondaryIndex} method is called for the first time for that index,
     * or when the store is closed and re-opened with this property set to
     * false and the primary index is obtained.  In either case, the secondary
     * index is populated by reading through the entire primary index and
     * adding records to the secondary index as needed.  While populating the
     * secondary, foreign key constraints will be enforced and an exception is
     * thrown if a constraint is violated.</p>
     *
     * <p>When loading a primary index along with secondary indexes from a
     * large input data set, configuring a bulk load of the secondary indexes
     * is sometimes more performant than updating the secondary indexes each
     * time the primary index is updated.  The absence of foreign key
     * constraints during the load also provides more flexibility.</p>
     */
    public void setSecondaryBulkLoad(boolean secondaryBulkLoad) {
        this.secondaryBulkLoad = secondaryBulkLoad;
    }

    /**
     * Returns the bulk-load-secondaries configuration property.
     */
    public boolean getSecondaryBulkLoad() {
        return secondaryBulkLoad;
    }

    /**
     * Sets the entity model that defines entity classes and index keys.
     *
     * <p>If null is specified or this method is not called, an {@link
     * AnnotationModel} instance is used by default.</p>
     */
    public void setModel(EntityModel model) {
        this.model = model;
    }

    /**
     * Returns the entity model that defines entity classes and index keys.
     */
    public EntityModel getModel() {
        return model;
    }

    /**
     * Configures mutations for performing lazy evolution of stored instances.
     * Existing mutations for this store are not cleared, so the mutations
     * required are only those changes that have been made since the store was
     * last opened.  Some new mutations may override existing specifications,
     * and some may be supplemental.
     *
     * <p>If null is specified and the store already exists, the previously
     * specified mutations are used.  The mutations are stored persistently in
     * serialized form.</p>
     *
     * <p>Mutations must be available to handle all changes to classes that are
     * incompatible with the class definitions known to this store.  See {@link
     * Mutations} and {@link com.sleepycat.persist.evolve Class Evolution} for
     * more information.</p>
     *
     * <p>If an incompatible class change has been made and mutations are not
     * available for handling the change, {@link IncompatibleClassException}
     * will be thrown when creating an {@link EntityStore}.</p>
     */
    public void setMutations(Mutations mutations) {
        this.mutations = mutations;
    }

    /**
     * Returns the configured mutations for performing lazy evolution of stored
     * instances.
     */
    public Mutations getMutations() {
        return mutations;
    }

    /**
     * Specifies the object reponsible for naming of files and databases.
     *
     * By default this property is {@link DatabaseNamer#DEFAULT}.
     *
     * @throws NullPointerException if a null parameter value is passed.
     */
    public void setDatabaseNamer(DatabaseNamer databaseNamer) {
        if (databaseNamer == null) {
            throw new NullPointerException();
        }
        this.databaseNamer = databaseNamer;
    }

    /**
     * Returns the object reponsible for naming of files and databases.
     */
    public DatabaseNamer getDatabaseNamer() {
        return databaseNamer;
    }
}
