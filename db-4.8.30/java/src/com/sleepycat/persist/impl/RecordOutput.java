/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.IdentityHashMap;
import java.util.Map;

import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.persist.raw.RawObject;

/**
 * Implements EntityOutput to write record key-data pairs.  Extends TupleOutput
 * to implement the subset of TupleOutput methods that are defined in the
 * EntityOutput interface.
 *
 * @author Mark Hayes
 */
class RecordOutput extends TupleOutput implements EntityOutput {

    private Catalog catalog;
    private boolean rawAccess;
    private Map<Object,Integer> visited;

    /**
     * Creates a new output with an empty/null visited map.
     */
    RecordOutput(Catalog catalog, boolean rawAccess) {

        super();
        this.catalog = catalog;
        this.rawAccess = rawAccess;
    }

    /**
     * @see EntityOutput#writeObject
     */
    public void writeObject(Object o, Format fieldFormat) {

        /* For a null instance, write a zero format ID. */
        if (o == null) {
            writePackedInt(Format.ID_NULL);
            return;
        }

        /*
         * For an already visited instance, output a reference to it.  The
         * reference is the negation of the visited offset minus one.
         */
        if (visited != null) {
            Integer offset = visited.get(o);
            if (offset != null) {
                if (offset == RecordInput.PROHIBIT_REF_OFFSET) {
                    throw new IllegalArgumentException
                        (RecordInput.PROHIBIT_NESTED_REF_MSG);
                } else {
                    writePackedInt(-(offset + 1));
                    return;
                }
            }
        }

        /*
         * Get and validate the format.  Catalog.getFormat(Class) throws
         * IllegalArgumentException if the class is not persistent.  We don't
         * need to check the fieldFormat (and it will be null) for non-raw
         * access because field type checking is enforced by Java.
         */
        Format format;
        if (rawAccess) {
            format = RawAbstractInput.checkRawType(catalog, o, fieldFormat);
        } else {

            /*
             * Do not attempt to open subclass indexes in case this is an
             * embedded entity.  We will detect that error below, but we must
             * not fail first when attempting to open the secondaries.
             */
            format = catalog.getFormat
                (o.getClass(), false /*checkEntitySubclassIndexes*/);
        }
        if (format.getProxiedFormat() != null) {
            throw new IllegalArgumentException
                ("May not store proxy classes directly: " +
                 format.getClassName());
        }
        /* Check for embedded entity classes and subclasses. */
        if (format.getEntityFormat() != null) {
            throw new IllegalArgumentException
                ("References to entities are not allowed: " +
                 o.getClass().getName());
        }

        /*
         * Remember that we visited this instance.  Certain formats
         * (ProxiedFormat for example) prohibit nested fields that reference
         * the parent object. [#15815]
         */
        if (visited == null) {
            visited = new IdentityHashMap<Object,Integer>();
        }
        boolean prohibitNestedRefs = format.areNestedRefsProhibited();
        Integer visitedOffset = size();
        visited.put(o, prohibitNestedRefs ? RecordInput.PROHIBIT_REF_OFFSET :
                       visitedOffset);

        /* Finally, write the formatId and object value. */
        writePackedInt(format.getId());
        format.writeObject(o, this, rawAccess);

        /* Always allow references from siblings that follow. */
        if (prohibitNestedRefs) {
            visited.put(o, visitedOffset);
        }
    }

    /**
     * @see EntityOutput#writeKeyObject
     */
    public void writeKeyObject(Object o, Format fieldFormat) {

        /* Key objects must not be null and must be of the declared class. */
        if (o == null) {
            throw new IllegalArgumentException
                ("Key field object may not be null");
        }
        Format format;
        if (rawAccess) {
            if (o instanceof RawObject) {
                format = (Format) ((RawObject) o).getType();
            } else {
                format = catalog.getFormat
                    (o.getClass(), false /*checkEntitySubclassIndexes*/);
                /* Expect primitive wrapper class in raw mode. */
                if (fieldFormat.isPrimitive()) {
                    fieldFormat = fieldFormat.getWrapperFormat();
                }
            }
        } else {
            format = catalog.getFormat(o.getClass(),
                                       false /*checkEntitySubclassIndexes*/);
        }
        if (fieldFormat != format) {
            throw new IllegalArgumentException
                ("The key field object class (" + o.getClass().getName() +
                 ") must be the field's declared class: " +
                 fieldFormat.getClassName());
        }

        /* Write the object value (no formatId is written for keys). */
        fieldFormat.writeObject(o, this, rawAccess);
    }

    /**
     * @see EntityOutput#registerPriKeyObject
     */
    public void registerPriKeyObject(Object o) {

        /*
         * PRI_KEY_VISITED_OFFSET is used as the visited offset to indicate
         * that the visited object is stored in the primary key byte array.
         */
        if (visited == null) {
            visited = new IdentityHashMap<Object,Integer>();
        }
        visited.put(o, RecordInput.PRI_KEY_VISITED_OFFSET);
    }

    /**
     * @see EntityOutput#writeArrayLength
     */
    public void writeArrayLength(int length) {
        writePackedInt(length);
    }

    /**
     * @see EntityOutput#writeEnumConstant
     */
    public void writeEnumConstant(String[] names, int index) {
        writePackedInt(index);
    }
}
