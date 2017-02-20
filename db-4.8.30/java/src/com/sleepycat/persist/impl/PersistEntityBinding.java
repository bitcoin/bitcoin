/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.tuple.TupleBase;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.persist.raw.RawObject;

/**
 * A persistence entity binding for a given entity class.
 *
 * @author Mark Hayes
 */
public class PersistEntityBinding implements EntityBinding {

    PersistCatalog catalog;
    Format entityFormat;
    boolean rawAccess;
    PersistKeyAssigner keyAssigner;

    /**
     * Creates a key binding for a given entity class.
     */
    public PersistEntityBinding(PersistCatalog catalog,
                                String entityClassName,
                                boolean rawAccess) {
        this.catalog = catalog;
        entityFormat = getOrCreateFormat(catalog, entityClassName, rawAccess);
        if (!entityFormat.isEntity()) {
            throw new IllegalArgumentException
                ("Not an entity class: " + entityClassName);
        }
        this.rawAccess = rawAccess;
    }

    public PersistKeyAssigner getKeyAssigner() {
        return keyAssigner;
    }

    public Object entryToObject(DatabaseEntry key, DatabaseEntry data) {
        return readEntity(catalog, key, data, rawAccess);
    }

    /**
     * Creates the instance, reads the entity key first to track visited
     * entities correctly, then reads the data and returns the entity.
     *
     * This is a special case of EntityInput.readObject for a top level entity.
     * Special treatments are:
     * - The formatId must be >= 0; since this is the top level instance, it
     *   cannot refer to a visited object nor be a null reference.
     * - The resulting entity is not added to the visited object set; entities
     *   cannot be referenced by another (or the same) entity.
     * - Reader.readPriKey must be called prior to calling Reader.readObject.
     */
    static Object readEntity(Catalog catalog,
                             DatabaseEntry key,
                             DatabaseEntry data,
                             boolean rawAccess) {
        RecordInput keyInput = new RecordInput
            (catalog, rawAccess, null, 0,
             key.getData(), key.getOffset(), key.getSize());
        RecordInput dataInput = new RecordInput
            (catalog, rawAccess, null, 0,
             data.getData(), data.getOffset(), data.getSize());
        int formatId = dataInput.readPackedInt();
        Format format = catalog.getFormat(formatId);
        Reader reader = format.getReader();
        Object entity = reader.newInstance(dataInput, rawAccess);
        reader.readPriKey(entity, keyInput, rawAccess);
        return reader.readObject(entity, dataInput, rawAccess);
    }

    public void objectToData(Object entity, DatabaseEntry data) {
        Format format = getValidFormat(entity);
        writeEntity(format, catalog, entity, data, rawAccess);
    }

    /**
     * Writes the formatId and object, and returns the bytes.
     *
     * This is a special case of EntityOutput.writeObject for a top level
     * entity.  Special treatments are:
     * - The entity may not be null.
     * - The entity is not added to the visited object set nor checked for
     *   existence in the visited object set; entities cannot be referenced by
     *   another (or the same) entity.
     */
    static void writeEntity(Format format,
                            Catalog catalog,
                            Object entity,
                            DatabaseEntry data,
                            boolean rawAccess) {
        RecordOutput output = new RecordOutput(catalog, rawAccess);
        output.writePackedInt(format.getId());
        format.writeObject(entity, output, rawAccess);
        TupleBase.outputToEntry(output, data);
    }

    public void objectToKey(Object entity, DatabaseEntry key) {

        /*
         * Write the primary key field as a special case since the output
         * format is for a key binding, not entity data.
         */
        Format format = getValidFormat(entity);
        RecordOutput output = new RecordOutput(catalog, rawAccess);

        /* Write the primary key and return the bytes. */
        format.writePriKey(entity, output, rawAccess);
        TupleBase.outputToEntry(output, key);
    }

    /**
     * Returns the format for the given entity and validates it, throwing an
     * exception if it is invalid for this binding.
     */
    private Format getValidFormat(Object entity) {

        /* A null entity is not allowed. */
        if (entity == null) {
            throw new IllegalArgumentException("An entity may not be null");
        }

        /*
         * Get the format.  getFormat throws IllegalArgumentException if the
         * class is not persistent.
         */
        Format format;
        if (rawAccess) {
            if (!(entity instanceof RawObject)) {
                throw new IllegalArgumentException
                    ("Entity must be a RawObject");
            }
            format = (Format) ((RawObject) entity).getType();
        } else {
            format = catalog.getFormat
                (entity.getClass(), true /*checkEntitySubclassIndexes*/);
        }

        /* Check that the entity class/subclass is valid for this binding. */
        if (format.getEntityFormat() != entityFormat) {
            throw new IllegalArgumentException
                ("The entity class (" + format.getClassName() +
                 ") must be this entity class or a subclass of it: " +
                 entityFormat.getClassName());
        }

        return format;
    }
    
    /**
     * Utility method for getting or creating a format as appropriate for
     * bindings and key creators.
     */
    static Format getOrCreateFormat(Catalog catalog,
                                    String clsName,
                                    boolean rawAccess) {
        if (rawAccess) {
            Format format = catalog.getFormat(clsName);
            if (format == null) {
                throw new IllegalArgumentException
                    ("Not a persistent class: " + clsName);
            }
            return format;
        } else {
            Class cls = SimpleCatalog.keyClassForName(clsName);
            return catalog.getFormat(cls, true /*checkEntitySubclassIndexes*/);
        }
    }
}
