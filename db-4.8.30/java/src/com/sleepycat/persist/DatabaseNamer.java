/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist;

import com.sleepycat.db.Database; // for javadoc

/**
 * Determines the file names to use for primary and secondary databases.
 *
 * <p>Each {@link PrimaryIndex} and {@link SecondaryIndex} is represented
 * internally as a Berkeley DB {@link Database}.  The file names of primary and
 * secondary indices must be unique within the environment, so that each index
 * is stored in a separate database file.</p>
 *
 * <p>By default, the file names of primary and secondary databases are
 * defined as follows.</p>
 *
 * <p>The syntax of a primary index database file name is:</p>
 * <pre>   STORE_NAME-ENTITY_CLASS</pre>
 * <p>Where STORE_NAME is the name parameter passed to {@link
 * EntityStore#EntityStore EntityStore} and ENTITY_CLASS is name of the class
 * passed to {@link EntityStore#getPrimaryIndex getPrimaryIndex}.</p>
 *
 * <p>The syntax of a secondary index database file name is:</p>
 * <pre>   STORE_NAME-ENTITY_CLASS-KEY_NAME</pre>
 * <p>Where KEY_NAME is the secondary key name passed to {@link
 * EntityStore#getSecondaryIndex getSecondaryIndex}.</p>
 *
 * <p>The default naming described above is implemented by the built-in {@link
 * DatabaseNamer#DEFAULT} object.  An application may supply a custom {@link
 * DatabaseNamer} to overrride the default naming scheme.  For example, a
 * custom namer could place all database files in a subdirectory with the name
 * of the store.  A custom namer could also be used to name files according to
 * specific file system restrictions.</p>
 *
 * <p>The custom namer object must be an instance of the {@code DatabaseNamer}
 * interface and is configured using {@link StoreConfig#setDatabaseNamer
 * setDatabaseNamer}.</p>
 *
 * <p>When copying or removing all databases in a store, there is one further
 * consideration.  There are two internal databases that must be kept with the
 * other databases in the store in order for the store to be used.  These
 * contain the data formats and sequences for the store.  Their entity class
 * names are:</p>
 *
 * <pre>   com.sleepycat.persist.formats</pre>
 * <pre>   com.sleepycat.persist.sequences</pre>
 *
 * <p>With default database naming, databases with the following names will be
 * present each store.</p>
 *
 * <pre>   STORE_NAME-com.sleepycat.persist.formats</pre>
 * <pre>   STORE_NAME-com.sleepycat.persist.sequences</pre>
 *
 * <p>These databases must normally be included with copies of other databases
 * in the store.  They should not be modified by the application.</p>
 */
public interface DatabaseNamer {

    /**
     * Returns the name of the file to be used to store the dataabase for the
     * given store, entity class and key.  This method may not return null.
     *
     * @param storeName the name of the {@link EntityStore}.
     *
     * @param entityClassName the complete name of the entity class for a
     * primary or secondary index.
     *
     * @param keyName the key name identifying a secondary index, or null for
     * a primary index.
     */
    public String getFileName(String storeName,
                              String entityClassName,
                              String keyName);

    /**
     * The default database namer.
     *
     * <p>The {@link #getFileName getFileName} method of this namer returns the
     * {@code storeName}, {@code entityClassName} and {@code keyName}
     * parameters as follows:<p>
     *
     * <pre class="code">
     * if (keyName != null) {
     *     return storeName + '-' + entityClassName + '-' + keyName;
     * } else {
     *     return storeName + '-' + entityClassName;
     * }</pre>
     */
    public static final DatabaseNamer DEFAULT = new DatabaseNamer() {

        public String getFileName(String storeName,
                                  String entityClassName,
                                  String keyName) {
            if (keyName != null) {
                return storeName + '-' + entityClassName + '-' + keyName;
            } else {
                return storeName + '-' + entityClassName;
            }
        }
    };
}
