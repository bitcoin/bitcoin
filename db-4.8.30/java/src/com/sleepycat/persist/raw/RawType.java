/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.raw;

import java.util.List;
import java.util.Map;

import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.Entity;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.Persistent;

/**
 * The type definition for a simple or complex persistent type, or an array
 * of persistent types.
 *
 * <p>{@code RawType} objects are thread-safe.  Multiple threads may safely
 * call the methods of a shared {@code RawType} object.</p>
 *
 * @author Mark Hayes
 */
public interface RawType {

    /**
     * Returns the class name for this type in the format specified by {@link
     * Class#getName}.
     *
     * <p>If this class currently exists (has not been removed or renamed) then
     * the class name may be passed to {@link Class#forName} to get the current
     * {@link Class} object.  However, if this raw type is not the current
     * version of the class, this type information may differ from that of the
     * current {@link Class}.</p>
     */
    String getClassName();

    /**
     * Returns the class version for this type.  For simple types, zero is
     * always returned.
     *
     * @see Entity#version
     * @see Persistent#version
     */
    int getVersion();

    /**
     * Returns the internal unique ID for this type.
     */
    int getId();

    /**
     * Returns whether this is a {@link <a
     * href="../model/Entity.html#simpleTypes">simple type</a>}: primitive,
     * primitive wrapper, BigInteger, String or Date.
     * <!--
     * primitive wrapper, BigInteger, BigDecimal, String or Date.
     * -->
     *
     * <p>If true is returned, {@link #isPrimitive} can be called for more
     * information, and a raw value of this type is represented as a simple
     * type object (not as a {@link RawObject}).</p>
     *
     * <p>If false is returned, this is a complex type, an array type (see
     * {@link #isArray}), or an enum type, and a raw value of this type is
     * represented as a {@link RawObject}.</p>
     */
    boolean isSimple();

    /**
     * Returns whether this type is a Java primitive: char, byte, short, int,
     * long, float or double.
     *
     * <p>If true is returned, this is also a simple type.  In other words,
     * primitive types are a subset of simple types.</p>
     *
     * <p>If true is returned, a raw value of this type is represented as a
     * non-null instance of the primitive type's wrapper class.  For example,
     * an <code>int</code> raw value is represented as an
     * <code>Integer</code>.</p>
     */
    boolean isPrimitive();

    /**
     * Returns whether this is an enum type.
     *
     * <p>If true is returned, a value of this type is a {@link RawObject} and
     * the enum constant String is available via {@link RawObject#getEnum}.</p>
     *
     * <p>If false is returned, then this is a complex type, an array type (see
     * {@link #isArray}), or a simple type (see {@link #isSimple}).</p>
     */
    boolean isEnum();

    /**
     * Returns an unmodifiable list of the names of the enum instances, or null
     * if this is not an enum type.
     */
    List<String> getEnumConstants();

    /**
     * Returns whether this is an array type.  Raw value arrays are represented
     * as {@link RawObject} instances.
     *
     * <p>If true is returned, the array component type is returned by {@link
     * #getComponentType} and the number of array dimensions is returned by
     * {@link #getDimensions}.</p>
     *
     * <p>If false is returned, then this is a complex type, an enum type (see
     * {@link #isEnum}), or a simple type (see {@link #isSimple}).</p>
     */
    boolean isArray();

    /**
     * Returns the number of array dimensions, or zero if this is not an array
     * type.
     */
    int getDimensions();

    /**
     * Returns the array component type, or null if this is not an array type.
     */
    RawType getComponentType();

    /**
     * Returns a map of field name to raw field for each non-static
     * non-transient field declared in this class, or null if this is not a
     * complex type (in other words, this is a simple type or an array type).
     */
    Map<String,RawField> getFields();

    /**
     * Returns the type of the superclass, or null if the superclass is Object
     * or this is not a complex type (in other words, this is a simple type or
     * an array type).
     */
    RawType getSuperType();

    /**
     * Returns the original model class metadata used to create this class, or
     * null if this is not a model class.
     */
    ClassMetadata getClassMetadata();

    /**
     * Returns the original model entity metadata used to create this class, or
     * null if this is not an entity class.
     */
    EntityMetadata getEntityMetadata();

    /**
     * Returns an XML representation of the raw type.
     */
    String toString();
}
