/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

/**
 * Keeps track of a set of visited objects and their corresponding offset in a
 * byte array.  This uses a resizable int array for speed and simplicity.  If
 * in the future the array resizing or linear search are performance issues, we
 * could try using an IdentityHashMap instead.
 *
 * @author Mark Hayes
 */
class VisitedObjects {

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

    private static final int INIT_LEN = 50;

    private Object[] objects;
    private int[] offsets;
    private int nextIndex;

    /**
     * Creates an empty set.
     */
    VisitedObjects() {
        objects = new Object[INIT_LEN];
        offsets = new int[INIT_LEN];
        nextIndex = 0;
    }

    /**
     * Adds a visited object and offset, growing the visited arrays as needed.
     * @return the index of the new slot.
     */
    int add(Object o, int offset) {

        int i = nextIndex;
        nextIndex += 1;
        if (nextIndex > objects.length) {
            growVisitedArrays();
        }
        objects[i] = o;
        offsets[i] = offset;
        return i;
    }

    /**
     * Sets the object for an existing slot index.
     */
    void setObject(int index, Object o) {
        objects[index] = o;
    }

    /**
     * Sets the offset for an existing slot index.
     */
    void setOffset(int index, int offset) {
        offsets[index] = offset;
    }

    /**
     * Returns the offset for a visited object, or -1 if never visited.
     */
    int getOffset(Object o) {
        for (int i = 0; i < nextIndex; i += 1) {
            if (objects[i] == o) {
                return offsets[i];
            }
        }
        return -1;
    }

    /**
     * Returns the visited object for a given offset, or null if never visited.
     */
    Object getObject(int offset) {
        for (int i = 0; i < nextIndex; i += 1) {
            if (offsets[i] == offset) {
                return objects[i];
            }
        }
        return null;
    }

    /**
     * Replaces a given object in the list.  Used when an object is converted
     * after adding it to the list.
     */
    void replaceObject(Object existing, Object replacement) {
        for (int i = nextIndex - 1; i >= 0; i -= 1) {
            if (objects[i] == existing) {
                objects[i] = replacement;
                return;
            }
        }
        assert false;
    }

    /**
     * Doubles the size of the visited arrays.
     */
    private void growVisitedArrays() {

        int oldLen = objects.length;
        int newLen = oldLen * 2;

        Object[] newObjects = new Object[newLen];
        int[] newOffsets = new int[newLen];

        System.arraycopy(objects, 0, newObjects, 0, oldLen);
        System.arraycopy(offsets, 0, newOffsets, 0, oldLen);

        objects = newObjects;
        offsets = newOffsets;
    }
}
