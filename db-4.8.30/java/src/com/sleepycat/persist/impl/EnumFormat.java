/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawObject;

/**
 * Format for all enum types.
 *
 * In this class we resort to using reflection to allocate arrays of enums.
 * If there is a need for it, reflection could be avoided in the future by
 * generating code as new array formats are encountered.
 *
 * @author Mark Hayes
 */
public class EnumFormat extends Format {

    private static final long serialVersionUID = 1069833955604373538L;

    private String[] names;
    private transient Object[] values;

    EnumFormat(Class type) {
        super(type);
        values = type.getEnumConstants();
        names = new String[values.length];
        for (int i = 0; i < names.length; i += 1) {
            names[i] = ((Enum) values[i]).name();
        }
    }

    /**
     * For use in a deserialized CompositeKeyFormat.
     */
    EnumFormat(Class type, String[] enumData) {
        super(type);
        names = enumData;
    }

    /**
     * Returns data needed for serialization of a CompositeKeyFormat.
     */
    String[] getFormatData() {
        return names;
    }

    @Override
    public boolean isEnum() {
        return true;
    }

    @Override
    public List<String> getEnumConstants() {
        return Arrays.asList(names);
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String,Format> newFormats) {
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
        if (values == null) {
            initValues();
        }
    }

    private void initValues() {
        Class cls = getType();
        if (cls != null) {
            values = new Object[names.length];
            for (int i = 0; i < names.length; i += 1) {
                try {
                    values[i] = Enum.valueOf(cls, names[i]);
                } catch (IllegalArgumentException e) {
                    throw new IllegalArgumentException
                        ("Deletion and renaming of enum values is not " +
                         "supported: " + names[i], e);
                }
            }
        }
    }

    @Override
    Object newArray(int len) {
        return Array.newInstance(getType(), len);
    }

    @Override
    public Object newInstance(EntityInput input, boolean rawAccess) {
        int index = input.readEnumConstant(names);
        if (rawAccess) {
            return new RawObject(this, names[index]);
        } else {
            return values[index];
        }
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess) {
        /* newInstance reads the value -- do nothing here. */
        return o;
    }

    @Override
    void writeObject(Object o, EntityOutput output, boolean rawAccess) {
        if (rawAccess) {
            String name = ((RawObject) o).getEnum();
            for (int i = 0; i < names.length; i += 1) {
                if (names[i].equals(name)) {
                    output.writeEnumConstant(names, i);
                    return;
                }
            }
        } else {
            for (int i = 0; i < values.length; i += 1) {
                if (o == values[i]) {
                    output.writeEnumConstant(names, i);
                    return;
                }
            }
        }
        throw new IllegalStateException("Bad enum: " + o);
    }

    @Override
    Object convertRawObject(Catalog catalog,
                            boolean rawAccess,
                            RawObject rawObject,
                            IdentityHashMap converted) {
        String name = rawObject.getEnum();
        for (int i = 0; i < names.length; i += 1) {
            if (names[i].equals(name)) {
                Object o = values[i];
                converted.put(rawObject, o);
                return o;
            }
        }
        throw new IllegalArgumentException
            ("Enum constant is not defined: " + name);
    }

    @Override
    void skipContents(RecordInput input) {
        input.skipFast(input.getPackedIntByteLength());
    }

    @Override
    void copySecKey(RecordInput input, RecordOutput output) {
        int len = input.getPackedIntByteLength();
        output.writeFast
            (input.getBufferBytes(), input.getBufferOffset(), len);
        input.skipFast(len);
    }

    @Override
    boolean evolve(Format newFormatParam, Evolver evolver) {
        if (!(newFormatParam instanceof EnumFormat)) {
            evolver.addEvolveError
                (this, newFormatParam,
                 "Incompatible enum type changed detected",
                 "An enum class may not be changed to a non-enum type");
            /* For future:
            evolver.addMissingMutation
                (this, newFormatParam,
                 "Converter is required when an enum class is changed to " +
                 "a non-enum type");
            */
            return false;
        }

        final EnumFormat newFormat = (EnumFormat) newFormatParam;

        /* Return quickly if the enum was not changed at all. */
        if (Arrays.equals(names, newFormat.names)) {
            evolver.useOldFormat(this, newFormat);
            return true;
        }

        final List<String> newNamesList = Arrays.asList(newFormat.names);
        final Set<String> newNamesSet = new HashSet<String>(newNamesList);
        final List<String> oldNamesList = Arrays.asList(names);

        /* Deletion (or renaming, which appears as deletion) is not allowed. */
        if (!newNamesSet.containsAll(oldNamesList)) {
            final Set<String> oldNamesSet = new HashSet<String>(oldNamesList);
            oldNamesSet.removeAll(newNamesSet);
            evolver.addEvolveError
                (this, newFormat,
                 "Incompatible enum type changed detected",
                 "Enum values may not be removed: " + oldNamesSet);
        }

        /* Use a List for additional names to preserve ordinal order. */
        final List<String> additionalNamesList =
            new ArrayList<String>(newNamesList);
        additionalNamesList.removeAll(oldNamesList);
        final int nAdditionalNames = additionalNamesList.size();

        /*
         * If there are no aditional names, the new and old formats are
         * equivalent.  This is the case where only the declaration order was
         * changed.
         */
        if (nAdditionalNames == 0) {
            evolver.useOldFormat(this, newFormat);
            return true;
        }

        /*
         * Evolve the new format.  It should use the old names array, but with
         * any additional names appended.  [#17140]
         */
        final int nOldNames = names.length;
        newFormat.names = new String[nOldNames + nAdditionalNames];
        System.arraycopy(names, 0, newFormat.names, 0, nOldNames);
        for (int i = 0; i < nAdditionalNames; i += 1) {
            newFormat.names[nOldNames + i] = additionalNamesList.get(i);
        }
        newFormat.initValues();

        /*
         * Because we never change the array index (stored integer value) for
         * an enum value, the new format can read the values written by the old
         * format (newFormat is used as the Reader in the 2nd param below).
         */
        evolver.useEvolvedFormat(this, newFormat, newFormat);
        return true;
    }
}
