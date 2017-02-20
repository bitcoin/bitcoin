/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.HashMap;
import java.util.Map;

import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.db.DatabaseEntry;

/**
 * Implements EntityInput to read record key-data pairs.  Extends TupleInput to
 * implement the subset of TupleInput methods that are defined in the
 * EntityInput interface.
 *
 * @author Mark Hayes
 */
class RecordInput extends TupleInput implements EntityInput {

    /* Initial size of visited map. */
    static final int VISITED_INIT_SIZE = 50;

    /*
     * Offset to indicate that the visited object is stored in the primary key
     * byte array.
     */
    static final int PRI_KEY_VISITED_OFFSET = Integer.MAX_VALUE - 1;

    /* Used by RecordOutput to prevent illegal nested references. */
    static final int PROHIBIT_REF_OFFSET = Integer.MAX_VALUE - 2;

    /* Used by RecordInput to prevent illegal nested references. */
    static final Object PROHIBIT_REF_OBJECT = new Object();

    static final String PROHIBIT_NESTED_REF_MSG =
        "Cannot embed a reference to a proxied object in the proxy; for " +
        "example, a collection may not be an element of the collection " +
        "because collections are proxied";

    private Catalog catalog;
    private boolean rawAccess;
    private Map<Integer,Object> visited;
    private DatabaseEntry priKeyEntry;
    private int priKeyFormatId;

    /**
     * Creates a new input with a empty/null visited map.
     */
    RecordInput(Catalog catalog,
                boolean rawAccess,
                DatabaseEntry priKeyEntry,
                int priKeyFormatId,
                byte[] buffer,
                int offset,
                int length) {
        super(buffer, offset, length);
        this.catalog = catalog;
        this.rawAccess = rawAccess;
        this.priKeyEntry = priKeyEntry;
        this.priKeyFormatId = priKeyFormatId;
    }

    /**
     * Copy contructor where a new offset can be specified.
     */
    private RecordInput(RecordInput other, int offset) {
        this(other.catalog, other.rawAccess, other.priKeyEntry,
             other.priKeyFormatId, other.buf, offset, other.len);
        visited = other.visited;
    }

    /**
     * Copy contructor where a DatabaseEntry can be specified.
     */
    private RecordInput(RecordInput other, DatabaseEntry entry) {
        this(other.catalog, other.rawAccess, other.priKeyEntry,
             other.priKeyFormatId, entry.getData(), entry.getOffset(),
             entry.getSize());
        visited = other.visited;
    }

    /**
     * @see EntityInput#getCatalog
     */
    public Catalog getCatalog() {
        return catalog;
    }

    /**
     * @see EntityInput#isRawAccess
     */
    public boolean isRawAccess() {
        return rawAccess;
    }

    /**
     * @see EntityInput#setRawAccess
     */
    public boolean setRawAccess(boolean rawAccessParam) {
        boolean original = rawAccess;
        rawAccess = rawAccessParam;
        return original;
    }

