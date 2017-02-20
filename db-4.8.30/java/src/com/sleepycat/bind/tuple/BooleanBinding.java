/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple;

import com.sleepycat.db.DatabaseEntry;

/**
 * A concrete <code>TupleBinding</code> for a <code>Boolean</code> primitive
 * wrapper or a <code>boolean</code> primitive.
 *
 * <p>There are two ways to use this class:</p>
 * <ol>
 * <li>When using the {@link com.sleepycat.db} package directly, the static
 * methods in this class can be used to convert between primitive values and
 * {@link DatabaseEntry} objects.</li>
 * <li>When using the {@link com.sleepycat.collections} package, an instance of
 * this class can be used with any stored collection.  The easiest way to
 * obtain a binding instance is with the {@link
 * TupleBinding#getPrimitiveBinding} method.</li>
 * </ol>
 */
public class BooleanBinding extends TupleBinding<Boolean> {

    private static final int BOOLEAN_SIZE = 1;

    // javadoc is inherited
    public Boolean entryToObject(TupleInput input) {

	return input.readBoolean();
    }

    // javadoc is inherited
    public void objectToEntry(Boolean object, TupleOutput output) {

        output.writeBoolean(object);
    }

    // javadoc is inherited
    protected TupleOutput getTupleOutput(Boolean object) {

        return sizedOutput();
    }

    /**
     * Converts an entry buffer into a simple <code>boolean</code> value.
     *
     * @param entry is the source entry buffer.
     *
     * @return the resulting value.
     */
    public static boolean entryToBoolean(DatabaseEntry entry) {

        return entryToInput(entry).readBoolean();
    }

    /**
     * Converts a simple <code>boolean</code> value into an entry buffer.
     *
     * @param val is the source value.
     *
     * @param entry is the destination entry buffer.
     */
    public static void booleanToEntry(boolean val, DatabaseEntry entry) {

        outputToEntry(sizedOutput().writeBoolean(val), entry);
    }

    /**
     * Returns a tuple output object of the exact size needed, to avoid
     * wasting space when a single primitive is output.
     */
    private static TupleOutput sizedOutput() {

        return new TupleOutput(new byte[BOOLEAN_SIZE]);
    }
}
