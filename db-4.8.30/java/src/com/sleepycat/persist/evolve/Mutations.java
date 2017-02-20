/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.evolve;

import java.io.Serializable;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.StoreConfig;

/**
 * A collection of mutations for configuring class evolution.
 *
 * <p>Mutations are configured when a store is opened via {@link
 * StoreConfig#setMutations StoreConfig.setMutations}.  For example:</p>
 *
 * <pre class="code">
 *  Mutations mutations = new Mutations();
 *  // Add mutations...
 *  StoreConfig config = new StoreConfig();
 *  config.setMutations(mutations);
 *  EntityStore store = new EntityStore(env, "myStore", config);</pre>
 *
 * <p>Mutations cause data conversion to occur lazily as instances are read
 * from the store.  The {@link EntityStore#evolve EntityStore.evolve} method
 * may also be used to perform eager conversion.</p>
 *
 * <p>Not all incompatible class changes can be handled via mutations.  For
 * example, complex refactoring may require a transformation that manipulates
 * multiple entity instances at once.  Such changes are not possible with
 * mutations but can made by performing a <a
 * href="package-summary.html#storeConversion">store conversion</a>.</p>
 *
 * @see com.sleepycat.persist.evolve Class Evolution
 * @author Mark Hayes
 */
public class Mutations implements Serializable {

    private static final long serialVersionUID = -1744401530444812916L;

    private Map<Mutation,Renamer> renamers;
    private Map<Mutation,Deleter> deleters;
    private Map<Mutation,Converter> converters;

    /**
     * Creates an empty set of mutations.
     */
    public Mutations() {
        renamers = new HashMap<Mutation,Renamer>();
        deleters = new HashMap<Mutation,Deleter>();
        converters = new HashMap<Mutation,Converter>();
    }

    /**
     * Returns true if no mutations are present.
     */
    public boolean isEmpty() {
        return renamers.isEmpty() &&
               deleters.isEmpty() &&
               converters.isEmpty();
    }

    /**
     * Adds a renamer mutation.
     */
    public void addRenamer(Renamer renamer) {
        renamers.put(new Key(renamer), renamer);
    }

    /**
     * Returns the renamer mutation for the given class, version and field, or
     * null if none exists.  A null field name should be specified to get a
     * class renamer.
     */
    public Renamer getRenamer(String className,
                              int classVersion,
                              String fieldName) {
        return renamers.get(new Key(className, classVersion, fieldName));
    }

    /**
     * Returns an unmodifiable collection of all renamer mutations.
     */
    public Collection<Renamer> getRenamers() {
        return renamers.values();
    }

    /**
     * Adds a deleter mutation.
     */
    public void addDeleter(Deleter deleter) {
        deleters.put(new Key(deleter), deleter);
    }

    /**
     * Returns the deleter mutation for the given class, version and field, or
     * null if none exists.  A null field name should be specified to get a
     * class deleter.
     */
    public Deleter getDeleter(String className,
                              int classVersion,
                              String fieldName) {
        return deleters.get(new Key(className, classVersion, fieldName));
    }

    /**
     * Returns an unmodifiable collection of all deleter mutations.
     */
    public Collection<Deleter> getDeleters() {
        return deleters.values();
    }

    /**
     * Adds a converter mutation.
     */
    public void addConverter(Converter converter) {
        converters.put(new Key(converter), converter);
    }

    /**
     * Returns the converter mutation for the given class, version and field,
     * or null if none exists.  A null field name should be specified to get a
     * class converter.
     */
    public Converter getConverter(String className,
                                  int classVersion,
                                  String fieldName) {
        return converters.get(new Key(className, classVersion, fieldName));
    }

    /**
     * Returns an unmodifiable collection of all converter mutations.
     */
    public Collection<Converter> getConverters() {
        return converters.values();
    }

    private static class Key extends Mutation {
        static final long serialVersionUID = 2793516787097085621L;

        Key(String className, int classVersion, String fieldName) {
            super(className, classVersion, fieldName);
        }

        Key(Mutation mutation) {
            super(mutation.getClassName(),
                  mutation.getClassVersion(),
                  mutation.getFieldName());
        }
    }

    /**
     * Returns true if this collection has the same set of mutations as the
     * given collection and all mutations are equal.
     */
    @Override
    public boolean equals(Object other) {
        if (other instanceof Mutations) {
            Mutations o = (Mutations) other;
            return renamers.equals(o.renamers) &&
                   deleters.equals(o.deleters) &&
                   converters.equals(o.converters);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return renamers.hashCode() +
               deleters.hashCode() +
               converters.hashCode();
    }

    @Override
    public String toString() {
        StringBuffer buf = new StringBuffer();
        if (renamers.size() > 0) {
            buf.append(renamers.values());
        }
        if (deleters.size() > 0) {
            buf.append(deleters.values());
        }
        if (converters.size() > 0) {
            buf.append(converters.values());
        }
        if (buf.length() > 0) {
            return buf.toString();
        } else {
            return "[Empty Mutations]";
        }
    }
}
