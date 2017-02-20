/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.collections;

import java.util.Map;

/**
 * A simple <code>Map.Entry</code> implementation that can be used as in
 * input parameter.  Since a <code>MapEntryParameter</code> is not obtained
 * from a map, it is not attached to any map in particular.  To emphasize that
 * changing this object does not change the map, the {@link #setValue} method
 * always throws <code>UnsupportedOperationException</code>.
 *
 * <p><b>Warning:</b> Use of this interface violates the Java Collections
 * interface contract since these state that <code>Map.Entry</code> objects
 * should only be obtained from <code>Map.entrySet()</code> sets, while this
 * class allows constructing them directly.  However, it is useful for
 * performing operations on an entry set such as add(), contains(), etc.  For
 * restrictions see {@link #getValue} and {@link #setValue}.</p>
 *
 * @author Mark Hayes
 */
public class MapEntryParameter<K,V> implements Map.Entry<K,V> {

    private K key;
    private V value;

    /**
     * Creates a map entry with a given key and value.
     *
     * @param key is the key to use.
     *
     * @param value is the value to use.
     */
    public MapEntryParameter(K key, V value) {

        this.key = key;
        this.value = value;
    }

    /**
     * Computes a hash code as specified by {@link
     * java.util.Map.Entry#hashCode}.
     *
     * @return the computed hash code.
     */
    public int hashCode() {

        return ((key == null)    ? 0 : key.hashCode()) ^
               ((value == null)  ? 0 : value.hashCode());
    }

    /**
     * Compares this entry to a given entry as specified by {@link
     * java.util.Map.Entry#equals}.
     *
     * @return the computed hash code.
     */
    public boolean equals(Object other) {

        if (!(other instanceof Map.Entry)) {
            return false;
        }

        Map.Entry e = (Map.Entry) other;

        return ((key == null) ? (e.getKey() == null)
                              : key.equals(e.getKey())) &&
               ((value == null) ? (e.getValue() == null)
                                : value.equals(e.getValue()));
    }

    /**
     * Returns the key of this entry.
     *
     * @return the key of this entry.
     */
    public final K getKey() {

        return key;
    }

    /**
     * Returns the value of this entry.  Note that this will be the value
     * passed to the constructor or the last value passed to {@link #setValue}.
     * It will not reflect changes made to a Map.
     *
     * @return the value of this entry.
     */
    public final V getValue() {

        return value;
    }

    /**
     * Always throws <code>UnsupportedOperationException</code> since this
     * object is not attached to a map.
     */
    public V setValue(V newValue) {

        throw new UnsupportedOperationException();
    }

    final void setValueInternal(V newValue) {

        this.value = newValue;
    }

    /**
     * Converts the entry to a string representation for debugging.
     *
     * @return the string representation.
     */
    public String toString() {

        return "[key [" + key + "] value [" + value + ']';
    }
}
