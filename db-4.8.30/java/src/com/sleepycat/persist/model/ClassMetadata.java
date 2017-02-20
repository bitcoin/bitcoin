/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.persist.model;

import java.io.Serializable;
import java.util.Collection;
import java.util.List;
import java.util.Map;

/**
 * The metadata for a persistent class.  A persistent class may be specified
 * with the {@link Entity} or {@link Persistent} annotation.
 *
 * <p>{@code ClassMetadata} objects are thread-safe.  Multiple threads may
 * safely call the methods of a shared {@code ClassMetadata} object.</p>
 *
 * <p>This and other metadata classes are classes rather than interfaces to
 * allow adding properties to the model at a future date without causing
 * incompatibilities.  Any such property will be given a default value and
 * its use will be optional.</p>
 *
 * @author Mark Hayes
 */
public class ClassMetadata implements Serializable {

    private static final long serialVersionUID = -2520207423701776679L;

    private String className;
    private int version;
    private String proxiedClassName;
    private boolean entityClass;
    private PrimaryKeyMetadata primaryKey;
    private Map<String,SecondaryKeyMetadata> secondaryKeys;
    private List<FieldMetadata> compositeKeyFields;
    private Collection<FieldMetadata> persistentFields;

    /**
     * Used by an {@code EntityModel} to construct persistent class metadata.
     * The optional {@link #getPersistentFields} property will be set to null.
     */
    public ClassMetadata(String className,
                         int version,
                         String proxiedClassName,
                         boolean entityClass,
                         PrimaryKeyMetadata primaryKey,
                         Map<String,SecondaryKeyMetadata> secondaryKeys,
                         List<FieldMetadata> compositeKeyFields) {

        this(className, version, proxiedClassName, entityClass, primaryKey,
             secondaryKeys, compositeKeyFields, null /*persistentFields*/);
    }

    /**
     * Used by an {@code EntityModel} to construct persistent class metadata.
     */
    public ClassMetadata(String className,
                         int version,
                         String proxiedClassName,
                         boolean entityClass,
                         PrimaryKeyMetadata primaryKey,
                         Map<String,SecondaryKeyMetadata> secondaryKeys,
                         List<FieldMetadata> compositeKeyFields,
                         Collection<FieldMetadata> persistentFields) {
        this.className = className;
        this.version = version;
        this.proxiedClassName = proxiedClassName;
        this.entityClass = entityClass;
        this.primaryKey = primaryKey;
        this.secondaryKeys = secondaryKeys;
        this.compositeKeyFields = compositeKeyFields;
        this.persistentFields = persistentFields;
    }

    /**
     * Returns the name of the persistent class.
     */
    public String getClassName() {
        return className;
    }

    /**
     * Returns the version of this persistent class.  This may be specified
     * using the {@link Entity#version} or {@link Persistent#version}
     * annotation.
     */
    public int getVersion() {
        return version;
    }

    /**
     * Returns the class name of the proxied class if this class is a {@link
     * PersistentProxy}, or null otherwise.
     */
    public String getProxiedClassName() {
        return proxiedClassName;
    }

    /**
     * Returns whether this class is an entity class.
     */
    public boolean isEntityClass() {
        return entityClass;
    }

    /**
     * Returns the primary key metadata for a key declared in this class, or
     * null if none is declared.  This may be specified using the {@link
     * PrimaryKey} annotation.
     */
    public PrimaryKeyMetadata getPrimaryKey() {
        return primaryKey;
    }

    /**
     * Returns an unmodifiable map of key name (which may be different from
     * field name) to secondary key metadata for all secondary keys declared in
     * this class, or null if no secondary keys are declared in this class.
     * This metadata may be specified using {@link SecondaryKey} annotations.
     */
    public Map<String,SecondaryKeyMetadata> getSecondaryKeys() {
        return secondaryKeys;
    }

    /**
     * Returns an unmodifiable list of metadata for the fields making up a
     * composite key, or null if this is a not a composite key class.  The
     * order of the fields in the returned list determines their stored order
     * and may be specified using the {@link KeyField} annotation.  When the
     * composite key class does not implement {@link Comparable}, the order of
     * the fields is the relative sort order.
     */
    public List<FieldMetadata> getCompositeKeyFields() {
        return compositeKeyFields;
    }

    /**
     * Returns an unmodifiable list of metadata for the persistent fields in
     * this class, or null if the default rules for persistent fields should be
     * used.  All fields returned must be declared in this class and must be
     * non-static.
     *
     * <p>By default (if null is returned) the persistent fields of a class
     * will be all declared instance fields that are non-transient (are not
     * declared with the <code>transient</code> keyword).  The default rules
     * may be overridden by an {@link EntityModel}.  For example, the {@link
     * AnnotationModel} overrides the default rules when the {@link
     * NotPersistent} or {@link NotTransient} annotation is specified.</p>
     */
    public Collection<FieldMetadata> getPersistentFields() {
        return persistentFields;
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof ClassMetadata) {
            ClassMetadata o = (ClassMetadata) other;
            return version == o.version &&
                   entityClass == o.entityClass &&
                   nullOrEqual(className, o.className) &&
                   nullOrEqual(proxiedClassName, o.proxiedClassName) &&
                   nullOrEqual(primaryKey, o.primaryKey) &&
                   nullOrEqual(secondaryKeys, o.secondaryKeys) &&
                   nullOrEqual(compositeKeyFields, o.compositeKeyFields);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return version +
               (entityClass ? 1 : 0) +
               hashCode(className) +
               hashCode(proxiedClassName) +
               hashCode(primaryKey) +
               hashCode(secondaryKeys) +
               hashCode(compositeKeyFields);
    }

    static boolean nullOrEqual(Object o1, Object o2) {
        if (o1 == null) {
            return o2 == null;
        } else {
            return o1.equals(o2);
        }
    }

    static int hashCode(Object o) {
        if (o != null) {
            return o.hashCode();
        } else {
            return 0;
        }
    }
}
