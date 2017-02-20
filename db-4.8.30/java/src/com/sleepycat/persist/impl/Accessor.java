/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

/**
 * Field binding operations implemented via reflection (ReflectionAccessor) or
 * bytecode enhancement (EnhancedAccessor).
 *
 * <p>Normally we read the set of all secondary key fields first and then the
 * set of all non-key fields, reading each set in order of field name.  But
 * when reading an old format record we must account for the following
 * class evolution conversions:</p>
 * <ul>
 * <li>Convert a field: pass value thru converter</li>
 * <li>Widen a field type: pass value thru widener</li>
 * <li>Add a field: don't read the new field</li>
 * <li>Delete a field: skip the deleted field</li>
 * <li>Rename a field: read field in a different order</li>
 * </ul>
 * <p>To support these operations, the methods for reading fields allow reading
 * specific ranges of fields as well as all fields.  For example, all fields
 * up to a deleted field could be read, and then all fields from the following
 * field onward.</p>
 *
 * @author Mark Hayes
 */
interface Accessor {

    /**
     * A large field value to use instead of Integer.MAX_VALUE, to work around
     * Java JIT compiler bug when doing an (X <= Integer.MAX_VALUE) as would be
     * done in readXxxKeyFields methods.
     */
    final int MAX_FIELD_NUM = Integer.MAX_VALUE - 1;

    /**
     * Creates a new instance of the target class using its default
     * constructor.
     */
    Object newInstance();

    /**
     * Creates a new one dimensional array of the given length, having the
     * target class as its component type.
     *
     * <p>Using a special method for a one dimensional array, which can be
     * implemented by bytecode generation, is a compromise.  We use reflection
     * to create multidimensional arrays.  We could in the future generate code
     * to create arrays as they are encountered, if there is a need to avoid
     * reflection for multidimensional arrays.</p>
     */
    Object newArray(int len);

    /**
     * Returns whether the primary key field is null (for a reference type) or
     * zero (for a primitive integer type).  Null and zero are used as an
     * indication that the key should be assigned from a sequence.
     */
    boolean isPriKeyFieldNullOrZero(Object o);

    /**
     * Writes the primary key field value to the given EntityOutput.
     *
     * <p>To write a primary key with a reference type, this method must call
     * EntityOutput.writeKeyObject.</p>
     *
     * @param o is the object whose primary key field is to be written.
     *
     * @param output the output data to write to.
     */
    void writePriKeyField(Object o, EntityOutput output);

    /**
     * Reads the primary key field value from the given EntityInput.
     *
     * <p>To read a primary key with a reference type, this method must call
     * EntityInput.readKeyObject.</p>
     *
     * @param o is the object whose primary key field is to be read.
     *
     * @param input the input data to read from.
     */
    void readPriKeyField(Object o, EntityInput input);

    /**
     * Writes all secondary key field values to the given EntityOutput,
     * writing fields in super classes first and in name order within class.
     *
     * @param o is the object whose secondary key fields are to be written.
     *
     * <p>If the primary key has a reference type, this method must call
     * EntityOutput.registerPriKeyObject before writing any other fields.</p>
     *
     * @param output the output data to write to.
     */
    void writeSecKeyFields(Object o, EntityOutput output);

    /**
     * Reads a range of secondary key field values from the given EntityInput,
     * reading fields in super classes first and in name order within class.
     *
     * <p>If the primary key has a reference type, this method must call
     * EntityInput.registerPriKeyObject before reading any other fields.</p>
     *
     * <p>To read all fields, pass -1 for superLevel, zero for startField and
     * MAX_FIELD_NUM for endField.  Fields from super classes are read
     * first.</p>
     *
     * <p>To read a specific range of fields, pass a non-negative number for
     * superLevel and the specific indices of the field range to be read in the
     * class at that level.</p>
     *
     * @param o is the object whose secondary key fields are to be read.
     *
     * @param input the input data to read from.
     *
     * @param startField the starting field index in the range of fields to
     * read.  To read all fields, the startField should be zero.
     *
     * @param endField the ending field index in the range of fields to read.
     * To read all fields, the endField should be MAX_FIELD_NUM.
     *
     * @param superLevel is a non-negative number to read the fields of the
     * class that is the Nth super instance; or a negative number to read
     * fields in all classes.
     */
    void readSecKeyFields(Object o,
                          EntityInput input,
                          int startField,
                          int endField,
                          int superLevel);

    /**
     * Writes all non-key field values to the given EntityOutput, writing
     * fields in super classes first and in name order within class.
     *
     * @param o is the object whose non-key fields are to be written.
     *
     * @param output the output data to write to.
     */
    void writeNonKeyFields(Object o, EntityOutput output);

    /**
     * Reads a range of non-key field values from the given EntityInput,
     * reading fields in super classes first and in name order within class.
     *
     * <p>To read all fields, pass -1 for superLevel, zero for startField and
     * MAX_FIELD_NUM for endField.  Fields from super classes are read
     * first.</p>
     *
     * <p>To read a specific range of fields, pass a non-negative number for
     * superLevel and the specific indices of the field range to be read in the
     * class at that level.</p>
     *
     * @param o is the object whose non-key fields are to be read.
     *
     * @param input the input data to read from.
     *
     * @param startField the starting field index in the range of fields to
     * read.  To read all fields, the startField should be zero.
     *
     * @param endField the ending field index in the range of fields to read.
     * To read all fields, the endField should be MAX_FIELD_NUM.
     *
     * @param superLevel is a non-negative number to read the fields of the
     * class that is the Nth super instance; or a negative number to read
     * fields in all classes.
     */
    void readNonKeyFields(Object o,
                          EntityInput input,
                          int startField,
                          int endField,
                          int superLevel);

    /**
     * Writes all composite key field values to the given EntityOutput, writing
     * in declared field number order.
     *
     * @param o the composite key object whose fields are to be written.
     *
     * @param output the output data to write to.
     */
    void writeCompositeKeyFields(Object o, EntityOutput output);

    /**
     * Reads all composite key field values from the given EntityInput,
     * reading in declared field number order.
     *
     * @param o the composite key object whose fields are to be read.
     *
     * @param input the input data to read from.
     */
    void readCompositeKeyFields(Object o, EntityInput input);

    /**
     * Returns the value of a given field, representing primitives as primitive
     * wrapper objects.
     *
     * @param o is the object containing the key field.
     *
     * @param field is the field index.
     *
     * @param superLevel is a positive number to identify the field of the
     * class that is the Nth super instance; or zero to identify the field in
     * this class.
     *
     * @param isSecField is true for a secondary key field or false for a
     * non-key field.
     *
     * @return the current field value, or null for a reference type field
     * that is null.
     */
    Object getField(Object o,
                    int field,
                    int superLevel,
                    boolean isSecField);

    /**
     * Changes the value of a given field, representing primitives as primitive
     * wrapper objects.
     *
     * @param o is the object containing the key field.
     *
     * @param field is the field index.
     *
     * @param superLevel is a positive number to identify the field of the
     * class that is the Nth super instance; or zero to identify the field in
     * this class.
     *
     * @param isSecField is true for a secondary key field or false for a
     * non-key field.
     *
     * @param value is the new value of the field, or null to set a reference
     * type field to null.
     */
    void setField(Object o,
                  int field,
                  int superLevel,
                  boolean isSecField,
                  Object value);
}
