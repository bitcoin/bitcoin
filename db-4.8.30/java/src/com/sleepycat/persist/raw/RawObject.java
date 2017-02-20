/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.raw;

import java.util.Arrays;
import java.util.Map;
import java.util.TreeSet;

import com.sleepycat.persist.evolve.Conversion;
import com.sleepycat.persist.model.EntityModel;

/**
 * A raw instance that can be used with a {@link RawStore} or {@link
 * Conversion}.  A <code>RawObject</code> is used to represent instances of
 * complex types (persistent classes with fields), arrays, and enum values.  It
 * is not used to represent non-enum simple types, which are represented as
 * simple objects.  This includes primitives, which are represented as
 * instances of their wrapper class.
 *
 * <p>{@code RawObject} objects are thread-safe.  Multiple threads may safely
 * call the methods of a shared {@code RawObject} object.</p>
 *
 * @author Mark Hayes
 */
public class RawObject {

    private static final String INDENT = "  ";

    private RawType type;
    private Map<String,Object> values;
    private Object[] elements;
    private String enumConstant;
    private RawObject superObject;

    /**
     * Creates a raw object with a given set of field values for a complex
     * type.
     *
     * @param type the type of this raw object.
     *
     * @param values a map of field name to value for each declared field in
     * the class, or null to create an empty map.  Each value in the map is a
     * {@link RawObject}, a {@link <a
     * href="../model/Entity.html#simpleTypes">simple type</a>} instance, or
     * null.
     *
     * @param superObject the instance of the superclass, or null if the
     * superclass is {@code Object}.
     *
     * @throws IllegalArgumentException if the type argument is an array type.
     */
    public RawObject(RawType type,
                     Map<String,Object> values,
                     RawObject superObject) {
        if (type == null || values == null) {
            throw new NullPointerException();
        }
        this.type = type;
        this.values = values;
        this.superObject = superObject;
    }

    /**
     * Creates a raw object with the given array elements for an array type.
     *
     * @param type the type of this raw object.
     *
     * @param elements an array of elements.  Each element in the array is a
     * {@link RawObject}, a {@link <a
     * href="../model/Entity.html#simpleTypes">simple type</a>} instance, or
     * null.
     *
     * @throws IllegalArgumentException if the type argument is not an array
     * type.
     */
    public RawObject(RawType type, Object[] elements) {
        if (type == null || elements == null) {
            throw new NullPointerException();
        }
        this.type = type;
        this.elements = elements;
    }

    /**
     * Creates a raw object with the given enum value for an enum type.
     *
     * @param type the type of this raw object.
     *
     * @param enumConstant the String value of this enum constant; must be
     * one of the Strings returned by {@link RawType#getEnumConstants}.
     *
     * @throws IllegalArgumentException if the type argument is not an array
     * type.
     */
    public RawObject(RawType type, String enumConstant) {
        if (type == null || enumConstant == null) {
            throw new NullPointerException();
        }
        this.type = type;
        this.enumConstant = enumConstant;
    }

    /**
     * Returns the raw type information for this raw object.
     *
     * <p>Note that if this object is unevolved, the returned type may be
     * different from the current type returned by {@link
     * EntityModel#getRawType EntityModel.getRawType} for the same class name.
     * This can only occur in a {@link Conversion#convert
     * Conversion.convert}.</p>
     */
    public RawType getType() {
        return type;
    }

    /**
     * Returns a map of field name to value for a complex type, or null for an
     * array type or an enum type.  The map contains a String key for each
     * declared field in the class.  Each value in the map is a {@link
     * RawObject}, a {@link <a href="../model/Entity.html#simpleTypes">simple
     * type</a>} instance, or null.
     *
     * <p>There will be an entry in the map for every field declared in this
     * type, as determined by {@link RawType#getFields} for the type returned
     * by {@link #getType}.  Values in the map may be null for fields with
     * non-primitive types.</p>
     */
    public Map<String,Object> getValues() {
        return values;
    }

    /**
     * Returns the array of elements for an array type, or null for a complex
     * type or an enum type.  Each element in the array is a {@link RawObject},
     * a {@link <a href="../model/Entity.html#simpleTypes">simple type</a>}
     * instance, or null.
     */
    public Object[] getElements() {
        return elements;
    }

