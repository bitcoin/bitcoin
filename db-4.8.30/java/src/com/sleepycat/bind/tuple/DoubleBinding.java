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
 * A concrete <code>TupleBinding</code> for a <code>Double</code> primitive
 * wrapper or a <code>double</code> primitive.
 *
 * <p><em>Note:</em> This class produces byte array values that by default
 * (without a custom comparator) do <em>not</em> sort correctly for negative
 * values.  Only non-negative values are sorted correctly by default.  To sort
 * all values correctly by default, use {@link SortedDoubleBinding}.</p>
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
public class DoubleBinding extends TupleBinding<Double> {

    private static final int DOUBLE_SIZE = 8;

    // javadoc is inherited
    public Double entryToObject(TupleInput input) {

        return input.readDouble();
    }

    // javadoc is inherited
    public void objectToEntry(Double object, TupleOutput output) {

        output.writeDouble(object);
    }

    // javadoc is inherited
    protected TupleOutput getTupleOutput(Double object) {

        return sizedOutput();
    }

    /**
     * Converts an entry buffer into a simple <code>double</code> value.
     *
     * @param entry is the source entry buffer.
     *
     * @return the resulting value.
     */
    public static double entryToDouble(DatabaseEntry entry) {

        return entryToInput(entry).readDouble();
    }

    /**
     * Converts a simple <code>double</code> value into an entry buffer.
     *
     * @param val is the source value.
     *
     * @param entry is the destination entry buffer.
     */
    public static void doubleToEntry(double val, DatabaseEntry entry) {

        outputToEntry(sizedOutput().writeDouble(val), entry);
    }

    /**
     * Returns a tuple output object of the exact size needed, to avoid
     * wasting space when a single primitive is output.
     */
    static TupleOutput sizedOutput() {

        return new TupleOutput(new byte[DOUBLE_SIZE]);
    }
}
