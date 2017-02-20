/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Modifier;
import java.util.List;

/**
 * Implements Accessor using reflection.
 *
 * @author Mark Hayes
 */
class ReflectionAccessor implements Accessor {

    private static final FieldAccess[] EMPTY_KEYS = {};

    private Class type;
    private Accessor superAccessor;
    private Constructor constructor;
    private FieldAccess priKey;
    private FieldAccess[] secKeys;
    private FieldAccess[] nonKeys;

    private ReflectionAccessor(Class type, Accessor superAccessor) {
        this.type = type;
        this.superAccessor = superAccessor;
        try {
            constructor = type.getDeclaredConstructor();
        } catch (NoSuchMethodException e) {
            throw new IllegalStateException(type.getName());
        }
        if (!Modifier.isPublic(constructor.getModifiers())) {
            setAccessible(constructor, type.getName() + "()");
        }
    }

    /**
     * Creates an accessor for a complex type.
     */
    ReflectionAccessor(Catalog catalog,
                       Class type,
                       Accessor superAccessor,
                       FieldInfo priKeyField,
                       List<FieldInfo> secKeyFields,
                       List<FieldInfo> nonKeyFields) {
        this(type, superAccessor);
        if (priKeyField != null) {
            priKey = getField(catalog, priKeyField,
                              true /*isRequiredKeyField*/,
                              false /*isCompositeKey*/);
        } else {
            priKey = null;
        }
        if (secKeyFields.size() > 0) {
            secKeys = getFields(catalog, secKeyFields,
                                false /*isRequiredKeyField*/,
                                false /*isCompositeKey*/);
        } else {
            secKeys = EMPTY_KEYS;
        }
        if (nonKeyFields.size() > 0) {
            nonKeys = getFields(catalog, nonKeyFields,
                                false /*isRequiredKeyField*/,
                                false /*isCompositeKey*/);
        } else {
            nonKeys = EMPTY_KEYS;
        }
    }

    /**
     * Creates an accessor for a composite key type.
     */
    ReflectionAccessor(Catalog catalog,
                       Class type,
                       List<FieldInfo> fieldInfos) {
        this(type, null);
        priKey = null;
        secKeys = EMPTY_KEYS;
        nonKeys = getFields(catalog, fieldInfos,
                            true /*isRequiredKeyField*/,
                            true /*isCompositeKey*/);
    }

    private FieldAccess[] getFields(Catalog catalog,
                                    List<FieldInfo> fieldInfos,
                                    boolean isRequiredKeyField,
                                    boolean isCompositeKey) {
        int index = 0;
        FieldAccess[] fields = new FieldAccess[fieldInfos.size()];
        for (FieldInfo info : fieldInfos) {
            fields[index] = getField
                (catalog, info, isRequiredKeyField, isCompositeKey);
            index += 1;
        }
        return fields;
    }

    private FieldAccess getField(Catalog catalog,
                                 FieldInfo fieldInfo,
                                 boolean isRequiredKeyField,
                                 boolean isCompositeKey) {
        Field field;
        try {
            field = type.getDeclaredField(fieldInfo.getName());
        } catch (NoSuchFieldException e) {
            throw new IllegalStateException(e);
        }
        if (!Modifier.isPublic(field.getModifiers())) {
            setAccessible(field, field.getName());
        }
        Class fieldCls = field.getType();
        if (fieldCls.isPrimitive()) {
            assert SimpleCatalog.isSimpleType(fieldCls);
            return new PrimitiveAccess
                (field, SimpleCatalog.getSimpleFormat(fieldCls));
        } else if (isRequiredKeyField) {
            Format format = catalog.getFormat(fieldInfo.getClassName());
            assert format != null;
            return new KeyObjectAccess(field, format);
        } else {
            return new ObjectAccess(field);
        }
    }

    private void setAccessible(AccessibleObject object, String memberName) {
        try {
            object.setAccessible(true);
        } catch (SecurityException e) {
            throw new IllegalStateException
                ("Unable to access non-public member: " +
                 type.getName() + '.' + memberName +
                 ". Please configure the Java Security Manager setting: " +
                 " ReflectPermission suppressAccessChecks", e);
        }
    }

    public Object newInstance() {
        try {
            return constructor.newInstance();
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        } catch (InstantiationException e) {
            throw new IllegalStateException(e);
        } catch (InvocationTargetException e) {
            throw new IllegalStateException(e);
        }
    }

    public Object newArray(int len) {
        return Array.newInstance(type, len);
    }

