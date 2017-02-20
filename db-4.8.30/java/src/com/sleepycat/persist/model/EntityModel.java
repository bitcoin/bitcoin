/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

import com.sleepycat.persist.EntityStore;
import com.sleepycat.persist.PrimaryIndex;
import com.sleepycat.persist.SecondaryIndex;
import com.sleepycat.persist.impl.Format;
import com.sleepycat.persist.impl.PersistCatalog;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.persist.raw.RawType;

/**
 * The base class for classes that provide entity model metadata.  An {@link
 * EntityModel} defines entity classes, primary keys, secondary keys, and
 * relationships between entities.  For each entity class that is part of the
 * model, a single {@link PrimaryIndex} object and zero or more {@link
 * SecondaryIndex} objects may be accessed via an {@link EntityStore}.
 *
 * <p>The built-in entity model, the {@link AnnotationModel}, is based on
 * annotations that are added to entity classes and their key fields.
 * Annotations are used in the examples in this package, and it is expected
 * that annotations will normally be used; most readers should therefore skip
 * to the {@link AnnotationModel} class.  However, a custom entity model class
 * may define its own metadata.  This can be used to define entity classes and
 * keys using mechanisms other than annotations.</p>
 *
 * <p>A concrete entity model class should extend this class and implement the
 * {@link #getClassMetadata}, {@link #getEntityMetadata} and {@link
 * #getKnownClasses} methods.</p>
 *
 * <p>This is an abstract class rather than an interface to allow adding
 * capabilities to the model at a future date without causing
 * incompatibilities.  For example, a method may be added in the future for
 * returning new information about the model and subclasses may override this
 * method to return the new information.  Any new methods will have default
 * implementations that return default values, and the use of the new
 * information will be optional.</p>
 *
 * @author Mark Hayes
 */
public abstract class EntityModel {

    private PersistCatalog catalog;

    /**
     * The default constructor for use by subclasses.
     */
    protected EntityModel() {
    }

    /**
     * Returns whether the model is associated with an open store.
     *
     * <p>The {@link #registerClass} method may only be called when the model
     * is not yet open.  Certain other methods may only be called when the
     * model is open:</p>
     * <ul>
     * <li>{@link #convertRawObject}</li>
     * <li>{@link #getAllRawTypeVersions}</li>
     * <li>{@link #getRawType}</li>
     * <li>{@link #getRawTypeVersion}</li>
     * </ul>
     */
    public final boolean isOpen() {
        return catalog != null;
    }

    /**
     * Registers a persistent class, most importantly, a {@link
     * PersistentProxy} class or entity subclass.
     *
     * <p>Any persistent class may be registered in advance of using it, to
     * avoid the overhead of updating the catalog database when an instance of
     * the class is first stored.  This method <em>must</em> be called in two
     * cases:</p>
     * <ol>
     * <li>to register all {@link PersistentProxy} classes, and</li>
     * <li>to register an entity subclass defining a secondary key, if {@link
     * EntityStore#getSubclassIndex getSubclassIndex} is not called for the
     * subclass.</li>
     * </ol>
     *
     * <p>For example:</p>
     *
     * <pre class="code">
     * EntityModel model = new AnnotationModel();
     * model.registerClass(MyProxy.class);
     * model.registerClass(MyEntitySubclass.class);
     *
     * StoreConfig config = new StoreConfig();
     * ...
     * config.setModel(model);
     *
     * EntityStore store = new EntityStore(..., config);</pre>
     *
     * <p>This method must be called before opening a store based on this
     * model.</p>
     *
     * @throws IllegalStateException if this method is called for a model that
     * is associated with an open store.
     *
     * @throws IllegalArgumentException if the given class is not persistent
     * or has a different class loader than previously registered classes.
     */
    public final void registerClass(Class persistentClass) {
        if (catalog != null) {
            throw new IllegalStateException("Store is already open");
        } else {
            String className = persistentClass.getName();
            ClassMetadata meta = getClassMetadata(className);
            if (meta == null) {
                throw new IllegalArgumentException
                    ("Class is not persistent: " + className);
            }
        }
    }