    /**
     * Returns the enum constant String for an enum type, or null for a complex
     * type or an array type.  The String returned will be one of the Strings
     * returned by {@link RawType#getEnumConstants}.
     */
    public String getEnum() {
        return enumConstant;
    }

    /**
     * Returns the instance of the superclass, or null if the superclass is
     * {@code Object} or {@code Enum}.
     */
    public RawObject getSuper() {
        return superObject;
    }

    @Override
    public boolean equals(Object other) {
        if (other == this) {
            return true;
        }
        if (!(other instanceof RawObject)) {
            return false;
        }
        RawObject o = (RawObject) other;
        if (type != o.type) {
            return false;
        }
        if (!Arrays.deepEquals(elements, o.elements)) {
            return false;
        }
        if (enumConstant != null) {
            if (!enumConstant.equals(o.enumConstant)) {
                return false;
            }
        } else {
            if (o.enumConstant != null) {
                return false;
            }
        }
        if (values != null) {
            if (!values.equals(o.values)) {
                return false;
            }
        } else {
            if (o.values != null) {
                return false;
            }
        }
        if (superObject != null) {
            if (!superObject.equals(o.superObject)) {
                return false;
            }
        } else {
            if (o.superObject != null) {
                return false;
            }
        }
        return true;
    }

    @Override
    public int hashCode() {
        return System.identityHashCode(type) +
               Arrays.deepHashCode(elements) +
               (enumConstant != null ? enumConstant.hashCode() : 0) +
               (values != null ? values.hashCode() : 0) +
               (superObject != null ? superObject.hashCode() : 0);
    }

    /**
     * Returns an XML representation of the raw object.
     */
    @Override
    public String toString() {
        StringBuffer buf = new StringBuffer(500);
        formatRawObject(buf, "", null, false);
        return buf.toString();
    }

    private void formatRawObject(StringBuffer buf,
                                 String indent,
                                 String id,
                                 boolean isSuper) {
        if (type.isEnum()) {
            buf.append(indent);
            buf.append("<Enum");
            formatId(buf, id);
            buf.append(" class=\"");
            buf.append(type.getClassName());
            buf.append("\" typeId=\"");
            buf.append(type.getId());
            buf.append("\">");
            buf.append(enumConstant);
            buf.append("</Enum>\n");
        } else {
            String indent2 = indent + INDENT;
            String endTag;
            buf.append(indent);
            if (type.isArray()) {
                buf.append("<Array");
                endTag = "</Array>";
            } else if (isSuper) {
                buf.append("<Super");
                endTag = "</Super>";
            } else {
                buf.append("<Object");
                endTag = "</Object>";
            }
            formatId(buf, id);
            if (type.isArray()) {
                buf.append(" length=\"");
                buf.append(elements.length);
                buf.append('"');
            }
            buf.append(" class=\"");
            buf.append(type.getClassName());
            buf.append("\" typeId=\"");
            buf.append(type.getId());
            buf.append("\">\n");

            if (superObject != null) {
                superObject.formatRawObject(buf, indent2, null, true);
            }
            if (type.isArray()) {
                for (int i = 0; i < elements.length; i += 1) {
                    formatValue(buf, indent2, String.valueOf(i), elements[i]);
                }
            } else {
                TreeSet<String> keys = new TreeSet<String>(values.keySet());
                for (String name : keys) {
                    formatValue(buf, indent2, name, values.get(name));
                }
            }
            buf.append(indent);
            buf.append(endTag);
            buf.append("\n");
        }
    }

    private static void formatValue(StringBuffer buf,
                                    String indent,
                                    String id,
                                    Object val) {
        if (val == null) {
            buf.append(indent);
            buf.append("<Null");
            formatId(buf, id);
            buf.append("/>\n");
        } else if (val instanceof RawObject) {
            ((RawObject) val).formatRawObject(buf, indent, id, false);
        } else {
            buf.append(indent);
            buf.append("<Value");
            formatId(buf, id);
            buf.append(" class=\"");
            buf.append(val.getClass().getName());
            buf.append("\">");
            buf.append(val.toString());
            buf.append("</Value>\n");
        }
    }

    private static void formatId(StringBuffer buf, String id) {
        if (id != null) {
            if (Character.isDigit(id.charAt(0))) {
                buf.append(" index=\"");
            } else {
                buf.append(" field=\"");
            }
            buf.append(id);
            buf.append('"');
        }
    }
}
