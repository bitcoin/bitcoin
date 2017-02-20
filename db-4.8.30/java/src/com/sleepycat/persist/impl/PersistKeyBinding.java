/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.tuple.TupleBase;
import com.sleepycat.db.DatabaseEntry;

/**
 * A persistence key binding for a given key class.
 *
 * @author Mark Hayes
 */
public class PersistKeyBinding implements EntryBinding {

    Catalog catalog;
    Format keyFormat;
    boolean rawAccess;

    /**
     * Creates a key binding for a given key class.
     */
    public PersistKeyBinding(Catalog catalog,
                             String clsName,
                             boolean rawAccess) {
        this.catalog = catalog;
        keyFormat = PersistEntityBinding.getOrCreateFormat
            (catalog, clsName, rawAccess);
        if (!keyFormat.isSimple() &&
            !keyFormat.isEnum() &&
            !(keyFormat.getClassMetadata() != null &&
              keyFormat.getClassMetadata().getCompositeKeyFields() != null)) {
            throw new IllegalArgumentException
                ("Key class is not a simple type, an enum, or a composite " +
                 "key class (composite keys must include @KeyField " +
                 "annotations): " +
                 clsName);
        }
        this.rawAccess = rawAccess;
    }

    /**
     * Creates a key binding dynamically for use by PersistComparator.  Formats
     * are created from scratch rather than using a shared catalog.
     */
    PersistKeyBinding(final Catalog catalog,
                      final Class cls,
                      final String[] compositeFieldOrder) {
        this.catalog = catalog;
        keyFormat = new CompositeKeyFormat(cls, compositeFieldOrder);
        keyFormat.initializeIfNeeded(catalog, null /*model*/);
    }

    /**
     * Binds bytes to an object for use by PersistComparator as well as
     * entryToObject.
     */
    Object bytesToObject(byte[] bytes, int offset, int length) {
        return readKey(keyFormat, catalog, bytes, offset, length, rawAccess);
    }

    /**
     * Binds bytes to an object for use by PersistComparator as well as
     * entryToObject.
     */
    static Object readKey(Format keyFormat,
                          Catalog catalog,
                          byte[] bytes,
                          int offset,
                          int length,
                          boolean rawAccess) {
        EntityInput input = new RecordInput
            (catalog, rawAccess, null, 0, bytes, offset, length);
        return input.readKeyObject(keyFormat);
    }

    public Object entryToObject(DatabaseEntry entry) {
        return bytesToObject
            (entry.getData(), entry.getOffset(), entry.getSize());
    }

    public void objectToEntry(Object object, DatabaseEntry entry) {
        RecordOutput output = new RecordOutput(catalog, rawAccess);
        output.writeKeyObject(object, keyFormat);
        TupleBase.outputToEntry(output, entry);
    }
}
