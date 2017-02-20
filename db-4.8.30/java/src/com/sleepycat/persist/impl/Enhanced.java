/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

/**
 * Interface implemented by a persistent class via bytecode enhancement.
 *
 * <p>See {@link Accessor} for method documentation.  {@link EnhancedAccessor}
 * implements Accessor and forwards all calls to methods in the Enhanced
 * class.</p>
 *
 * <p>Each class that implements this interface (including its subclasses and
 * superclasses except for Object) must also implement a static block that
 * registers a prototype instance by calling
 * EnhancedAccessor.registerPrototype.  Other instances are created from the
 * protype instance using {@link #bdbNewInstance}.</p>
 *
 * <pre>static { EnhancedAccessor.registerPrototype(new Xxx()); }</pre>
 *
 * <p>An example of the generated code for reading and writing fields is shown
 * below.</p>
 *
 * <pre>
 *  private int f1;
 *  private String f2;
 *  private MyClass f3;
 *
 *  public void bdbWriteNonKeyFields(EntityOutput output) {
 *
 *      super.bdbWriteNonKeyFields(output);
 *
 *      output.writeInt(f1);
 *      output.writeObject(f2, null);
 *      output.writeObject(f3, null);
 *  }
 *
 *  public void bdbReadNonKeyFields(EntityInput input,
 *                                  int startField,
 *                                  int endField,
 *                                  int superLevel) {
 *
 *      if (superLevel != 0) {
 *          super.bdbReadNonKeyFields(input, startField, endField,
 *                                    superLevel - 1);
 *      }
 *      if (superLevel &lt;= 0) {
 *          switch (startField) {
 *          case 0:
 *              f1 = input.readInt();
 *              if (endField == 0) break;
 *          case 1:
 *              f2 = (String) input.readObject();
 *              if (endField == 1) break;
 *          case 2:
 *              f3 = (MyClass) input.readObject();
 *          }
 *      }
 *  }
 * </pre>
 *
 * @author Mark Hayes
 */
public interface Enhanced {

    /**
     * @see Accessor#newInstance
     */
    Object bdbNewInstance();

    /**
     * @see Accessor#newArray
     */
    Object bdbNewArray(int len);

    /**
     * Calls the super class method if this class does not contain the primary
     * key field.
     *
     * @see Accessor#isPriKeyFieldNullOrZero
     */
    boolean bdbIsPriKeyFieldNullOrZero();

    /**
     * Calls the super class method if this class does not contain the primary
     * key field.
     *
     * @see Accessor#writePriKeyField
     */
    void bdbWritePriKeyField(EntityOutput output, Format format);

    /**
     * Calls the super class method if this class does not contain the primary
     * key field.
     *
     * @see Accessor#readPriKeyField
     */
    void bdbReadPriKeyField(EntityInput input, Format format);

    /**
     * @see Accessor#writeSecKeyFields
     */
    void bdbWriteSecKeyFields(EntityOutput output);

    /**
     * @see Accessor#readSecKeyFields
     */
    void bdbReadSecKeyFields(EntityInput input,
                             int startField,
                             int endField,
                             int superLevel);

    /**
     * @see Accessor#writeNonKeyFields
     */
    void bdbWriteNonKeyFields(EntityOutput output);

    /**
     * @see Accessor#readNonKeyFields
     */
    void bdbReadNonKeyFields(EntityInput input,
                             int startField,
                             int endField,
                             int superLevel);

    /**
     * @see Accessor#writeCompositeKeyFields
     */
    void bdbWriteCompositeKeyFields(EntityOutput output, Format[] formats);

    /**
     * @see Accessor#readCompositeKeyFields
     */
    void bdbReadCompositeKeyFields(EntityInput input, Format[] formats);

    /**
     * @see Accessor#getField
     */
    Object bdbGetField(Object o,
                       int field,
                       int superLevel,
                       boolean isSecField);

    /**
     * @see Accessor#setField
     */
    void bdbSetField(Object o,
                     int field,
                     int superLevel,
                     boolean isSecField,
                     Object value);
}
