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
 * A concrete <code>TupleBinding</code> for a <code>Long</code> primitive
 * wrapper or a <code>long</code> primitive.
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
public class LongBinding extends TupleBinding<Long> {

    private static final int LONG_SIZE = 8;

    // javadoc is inherited
    public Long entryToObject(TupleInput input) {

        return input.readLong();
    }

    // javadoc is inherited
    public void objectToEntry(Long object, TupleOutput output) {

        output.writeLong(object);
    }

    // javadoc is inherited
    protected TupleOutput getTupleOutput(Long object) {

        return sizedOutput();
    }

    /**
     * Converts an entry buffer into a simple <code>long</code> value.
     *
     * @param entry is the source entry buffer.
     *
     * @return the resulting value.
     */
    public static long entryToLong(DatabaseEntry entry) {

        return entryToInput(entry).readLong();
    }

    /**
     * Converts a simple <code>long</code> value into an entry buffer.
     *
     * @param val is the source value.
     *
     * @param entry is the destination entry buffer.
     */
    public static void longToEntry(long val, DatabaseEntry entry) {

        outputToEntry(sizedOutput().writeLong(val), entry);
    }

    /**
     * Returns a tuple output object of the exact size needed, to avoid
     * wasting space when a single primitive is output.
     */
    private static TupleOutput sizedOutput() {

        return new TupleOutput(new byte[LONG_SIZE]);
    }
}
