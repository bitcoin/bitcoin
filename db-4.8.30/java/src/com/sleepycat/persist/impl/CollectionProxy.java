/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.impl;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

import com.sleepycat.bind.tuple.TupleBase;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.persist.model.Persistent;
import com.sleepycat.persist.model.PersistentProxy;
import com.sleepycat.persist.raw.RawObject;

/**
 * Proxy for Collection types.
 *
 * @author Mark Hayes
 */
@Persistent
abstract class CollectionProxy<E>
    implements PersistentProxy<Collection<E>> {

    private E[] elements;

    protected CollectionProxy() {}

    public final void initializeProxy(Collection<E> collection) {
        elements = (E[]) new Object[collection.size()];
        int i = 0;
        for (E element : collection) {
            elements[i] = element;
            i += 1;
        }
    }

    public final Collection<E> convertProxy() {
        Collection<E> collection = newInstance(elements.length);
        for (E element : elements) {
            collection.add(element);
        }
        return collection;
    }

    protected abstract Collection<E> newInstance(int size);

    @Persistent(proxyFor=ArrayList.class)
    static class ArrayListProxy<E> extends CollectionProxy<E> {

        protected ArrayListProxy() {}

        protected Collection<E> newInstance(int size) {
            return new ArrayList<E>(size);
        }
    }

    @Persistent(proxyFor=LinkedList.class)
    static class LinkedListProxy<E> extends CollectionProxy<E> {

        protected LinkedListProxy() {}

        protected Collection<E> newInstance(int size) {
            return new LinkedList<E>();
        }
    }

    @Persistent(proxyFor=HashSet.class)
    static class HashSetProxy<E> extends CollectionProxy<E> {

        protected HashSetProxy() {}

        protected Collection<E> newInstance(int size) {
            return new HashSet<E>(size);
        }
    }

    @Persistent(proxyFor=TreeSet.class)
    static class TreeSetProxy<E> extends CollectionProxy<E> {

        protected TreeSetProxy() {}

        protected Collection<E> newInstance(int size) {
            return new TreeSet<E>();
        }
    }

    static Object[] getElements(RawObject collection) {
        Object value = null;
        while (value == null && collection != null) {
            Map<String,Object> values = collection.getValues();
            if (values != null) {
                value = values.get("elements");
                if (value == null) {
                    collection = collection.getSuper();
                }
            }
        }
        if (value == null || !(value instanceof RawObject)) {
            throw new IllegalStateException
                ("Collection proxy for a secondary key field must " +
                 "contain a field named 'elements'");
        }
        RawObject rawObj = (RawObject) value;
        Format format = (Format) rawObj.getType();
        if (!format.isArray() ||
            format.getComponentType().getId() != Format.ID_OBJECT) {
            throw new IllegalStateException
                ("Collection proxy 'elements' field must be an Object array");
        }
        return rawObj.getElements();
    }

    static void setElements(RawObject collection, Object[] elements) {
        RawObject value = null;
        while (value == null && collection != null) {
            Map<String,Object> values = collection.getValues();
            if (values != null) {
                value = (RawObject) values.get("elements");
                if (value != null) {
                    values.put("elements",
                               new RawObject(value.getType(), elements));
                } else {
                    collection = collection.getSuper();
                }
            }
        }
        if (value == null) {
            throw new IllegalStateException();
        }
    }

    static void copyElements(RecordInput input,
                             Format format,
                             Format keyFormat,
                             Set results) {
        /*
         * This could be optimized by traversing the byte format of the
         * collection's elements array.
         */
        RawObject collection = (RawObject) format.newInstance(input, true);
        collection = (RawObject) format.readObject(collection, input, true);
        Object[] elements = getElements(collection);
        if (elements != null) {
            for (Object elem : elements) {
                RecordOutput output =
                    new RecordOutput(input.getCatalog(), true);
                output.writeKeyObject(elem, keyFormat);
                DatabaseEntry entry = new DatabaseEntry();
                TupleBase.outputToEntry(output, entry);
                results.add(entry);
            }
        }
    }
}
