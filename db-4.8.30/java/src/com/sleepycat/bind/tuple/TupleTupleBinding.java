/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.db.DatabaseEntry;

/**
 * An abstract <code>EntityBinding</code> that treats an entity's key entry and
 * data entry as tuples.
 *
 * <p>This class takes care of converting the entries to/from {@link
 * TupleInput} and {@link TupleOutput} objects.  Its three abstract methods
 * must be implemented by a concrete subclass to convert between tuples and
 * entity objects.</p>
 * <ul>
 * <li> {@link #entryToObject(TupleInput,TupleInput)} </li>
 * <li> {@link #objectToKey(Object,TupleOutput)} </li>
 * <li> {@link #objectToData(Object,TupleOutput)} </li>
 * </ul>
 *
 * @author Mark Hayes
 */
public abstract class TupleTupleBinding<E> extends TupleBase<E>
    implements EntityBinding<E> {

    /**
     * Creates a tuple-tuple entity binding.
     */
    public TupleTupleBinding() {
    }

    // javadoc is inherited
    public E entryToObject(DatabaseEntry key, DatabaseEntry data) {

        return entryToObject(TupleBinding.entryToInput(key),
                             TupleBinding.entryToInput(data));
    }

    // javadoc is inherited
    public void objectToKey(E object, DatabaseEntry key) {

        TupleOutput output = getTupleOutput(object);
        objectToKey(object, output);
        outputToEntry(output, key);
    }

    // javadoc is inherited
    public void objectToData(E object, DatabaseEntry data) {

        TupleOutput output = getTupleOutput(object);
        objectToData(object, output);
        outputToEntry(output, data);
    }

    // abstract methods

    /**
     * Constructs an entity object from {@link TupleInput} key and data
     * entries.
     *
     * @param keyInput is the {@link TupleInput} key entry object.
     *
     * @param dataInput is the {@link TupleInput} data entry object.
     *
     * @return the entity object constructed from the key and data.
     */
    public abstract E entryToObject(TupleInput keyInput, TupleInput dataInput);

    /**
     * Extracts a key tuple from an entity object.
     *
     * @param object is the entity object.
     *
     * @param output is the {@link TupleOutput} to which the key should be
     * written.
     */
    public abstract void objectToKey(E object, TupleOutput output);

    /**
     * Extracts a key tuple from an entity object.
     *
     * @param object is the entity object.
     *
     * @param output is the {@link TupleOutput} to which the data should be
     * written.
     */
    public abstract void objectToData(E object, TupleOutput output);
}