    /**
     * Gives this model access to the catalog, which is used for returning
     * raw type information.
     */
    void setCatalog(PersistCatalog catalog) {
        this.catalog = catalog;
    }

    /**
     * Returns the metadata for a given persistent class name, including proxy
     * classes and entity classes.
     *
     * @return the metadata or null if the class is not persistent or does not
     * exist.
     */
    public abstract ClassMetadata getClassMetadata(String className);

    /**
     * Returns the metadata for a given entity class name.
     *
     * @return the metadata or null if the class is not an entity class or does
     * not exist.
     */
    public abstract EntityMetadata getEntityMetadata(String className);

    /**
     * Returns the names of all known persistent classes.  A type becomes known
     * when an instance of the type is stored for the first time or metadata or
     * type information is queried for a specific class name.
     *
     * @return an unmodifiable set of class names.
     *
     * @throws IllegalStateException if this method is called for a model that
     * is not associated with an open store.
     */
    public abstract Set<String> getKnownClasses();

    /**
     * Returns the type information for the current version of a given class,
     * or null if the class is not currently persistent.
     *
     * @param className the name of the current version of the class.
     *
     * @throws IllegalStateException if this method is called for a model that
     * is not associated with an open store.
     */
    public final RawType getRawType(String className) {
        if (catalog != null) {
            return catalog.getFormat(className);
        } else {
            throw new IllegalStateException("Store is not open");
        }
    }

    /**
     * Returns the type information for a given version of a given class,
     * or null if the given version of the class is unknown.
     *
     * @param className the name of the latest version of the class.
     *
     * @param version the desired version of the class.
     *
     * @throws IllegalStateException if this method is called for a model that
     * is not associated with an open store.
     */
    public final RawType getRawTypeVersion(String className, int version) {
        if (catalog != null) {
            Format format = catalog.getLatestVersion(className);
            while (format != null) {
                if (version == format.getVersion()) {
                    return format;
                }
            }
            return null;
        } else {
            throw new IllegalStateException("Store is not open");
        }
    }

    /**
     * Returns all known versions of type information for a given class name,
     * or null if no persistent version of the class is known.
     *
     * @param className the name of the latest version of the class.
     *
     * @return an unmodifiable list of types for the given class name in order
     * from most recent to least recent.
     *
     * @throws IllegalStateException if this method is called for a model that
     * is not associated with an open store.
     */
    public final List<RawType> getAllRawTypeVersions(String className) {
        if (catalog != null) {
            Format format = catalog.getLatestVersion(className);
            if (format != null) {
                List<RawType> list = new ArrayList<RawType>();
                while (format != null) {
                    list.add(format);
                    format = format.getPreviousVersion();
                }
                return Collections.unmodifiableList(list);
            } else {
                return null;
            }
        } else {
            throw new IllegalStateException("Store is not open");
        }
    }

    /**
     * Returns all versions of all known types.
     *
     * @return an unmodifiable list of types.
     *
     * @throws IllegalStateException if this method is called for a model that
     * is not associated with an open store.
     */
    public final List<RawType> getAllRawTypes() {
        if (catalog != null) {
            return catalog.getAllRawTypes();
        } else {
            throw new IllegalStateException("Store is not open");
        }
    }

    /**
     * Converts a given raw object to a live object according to the current
     * class definitions.
     *
     * <p>The given raw object must conform to the current class definitions.
     * However, the raw type ({@link RawObject#getType}) is allowed to be from
     * a different store, as long as the class names and the value types match.
     * This allows converting raw objects that are read from one store to live
     * objects in another store, for example, in a conversion program.</p>
     */
    public final Object convertRawObject(RawObject raw) {
        return catalog.convertRawObject(raw, null);
    }

    /**
     * Calls Class.forName with the current thread context class loader.  This
     * method should be called by entity model implementations instead of
     * calling Class.forName whenever loading an application class.
     */
    public static Class classForName(String className)
        throws ClassNotFoundException {

        try {
            return Class.forName(className, true /*initialize*/,
                             Thread.currentThread().getContextClassLoader());
        } catch (ClassNotFoundException e) {
            return Class.forName(className);
        }
    }
}