    /**
     * @see EntityInput#readObject
     */
    public Object readObject() {

        /* Save the current offset before reading the format ID. */
        Integer visitedOffset = off;
        RecordInput useInput = this;
        int formatId = readPackedInt();
        Object o = null;

        /* For a zero format ID, return a null instance. */
        if (formatId == Format.ID_NULL) {
            return null;
        }

        /* For a negative format ID, lookup an already visited instance. */
        if (formatId < 0) {
            int offset = (-(formatId + 1));
            if (visited != null) {
                o = visited.get(offset);
            }
            if (o == RecordInput.PROHIBIT_REF_OBJECT) {
                throw new IllegalArgumentException
                    (RecordInput.PROHIBIT_NESTED_REF_MSG);
            }
            if (o != null) {
                /* Return a previously visited object. */
                return o;
            } else {

                /*
                 * When reading starts from a non-zero offset, we may have to
                 * go back in the stream and read the referenced object.  This
                 * happens when reading secondary key fields.
                 */
                visitedOffset = offset;
                if (offset == RecordInput.PRI_KEY_VISITED_OFFSET) {
                    assert priKeyEntry != null && priKeyFormatId > 0;
                    useInput = new RecordInput(this, priKeyEntry);
                    formatId = priKeyFormatId;
                } else {
                    useInput = new RecordInput(this, offset);
                    formatId = useInput.readPackedInt();
                }
            }
        }

        /*
         * Add a visted object slot that prohibits nested references to this
         * object during the call to Reader.newInstance below.  The newInstance
         * method is allowed to read nested fields (in which case
         * Reader.readObject further below does nothing) under certain
         * conditions, but under these conditions we do not support nested
         * references to the parent object. [#15815]
         */
        if (visited == null) {
            visited = new HashMap<Integer,Object>(VISITED_INIT_SIZE);
        }
        visited.put(visitedOffset, RecordInput.PROHIBIT_REF_OBJECT);

        /* Create the object using the format indicated. */
        Format format = catalog.getFormat(formatId);
        Reader reader = format.getReader();
        o = reader.newInstance(useInput, rawAccess);

        /*
         * Set the newly created object in the map of visited objects.  This
         * must be done before calling Reader.readObject, which allows the
         * object to contain a reference to itself.
         */
        visited.put(visitedOffset, o);

        /*
         * Finish reading the object.  Then replace it in the visited map in
         * case a converted object is returned by readObject.
         */
        Object o2 = reader.readObject(o, useInput, rawAccess);
        if (o != o2) {
            visited.put(visitedOffset, o2);
        }
        return o2;
    }

    /**
     * @see EntityInput#readKeyObject
     */
    public Object readKeyObject(Format format) {

        /* Create and read the object using the given key format. */
        Reader reader = format.getReader();
        Object o = reader.newInstance(this, rawAccess);
        return reader.readObject(o, this, rawAccess);
    }

    /**
     * Called when copying secondary keys, for an input that is positioned on
     * the secondary key field.  Handles references to previously occurring
     * objects, returning a different RecordInput than this one if appropriate.
     */
    KeyLocation getKeyLocation(Format fieldFormat) {
        RecordInput input = this;
        if (!fieldFormat.isPrimitive()) {
            int formatId = input.readPackedInt();
            if (formatId == Format.ID_NULL) {
                /* Key field is null. */
                return null;
            }
            if (formatId < 0) {
                int offset = (-(formatId + 1));
                if (offset == RecordInput.PRI_KEY_VISITED_OFFSET) {
                    assert priKeyEntry != null && priKeyFormatId > 0;
                    input = new RecordInput(this, priKeyEntry);
                    formatId = priKeyFormatId;
                } else {
                    input = new RecordInput(this, offset);
                    formatId = input.readPackedInt();
                }
            }
            fieldFormat = catalog.getFormat(formatId);
        }
        /* Key field is non-null. */
        return new KeyLocation(input, fieldFormat);
    }

    /**
     * @see EntityInput#registerPriKeyObject
     */
    public void registerPriKeyObject(Object o) {

        /*
         * PRI_KEY_VISITED_OFFSET is used as the visited offset to indicate
         * that the visited object is stored in the primary key byte array.
         */
        if (visited == null) {
            visited = new HashMap<Integer,Object>(VISITED_INIT_SIZE);
        }
        visited.put(RecordInput.PRI_KEY_VISITED_OFFSET, o);
    }

    /**
     * @see EntityInput#skipField
     */
    public void skipField(Format declaredFormat) {
        if (declaredFormat != null && declaredFormat.isPrimitive()) {
            declaredFormat.skipContents(this);
        } else {
            int formatId = readPackedInt();
            if (formatId > 0) {
                Format format = catalog.getFormat(formatId);
                format.skipContents(this);
            }
        }
    }

    public int readArrayLength() {
        return readPackedInt();
    }

    public int readEnumConstant(String[] names) {
        return readPackedInt();
    }
}
