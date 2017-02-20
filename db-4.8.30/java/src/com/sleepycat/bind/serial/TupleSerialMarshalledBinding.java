/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.serial;

import com.sleepycat.bind.tuple.MarshalledTupleKeyEntity;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;

/**
 * A concrete <code>TupleSerialBinding</code> that delegates to the
 * <code>MarshalledTupleKeyEntity</code> interface of the entity class.
 *
 * <p>The {@link MarshalledTupleKeyEntity} interface must be implemented by the
 * entity class to convert between the key/data entry and entity object.</p>
 *
 * <p> The binding is "tricky" in that it uses the entity class for both the
 * stored data entry and the combined entity object.  To do this, the entity's
 * key field(s) are transient and are set by the binding after the data object
 * has been deserialized. This avoids the use of a "data" class completely.
 * </p>
 *
 * @see MarshalledTupleKeyEntity
 * @see <a href="SerialBinding.html#evolution">Class Evolution</a>
 *
 * @author Mark Hayes
 */
public class TupleSerialMarshalledBinding<E extends MarshalledTupleKeyEntity>
    extends TupleSerialBinding<E,E> {

    /**
     * Creates a tuple-serial marshalled binding object.
     *
     * @param classCatalog is the catalog to hold shared class information and
     * for a database should be a {@link StoredClassCatalog}.
     *
     * @param baseClass is the base class for serialized objects stored using
     * this binding -- all objects using this binding must be an instance of
     * this class.
     */
    public TupleSerialMarshalledBinding(ClassCatalog classCatalog,
                                        Class<E> baseClass) {

        this(new SerialBinding<E>(classCatalog, baseClass));
    }

    /**
     * Creates a tuple-serial marshalled binding object.
     *
     * @param dataBinding is the binding used for serializing and deserializing
     * the entity object.
     */
    public TupleSerialMarshalledBinding(SerialBinding<E> dataBinding) {

        super(dataBinding);
    }

    // javadoc is inherited
    public E entryToObject(TupleInput tupleInput, E javaInput) {

        /*
         * Creates the entity by combining the stored key and data.
         * This "tricky" binding returns the stored data as the entity, but
         * first it sets the transient key fields from the stored key.
         */
        if (tupleInput != null) { // may be null if not used by key extractor
            javaInput.unmarshalPrimaryKey(tupleInput);
        }
        return javaInput;
    }

    // javadoc is inherited
    public void objectToKey(E object, TupleOutput output) {

        /* Creates the stored key from the entity. */
        object.marshalPrimaryKey(output);
    }

    // javadoc is inherited
    public E objectToData(E object) {

        /*
         * Returns the entity as the stored data.  There is nothing to do here
         * since the entity's key fields are transient.
         */
        return object;
    }
}
