/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.math.BigInteger;

import com.sleepycat.bind.tuple.TupleOutput;

/**
 * Used for writing object fields.
 *
 * <p>Unlike TupleOutput, Strings should be passed to {@link #writeObject} when
 * using this class.</p>
 *
 * <p>Note that currently there is only one implementation of EntityOutput:
 * RecordOutput.  There is no RawObjectOutput implemention because we currently
 * have no need to convert from persistent objects to RawObject instances.
 * The EntityOutput interface is only for symmetry with EntityInput and in case
 * we need RawObjectOutput in the future.</p>
 *
 * @author Mark Hayes
 */
public interface EntityOutput {

    /**
     * Called via Accessor to write all fields with reference types, except for
     * the primary key field and composite key fields (see writeKeyObject
     * below).
     */
    void writeObject(Object o, Format fieldFormat);

    /**
     * Called for a primary key field or composite key field with a reference
     * type.
     */
    void writeKeyObject(Object o, Format fieldFormat);

    /**
     * Called via Accessor.writeSecKeyFields for a primary key field with a
     * reference type.  This method must be called before writing any other
     * fields.
     */
    void registerPriKeyObject(Object o);

    /**
     * Called by ObjectArrayFormat and PrimitiveArrayFormat to write the array
     * length.
     */
    void writeArrayLength(int length);

    /**
     * Called by EnumFormat to write the given index of the enum constant.
     */
    void writeEnumConstant(String[] names, int index);

    /* The following methods are a subset of the methods in TupleOutput. */

    TupleOutput writeString(String val);
    TupleOutput writeChar(int val);
    TupleOutput writeBoolean(boolean val);
    TupleOutput writeByte(int val);
    TupleOutput writeShort(int val);
    TupleOutput writeInt(int val);
    TupleOutput writeLong(long val);
    TupleOutput writeSortedFloat(float val);
    TupleOutput writeSortedDouble(double val);
    TupleOutput writeBigInteger(BigInteger val);
}
