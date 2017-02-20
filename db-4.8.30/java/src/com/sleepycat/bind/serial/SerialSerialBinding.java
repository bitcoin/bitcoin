/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.serial;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.db.DatabaseEntry;

/**
 * An abstract <code>EntityBinding</code> that treats an entity's key entry and
 * data entry as serialized objects.
 *
 * <p>This class takes care of serializing and deserializing the key and
 * data entry automatically.  Its three abstract methods must be implemented by
 * a concrete subclass to convert the deserialized objects to/from an entity
 * object.</p>
 * <ul>
 * <li> {@link #entryToObject(Object,Object)} </li>
 * <li> {@link #objectToKey(Object)} </li>
 * <li> {@link #objectToData(Object)} </li>
 * </ul>
 *
 * @see <a href="SerialBinding.html#evolution">Class Evolution</a>
 *
 * @author Mark Hayes
 */
public abstract class SerialSerialBinding<K,D,E> implements EntityBinding<E> {

    private SerialBinding<K> keyBinding;
    private SerialBinding<D> dataBinding;

    /**
     * Creates a serial-serial entity binding.
     *
     * @param classCatalog is the catalog to hold shared class information and
     * for a database should be a {@link StoredClassCatalog}.
     *
     * @param keyClass is the key base class.
     *
     * @param dataClass is the data base class.
     */
    public SerialSerialBinding(ClassCatalog classCatalog,
                               Class<K> keyClass,
                               Class<D> dataClass) {

        this(new SerialBinding<K>(classCatalog, keyClass),
             new SerialBinding<D>(classCatalog, dataClass));
    }

    /**
     * Creates a serial-serial entity binding.
     *
     * @param keyBinding is the key binding.
     *
     * @param dataBinding is the data binding.
     */
    public SerialSerialBinding(SerialBinding<K> keyBinding,
                               SerialBinding<D> dataBinding) {

        this.keyBinding = keyBinding;
        this.dataBinding = dataBinding;
    }

    // javadoc is inherited
    public E entryToObject(DatabaseEntry key, DatabaseEntry data) {

        return entryToObject(keyBinding.entryToObject(key),
                             dataBinding.entryToObject(data));
    }

    // javadoc is inherited
    public void objectToKey(E object, DatabaseEntry key) {

        K keyObject = objectToKey(object);
        keyBinding.objectToEntry(keyObject, key);
    }

    // javadoc is inherited
    public void objectToData(E object, DatabaseEntry data) {

        D dataObject = objectToData(object);
        dataBinding.objectToEntry(dataObject, data);
    }

    /**
     * Constructs an entity object from deserialized key and data objects.
     *
     * @param keyInput is the deserialized key object.
     *
     * @param dataInput is the deserialized data object.
     *
     * @return the entity object constructed from the key and data.
     */
    public abstract E entryToObject(K keyInput, D dataInput);

    /**
     * Extracts a key object from an entity object.
     *
     * @param object is the entity object.
     *
     * @return the deserialized key object.
     */
    public abstract K objectToKey(E object);

    /**
     * Extracts a data object from an entity object.
     *
     * @param object is the entity object.
     *
     * @return the deserialized data object.
     */
    public abstract D objectToData(E object);
}
