/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.raw;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.StoreConfig;
import com.sleepycat.persist.StoreExistsException;
import com.sleepycat.persist.StoreNotFoundException;
import com.sleepycat.persist.evolve.IncompatibleClassException;
import com.sleepycat.persist.evolve.Mutations;
import com.sleepycat.persist.impl.Store;
import com.sleepycat.persist.model.EntityModel;

/**
 * Provides access to the raw data in a store for use by general purpose tools.
 * A <code>RawStore</code> provides access to stored entities without using
 * entity classes or key classes.  Keys are represented as simple type objects
 * or, for composite keys, as {@link RawObject} instances, and entities are
 * represented as {@link RawObject} instances.
 *
 * <p>{@code RawStore} objects are thread-safe.  Multiple threads may safely
 * call the methods of a shared {@code RawStore} object.</p>
 *
 * <p>When using a {@code RawStore}, the current persistent class definitions
 * are not used.  Instead, the previously stored metadata and class definitions
 * are used.  This has several implications:</p>
 * <ol>
 * <li>An {@code EntityModel} may not be specified using {@link
 * StoreConfig#setModel}.  In other words, the configured model must be
 * null (the default).</li>
 * <li>When storing entities, their format will not automatically be evolved
 * to the current class definition, even if the current class definition has
 * changed.</li>
 * </ol>
 *
 * @author Mark Hayes
 */
public class RawStore {

    private Store store;

    /**
     * Opens an entity store for raw data access.
     *
     * @param env an open Berkeley DB environment.
     *
     * @param storeName the name of the entity store within the given
     * environment.
     *
     * @param config the store configuration, or null to use default
     * configuration properties.
     *
     * @throws IllegalArgumentException if the <code>Environment</code> is
     * read-only and the <code>config ReadOnly</code> property is false.
     */
    public RawStore(Environment env, String storeName, StoreConfig config)
        throws StoreNotFoundException, DatabaseException {

        try {
            store = new Store(env, storeName, config, true /*rawAccess*/);
        } catch (StoreExistsException e) {
            /* Should never happen, ExclusiveCreate not used. */
            throw new RuntimeException(e);
        } catch (IncompatibleClassException e) {
            /* Should never happen, evolution is not performed. */
            throw new RuntimeException(e);
        }
    }

    /**
     * Opens the primary index for a given entity class.
     */
    public PrimaryIndex<Object,RawObject> getPrimaryIndex(String entityClass)
        throws DatabaseException {

        return store.getPrimaryIndex
            (Object.class, null, RawObject.class, entityClass);
    }

    /**
     * Opens the secondary index for a given entity class and secondary key
     * name.
     */
    public SecondaryIndex<Object,Object,RawObject>
        getSecondaryIndex(String entityClass, String keyName)
        throws DatabaseException {

        return store.getSecondaryIndex
            (getPrimaryIndex(entityClass), RawObject.class, entityClass,
             Object.class, null, keyName);
    }

    /**
     * Returns the environment associated with this store.
     */
    public Environment getEnvironment() {
        return store.getEnvironment();
    }

    /**
     * Returns a copy of the entity store configuration.
     */
    public StoreConfig getConfig() {
        return store.getConfig();
    }

    /**
     * Returns the name of this store.
     */
    public String getStoreName() {
        return store.getStoreName();
    }

    /**
     * Returns the last configured and stored entity model for this store.
     */
    public EntityModel getModel() {
        return store.getModel();
    }

    /**
     * Returns the set of mutations that were configured and stored previously.
     */
    public Mutations getMutations() {
        return store.getMutations();
    }

    /**
     * Closes all databases and sequences that were opened by this model.  No
     * databases opened via this store may be in use.
     */
    public void close()
        throws DatabaseException {

        store.close();
    }
}
