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
import java.util.TreeMap;

import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PersistentProxy;

/**
 * Proxy for a Map.
 *
 * @author Mark Hayes
 */
@Persistent
abstract class MapProxy<K,V> implements PersistentProxy<Map<K,V>> {

    private K[] keys;
    private V[] values;

    protected MapProxy() {}

    public final void initializeProxy(Map<K,V> map) {
        int size = map.size();
        keys = (K[]) new Object[size];
        values = (V[]) new Object[size];
        int i = 0;
        for (Map.Entry<K,V> entry : map.entrySet()) {
            keys[i] = entry.getKey();
            values[i] = entry.getValue();
            i += 1;
        }
    }

    public final Map<K,V> convertProxy() {
        int size = values.length;
        Map<K,V> map = newInstance(size);
        for (int i = 0; i < size; i += 1) {
            map.put(keys[i], values[i]);
        }
        return map;
    }

    protected abstract Map<K,V> newInstance(int size);

    @Persistent(proxyFor=HashMap.class)
    static class HashMapProxy<K,V> extends MapProxy<K,V> {

        protected HashMapProxy() {}

        protected Map<K,V> newInstance(int size) {
            return new HashMap<K,V>(size);
        }
    }

    @Persistent(proxyFor=TreeMap.class)
    static class TreeMapProxy<K,V> extends MapProxy<K,V> {

        protected TreeMapProxy() {}

        protected Map<K,V> newInstance(int size) {
            return new TreeMap<K,V>();
        }
    }
}
