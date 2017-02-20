/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Array;
import java.lang.reflect.Modifier;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Implements Accessor for a complex persistent class.
 *
 * @author Mark Hayes
 */
public class EnhancedAccessor implements Accessor {

    private static final Map<String,Enhanced> classRegistry =
        Collections.synchronizedMap(new HashMap<String,Enhanced>());

    /* Is public for unit tests. */
    public static final boolean EXPECT_ENHANCED =
        "true".equals(System.getProperty("expectEnhanced"));

    private Enhanced prototype;
    private Format priKeyFormat;
    private Format[] compositeKeyFormats; 
    private Class type;

    /**
     * Registers a prototype instance, and should be called during
     * initialization of the prototype class.  The prototype may be null for
     * an abstract class.
     */
    public static void registerClass(String className, Enhanced prototype) {
        classRegistry.put(className, prototype);
    }

    /**
     * Returns whether a given class is a (registered) enhanced class.
     */
    static boolean isEnhanced(Class type) {
        boolean enhanced = classRegistry.containsKey(type.getName());
        if (!enhanced && EXPECT_ENHANCED) {
            throw new IllegalStateException
                ("Test was run with expectEnhanced=true but class " +
                 type.getName() + " is not enhanced");
        }
        return enhanced;
    }

    private EnhancedAccessor(Class type) {
        this.type = type;
        prototype = classRegistry.get(type.getName());
        assert prototype != null || Modifier.isAbstract(type.getModifiers());
    }

    /**
     * Creates an accessor for a complex type.
     */
    EnhancedAccessor(Catalog catalog, Class type, ComplexFormat format) {
        this(type);

        /*
         * Find the primary key format for this format or one of its superclass
         * formats.
         */
        ComplexFormat declaringFormat = format;
        while (declaringFormat != null) {
            FieldInfo priKeyField = declaringFormat.getPriKeyFieldInfo();
            if (priKeyField != null) {
                priKeyFormat = catalog.getFormat(priKeyField.getClassName());
                break;
            } else {
                declaringFormat = declaringFormat.getComplexSuper();
            }
        }
    }

    /**
     * Creates an accessor for a composite key type.
     */
    EnhancedAccessor(Catalog catalog, Class type, List<FieldInfo> fieldInfos) {
        this(type);
        final int nFields = fieldInfos.size();
        compositeKeyFormats = new Format[nFields];
        for (int i = 0; i < nFields; i += 1) {
            compositeKeyFormats[i] =
                catalog.getFormat(fieldInfos.get(i).getClassName());
        }
    }

    public Object newInstance() {
        if (prototype == null) {
            /* Abstract class -- internal error. */
            throw new IllegalStateException();
        }
        return prototype.bdbNewInstance();
    }

    public Object newArray(int len) {
        if (prototype == null) {
            /* Abstract class -- use reflection for now. */
            return Array.newInstance(type, len);
        }
        return prototype.bdbNewArray(len);
    }

    public boolean isPriKeyFieldNullOrZero(Object o) {
        if (priKeyFormat == null) {
            throw new IllegalStateException
                ("No primary key: " + o.getClass().getName());
        }
        return ((Enhanced) o).bdbIsPriKeyFieldNullOrZero();
    }

    public void writePriKeyField(Object o, EntityOutput output) {
        if (priKeyFormat == null) {
            throw new IllegalStateException
                ("No primary key: " + o.getClass().getName());
        }
        ((Enhanced) o).bdbWritePriKeyField(output, priKeyFormat);
    }

    public void readPriKeyField(Object o, EntityInput input) {
        if (priKeyFormat == null) {
            throw new IllegalStateException
                ("No primary key: " + o.getClass().getName());
        }
        ((Enhanced) o).bdbReadPriKeyField(input, priKeyFormat);
    }

    public void writeSecKeyFields(Object o, EntityOutput output) {
        ((Enhanced) o).bdbWriteSecKeyFields(output);
    }

    public void readSecKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel) {
        ((Enhanced) o).bdbReadSecKeyFields
            (input, startField, endField, superLevel);
    }

    public void writeNonKeyFields(Object o, EntityOutput output) {
        ((Enhanced) o).bdbWriteNonKeyFields(output);
    }

    public void readNonKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel) {
        ((Enhanced) o).bdbReadNonKeyFields
            (input, startField, endField, superLevel);
    }

    public void writeCompositeKeyFields(Object o, EntityOutput output) {
        ((Enhanced) o).bdbWriteCompositeKeyFields(output, compositeKeyFormats);
    }

    public void readCompositeKeyFields(Object o, EntityInput input) {
        ((Enhanced) o).bdbReadCompositeKeyFields(input, compositeKeyFormats);
    }

    public Object getField(Object o,
                           int field,
                           int superLevel,
                           boolean isSecField) {
        return ((Enhanced) o).bdbGetField(o, field, superLevel, isSecField);
    }

    public void setField(Object o,
                         int field,
                         int superLevel,
                         boolean isSecField,
                         Object value) {
        ((Enhanced) o).bdbSetField(o, field, superLevel, isSecField, value);
    }
}
