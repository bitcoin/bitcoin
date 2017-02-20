/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.bind.tuple;

import java.math.BigInteger;

import com.sleepycat.db.DatabaseEntry;

/**
 * A concrete <code>TupleBinding</code> for a <code>BigInteger</code> value.
 */
public class BigIntegerBinding extends TupleBinding<BigInteger> {

    // javadoc is inherited
    public BigInteger entryToObject(TupleInput input) {

        return input.readBigInteger();
    }

    // javadoc is inherited
    public void objectToEntry(BigInteger object, TupleOutput output) {

        output.writeBigInteger(object);
    }

    // javadoc is inherited
    protected TupleOutput getTupleOutput(BigInteger object) {

        return sizedOutput(object);
    }

    /**
     * Converts an entry buffer into a <code>BigInteger</code> value.
     *
     * @param entry is the source entry buffer.
     *
     * @return the resulting value.
     */
    public static BigInteger entryToBigInteger(DatabaseEntry entry) {

        return entryToInput(entry).readBigInteger();
    }

    /**
     * Converts a <code>BigInteger</code> value into an entry buffer.
     *
     * @param val is the source value.
     *
     * @param entry is the destination entry buffer.
     */
    public static void bigIntegerToEntry(BigInteger val, DatabaseEntry entry) {

        outputToEntry(sizedOutput(val).writeBigInteger(val), entry);
    }

    /**
     * Returns a tuple output object of the exact size needed, to avoid
     * wasting space when a single primitive is output.
     */
    private static TupleOutput sizedOutput(BigInteger val) {

        int len = TupleOutput.getBigIntegerByteLength(val);
        return new TupleOutput(new byte[len]);
    }
}