    public boolean isPriKeyFieldNullOrZero(Object o) {
        try {
            if (priKey != null) {
                return priKey.isNullOrZero(o);
            } else if (superAccessor != null) {
                return superAccessor.isPriKeyFieldNullOrZero(o);
            } else {
                throw new IllegalStateException("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void writePriKeyField(Object o, EntityOutput output) {
        try {
            if (priKey != null) {
                priKey.write(o, output);
            } else if (superAccessor != null) {
                superAccessor.writePriKeyField(o, output);
            } else {
                throw new IllegalStateException("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void readPriKeyField(Object o, EntityInput input) {
        try {
            if (priKey != null) {
                priKey.read(o, input);
            } else if (superAccessor != null) {
                superAccessor.readPriKeyField(o, input);
            } else {
                throw new IllegalStateException("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void writeSecKeyFields(Object o, EntityOutput output) {
        try {
            if (priKey != null && !priKey.isPrimitive) {
                output.registerPriKeyObject(priKey.field.get(o));
            }
            if (superAccessor != null) {
                superAccessor.writeSecKeyFields(o, output);
            }
            for (int i = 0; i < secKeys.length; i += 1) {
                secKeys[i].write(o, output);
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void readSecKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel) {
        try {
            if (priKey != null && !priKey.isPrimitive) {
                input.registerPriKeyObject(priKey.field.get(o));
            }
            if (superLevel != 0 && superAccessor != null) {
                superAccessor.readSecKeyFields
                    (o, input, startField, endField, superLevel - 1);
            } else {
                if (superLevel > 0) {
                    throw new IllegalStateException
                        ("Superclass does not exist");
                }
            }
            if (superLevel <= 0) {
                for (int i = startField;
                     i <= endField && i < secKeys.length;
                     i += 1) {
                    secKeys[i].read(o, input);
                }
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void writeNonKeyFields(Object o, EntityOutput output) {
        try {
            if (superAccessor != null) {
                superAccessor.writeNonKeyFields(o, output);
            }
            for (int i = 0; i < nonKeys.length; i += 1) {
                nonKeys[i].write(o, output);
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void readNonKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel) {
        try {
            if (superLevel != 0 && superAccessor != null) {
                superAccessor.readNonKeyFields
                    (o, input, startField, endField, superLevel - 1);
            } else {
                if (superLevel > 0) {
                    throw new IllegalStateException
                        ("Superclass does not exist");
                }
            }
            if (superLevel <= 0) {
                for (int i = startField;
                     i <= endField && i < nonKeys.length;
                     i += 1) {
                    nonKeys[i].read(o, input);
                }
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void writeCompositeKeyFields(Object o, EntityOutput output) {
        try {
            for (int i = 0; i < nonKeys.length; i += 1) {
                nonKeys[i].write(o, output);
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void readCompositeKeyFields(Object o, EntityInput input) {
        try {
            for (int i = 0; i < nonKeys.length; i += 1) {
                nonKeys[i].read(o, input);
            }
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public Object getField(Object o,
                           int field,
                           int superLevel,
                           boolean isSecField) {
        if (superLevel > 0) {
            return superAccessor.getField
                (o, field, superLevel - 1, isSecField);
        }
        try {
            Field fld =
		isSecField ? secKeys[field].field : nonKeys[field].field;
            return fld.get(o);
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    public void setField(Object o,
                         int field,
                         int superLevel,
                         boolean isSecField,
                         Object value) {
        if (superLevel > 0) {
            superAccessor.setField
                (o, field, superLevel - 1, isSecField, value);
	    return;
        }
        try {
            Field fld =
		isSecField ? secKeys[field].field : nonKeys[field].field;
            fld.set(o, value);
        } catch (IllegalAccessException e) {
            throw new IllegalStateException(e);
        }
    }

    /**
     * Abstract base class for field access classes.
     */
    private static abstract class FieldAccess {

        Field field;
        boolean isPrimitive;

        FieldAccess(Field field) {
            this.field = field;
            isPrimitive = field.getType().isPrimitive();
        }

        /**
         * Writes a field.
         */
        abstract void write(Object o, EntityOutput out)
            throws IllegalAccessException;

        /**
         * Reads a field.
         */
        abstract void read(Object o, EntityInput in)
            throws IllegalAccessException;

        /**
         * Returns whether a field is null (for reference types) or zero (for
         * primitive integer types).  This implementation handles the reference
         * types.
         */
        boolean isNullOrZero(Object o)
            throws IllegalAccessException {

            return field.get(o) == null;
        }
    }

    /**
     * Access for fields with object types.
     */
    private static class ObjectAccess extends FieldAccess {

        ObjectAccess(Field field) {
            super(field);
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException {

            out.writeObject(field.get(o), null);
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException {

            field.set(o, in.readObject());
        }
    }

    /**
     * Access for primary key fields and composite key fields with object
     * types.
     */
    private static class KeyObjectAccess extends FieldAccess {

        private Format format;

        KeyObjectAccess(Field field, Format format) {
            super(field);
            this.format = format;
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException {

            out.writeKeyObject(field.get(o), format);
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException {

            field.set(o, in.readKeyObject(format));
        }
    }

    /**
     * Access for fields with primitive types.
     */
    private static class PrimitiveAccess extends FieldAccess {

        private SimpleFormat format;

        PrimitiveAccess(Field field, SimpleFormat format) {
            super(field);
            this.format = format;
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException {

            format.writePrimitiveField(o, out, field);
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException {

            format.readPrimitiveField(o, in, field);
        }

        @Override
        boolean isNullOrZero(Object o)
            throws IllegalAccessException {

            return field.getLong(o) == 0;
        }
    }
}
