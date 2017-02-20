/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Array;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.Set;

import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawObject;

/**
 * An array of objects having a specified number of dimensions.  All
 * multidimensional arrays are handled by this class, since even a primitive
 * array of more than one dimension is an array of objects, where the component
 * objects may be primitive arrays.  The {@link PrimitiveArrayFormat} class
 * handles primitive arrays of one dimension only.
 *
 * In this class, and {@link PrimitiveArrayFormat}, we resort to using
 * reflection to allocate multidimensional arrays.  If there is a need for it,
 * reflection could be avoided in the future by generating code as new array
 * formats are encountered.
 *
 * @author Mark Hayes
 */
public class ObjectArrayFormat extends Format {

    private static final long serialVersionUID = 4317004346690441892L;

    private Format componentFormat;
    private int nDimensions;
    private transient Format useComponentFormat;

    ObjectArrayFormat(Class type) {
        super(type);
        String name = getClassName();
        for (nDimensions = 0;
             name.charAt(nDimensions) == '[';
             nDimensions += 1) {
        }
    }

    @Override
    public boolean isArray() {
        return true;
    }

    @Override
    public int getDimensions() {
        return nDimensions;
    }

    @Override
    public Format getComponentType() {
        return (useComponentFormat != null) ?
            useComponentFormat : componentFormat;
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String,Format> newFormats) {
        Class cls = getType().getComponentType();
        catalog.createFormat(cls, newFormats);
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
        /* Set the component format for a new (never initialized) format. */
        if (componentFormat == null) {
            Class cls = getType().getComponentType();
            componentFormat = catalog.getFormat(cls.getName());
        }
        useComponentFormat = componentFormat.getLatestVersion();
    }

    @Override
    boolean isAssignableTo(Format format) {
        if (super.isAssignableTo(format)) {
            return true;
        }
        if (format instanceof ObjectArrayFormat) {
            ObjectArrayFormat other = (ObjectArrayFormat) format;
            if (useComponentFormat.isAssignableTo(other.useComponentFormat)) {
                return true;
            }
        }
        return false;
    }

    @Override
    Object newArray(int len) {
        return Array.newInstance(getType(), len);
    }

    @Override
    public Object newInstance(EntityInput input, boolean rawAccess) {
        int len = input.readArrayLength();
        if (rawAccess) {
            return new RawObject(this, new Object[len]);
        } else {
            return useComponentFormat.newArray(len);
        }
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess) {
        Object[] a;
        if (rawAccess) {
            a = ((RawObject) o).getElements();
        } else {
            a = (Object[]) o;
        }
        for (int i = 0; i < a.length; i += 1) {
            a[i] = input.readObject();
        }
        return o;
    }

    @Override
    void writeObject(Object o, EntityOutput output, boolean rawAccess) {
        Object[] a;
        if (rawAccess) {
            a = ((RawObject) o).getElements();
        } else {
            a = (Object[]) o;
        }
        output.writeArrayLength(a.length);
        for (int i = 0; i < a.length; i += 1) {
            output.writeObject(a[i], useComponentFormat);
        }
    }

    @Override
    Object convertRawObject(Catalog catalog,
                            boolean rawAccess,
                            RawObject rawObject,
                            IdentityHashMap converted) {
        RawArrayInput input = new RawArrayInput
            (catalog, rawAccess, converted, rawObject, useComponentFormat);
        Object a = newInstance(input, rawAccess);
        converted.put(rawObject, a);
        return readObject(a, input, rawAccess);
    }

    @Override
    void skipContents(RecordInput input) {
        int len = input.readPackedInt();
        for (int i = 0; i < len; i += 1) {
            input.skipField(useComponentFormat);
        }
    }

    @Override
    void copySecMultiKey(RecordInput input, Format keyFormat, Set results) {
        int len = input.readPackedInt();
        for (int i = 0; i < len; i += 1) {
            KeyLocation loc = input.getKeyLocation(useComponentFormat);
            if (loc == null) {
                throw new IllegalArgumentException
                    ("Secondary key values in array may not be null");
            }
            if (loc.format != useComponentFormat) {
                throw new IllegalStateException
                    (useComponentFormat.getClassName());
            }
            int off1 = loc.input.getBufferOffset();
            useComponentFormat.skipContents(loc.input);
            int off2 = loc.input.getBufferOffset();
            DatabaseEntry entry = new DatabaseEntry
                (loc.input.getBufferBytes(), off1, off2 - off1);
            results.add(entry);
        }
    }

    @Override
    boolean evolve(Format newFormat, Evolver evolver) {

        /*
         * When the class name of the component changes, we need a new format
         * that references it.  Otherwise, don't propogate changes from
         * components upward to their arrays.
         */
        Format latest = componentFormat.getLatestVersion();
        if (latest != componentFormat &&
            !latest.getClassName().equals(componentFormat.getClassName())) {
            evolver.useEvolvedFormat(this, newFormat, newFormat);
        } else {
            evolver.useOldFormat(this, newFormat);
        }
        return true;
    }
}
