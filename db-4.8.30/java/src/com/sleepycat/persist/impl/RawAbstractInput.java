/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.math.BigInteger;
import java.util.IdentityHashMap;

import com.sleepycat.persist.raw.RawObject;

/**
 * Base class for EntityInput implementations that type-check RawObject
 * instances and convert them to regular persistent objects, via the
 * Format.convertRawObject method.
 *
 * The subclass implements readNext which should call checkAndConvert before
 * returning the final value.
 *
 * @author Mark Hayes
 */
abstract class RawAbstractInput extends AbstractInput {

    private IdentityHashMap converted;

    RawAbstractInput(Catalog catalog,
                     boolean rawAccess,
                     IdentityHashMap converted) {
        super(catalog, rawAccess);
        this.converted = converted;
    }

    public Object readObject() {
        return readNext();
    }

    public Object readKeyObject(Format format) {
        return readNext();
    }

    public void registerPriKeyObject(Object o) {
    }

    public int readArrayLength() {
        throw new UnsupportedOperationException();
    }

    public int readEnumConstant(String[] names) {
        throw new UnsupportedOperationException();
    }

    public void skipField(Format declaredFormat) {
    }

    abstract Object readNext();

    Object checkAndConvert(Object o, Format declaredFormat) {
        if (o == null) {
            if (declaredFormat.isPrimitive()) {
                throw new IllegalArgumentException
                    ("A primitive type may not be null or missing: " +
                     declaredFormat.getClassName());
            }
        } else if (declaredFormat.isSimple()) {
            if (declaredFormat.isPrimitive()) {
                if (o.getClass() !=
                    declaredFormat.getWrapperFormat().getType()) {
                    throw new IllegalArgumentException
                        ("Raw value class: " + o.getClass().getName() +
                         " must be the wrapper class for a primitive type: " +
                         declaredFormat.getClassName());
                }
            } else {
                if (o.getClass() != declaredFormat.getType()) {
                    throw new IllegalArgumentException
                        ("Raw value class: " + o.getClass().getName() +
                         " must be the declared class for a simple type: " +
                         declaredFormat.getClassName());
                }
            }
        } else {
            if (o instanceof RawObject) {
                Object o2 = null;
                if (!rawAccess) {
                    if (converted != null) {
                        o2 = converted.get(o);
                    } else {
                        converted = new IdentityHashMap();
                    }
                }
                if (o2 != null) {
                    o = o2;
                } else {
                    if (!rawAccess) {
                        o = catalog.convertRawObject((RawObject) o, converted);
                    }
                }
            } else {
                if (!SimpleCatalog.isSimpleType(o.getClass())) {
                    throw new IllegalArgumentException
                        ("Raw value class: " + o.getClass().getName() +
                         " must be RawObject a simple type");
                }
            }
            if (rawAccess) {
                checkRawType(catalog, o, declaredFormat);
            } else {
                if (!declaredFormat.getType().isAssignableFrom(o.getClass())) {
                    throw new IllegalArgumentException
                        ("Raw value class: " + o.getClass().getName() +
                         " is not assignable to type: " +
                         declaredFormat.getClassName());
                }
            }
        }
        return o;
    }

    static Format checkRawType(Catalog catalog,
                               Object o,
                               Format declaredFormat) {
        assert declaredFormat != null;
        Format format;
        if (o instanceof RawObject) {
            format = (Format) ((RawObject) o).getType();
        } else {
            format = catalog.getFormat(o.getClass(),
                                       false /*checkEntitySubclassIndexes*/);
            if (!format.isSimple() || format.isEnum()) {
                throw new IllegalArgumentException
                    ("Not a RawObject or a non-enum simple type: " +
                     format.getClassName());
            }
        }
        if (!format.isAssignableTo(declaredFormat)) {
            throw new IllegalArgumentException
                ("Not a subtype of the field's declared class " +
                 declaredFormat.getClassName() + ": " +
                 format.getClassName());
        }
        if (!format.isCurrentVersion()) {
            throw new IllegalArgumentException
                ("Raw type version is not current.  Class: " +
                 format.getClassName() + " Version: " +
                 format.getVersion());
        }
        Format proxiedFormat = format.getProxiedFormat();
        if (proxiedFormat != null) {
            format = proxiedFormat;
        }
        return format;
    }

    /* The following methods are a subset of the methods in TupleInput. */

    public String readString() {
        return (String) readNext();
    }

    public char readChar() {
        return ((Character) readNext()).charValue();
    }

    public boolean readBoolean() {
        return ((Boolean) readNext()).booleanValue();
    }

    public byte readByte() {
        return ((Byte) readNext()).byteValue();
    }

    public short readShort() {
        return ((Short) readNext()).shortValue();
    }

    public int readInt() {
        return ((Integer) readNext()).intValue();
    }

    public long readLong() {
        return ((Long) readNext()).longValue();
    }

    public float readSortedFloat() {
        return ((Float) readNext()).floatValue();
    }

    public double readSortedDouble() {
        return ((Double) readNext()).doubleValue();
    }

    public BigInteger readBigInteger() {
        return (BigInteger) readNext();
    }
}
